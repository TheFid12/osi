#include "parent.h"
#include "child.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int ask_for_file(char *name, size_t size)
{
    size_t length;

    printf("File name: ");
    if (fflush(stdout) == EOF) {
        perror("fflush");
        return 0;
    }

    if (fgets(name, size, stdin) == NULL) {
        fprintf(stderr, "Cannot read file name\n");
        return 0;
    }

    length = strlen(name);
    while (length > 0 &&
           (name[length - 1] == '\n' || name[length - 1] == '\r')) {
        name[--length] = '\0';
    }
    if (name[0] == '\0') {
        fprintf(stderr, "File name is empty\n");
        return 0;
    }
    return 1;
}

static int write_all(int file, const char *text, size_t size)
{
    size_t sent = 0;

    while (sent < size) {
        ssize_t count = write(file, text + sent, size - sent);

        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            return EXIT_FAILURE;
        }
        if (count == 0) {
            errno = EIO;
            return EXIT_FAILURE;
        }
        sent += (size_t)count;
    }
    return EXIT_SUCCESS;
}

static int read_child_output(int pipe_read)
{
    char buffer[4096];
    int result = EXIT_SUCCESS;

    while (1) {
        ssize_t count = read(pipe_read, buffer, sizeof(buffer));

        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("parent: read pipe");
            result = EXIT_FAILURE;
            break;
        }
        if (count == 0) {
            break;
        }
        if (write_all(STDOUT_FILENO, buffer, (size_t)count) !=
            EXIT_SUCCESS) {
            perror("parent: write");
            result = EXIT_FAILURE;
            break;
        }
    }

    if (close(pipe_read) < 0) {
        perror("parent: close pipe");
        result = EXIT_FAILURE;
    }
    return result;
}

static int wait_for_child(pid_t child)
{
    int status;

    while (waitpid(child, &status, 0) < 0) {
        if (errno != EINTR) {
            perror("waitpid");
            return EXIT_FAILURE;
        }
    }

    if (!WIFEXITED(status)) {
        fprintf(stderr, "Child did not exit normally\n");
        return EXIT_FAILURE;
    }
    if (WEXITSTATUS(status) != EXIT_SUCCESS) {
        fprintf(stderr, "Child returned %d\n", WEXITSTATUS(status));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int run_parent(void)
{
    char file_name[PATH_MAX];
    int file;
    int pipe_fds[2];
    pid_t child;
    int result = EXIT_SUCCESS;

    if (!ask_for_file(file_name, sizeof(file_name))) {
        return EXIT_FAILURE;
    }

    file = open(file_name, O_RDONLY);
    if (file < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (pipe(pipe_fds) < 0) {
        perror("pipe");
        if (close(file) < 0) {
            perror("close file");
        }
        return EXIT_FAILURE;
    }

    child = fork();
    if (child < 0) {
        perror("fork");
        if (close(file) < 0) {
            perror("close file");
        }
        if (close(pipe_fds[0]) < 0) {
            perror("close pipe read");
        }
        if (close(pipe_fds[1]) < 0) {
            perror("close pipe write");
        }
        return EXIT_FAILURE;
    }

    if (child == 0) {
        int child_result;

        if (close(pipe_fds[0]) < 0) {
            perror("child close");
            _exit(EXIT_FAILURE);
        }
        child_result = run_child(file, pipe_fds[1]);
        _exit(child_result);
    }

    if (close(file) < 0) {
        perror("close file");
        result = EXIT_FAILURE;
    }
    if (close(pipe_fds[1]) < 0) {
        perror("close pipe");
        result = EXIT_FAILURE;
    }
    if (read_child_output(pipe_fds[0]) != EXIT_SUCCESS) {
        result = EXIT_FAILURE;
    }
    if (wait_for_child(child) != EXIT_SUCCESS) {
        result = EXIT_FAILURE;
    }

    return result;
}
