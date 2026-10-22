#include "child.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int sum_line(const char *line, long long *sum)
{
    const char *current = line;
    char *end;
    long long result = 0;
    int numbers = 0;

    while (*current != '\0') {
        long number;

        while (*current == ' ' || *current == '\t' ||
               *current == '\r' || *current == '\n') {
            ++current;
        }
        if (*current == '\0') {
            break;
        }

        errno = 0;
        number = strtol(current, &end, 10);
        if (end == current || errno == ERANGE ||
            number < INT_MIN || number > INT_MAX) {
            return EXIT_FAILURE;
        }
        if ((number > 0 && result > LLONG_MAX - number) ||
            (number < 0 && result < LLONG_MIN - number)) {
            return EXIT_FAILURE;
        }

        result += number;
        current = end;
        ++numbers;
    }

    if (numbers == 0) {
        return EXIT_FAILURE;
    }
    *sum = result;
    return EXIT_SUCCESS;
}

int run_child(int input_file, int pipe_write)
{
    char line[1024];
    int result = EXIT_SUCCESS;

    if (dup2(input_file, STDIN_FILENO) < 0) {
        perror("child: dup2 input");
        return EXIT_FAILURE;
    }
    if (dup2(pipe_write, STDOUT_FILENO) < 0) {
        perror("child: dup2 pipe");
        return EXIT_FAILURE;
    }
    if (close(input_file) < 0) {
        perror("child: close input");
        return EXIT_FAILURE;
    }
    if (close(pipe_write) < 0) {
        perror("child: close pipe");
        return EXIT_FAILURE;
    }

    if (dprintf(STDOUT_FILENO, "[child %ld] started\n",
                (long)getpid()) < 0) {
        perror("child: write started");
        return EXIT_FAILURE;
    }

    while (fgets(line, sizeof(line), stdin) != NULL) {
        long long sum;

        if (sum_line(line, &sum) != EXIT_SUCCESS) {
            fprintf(stderr, "[child %ld] bad line: %s",
                    (long)getpid(), line);
            result = EXIT_FAILURE;
            continue;
        }
        if (dprintf(STDOUT_FILENO, "[child %ld] sum = %lld\n",
                    (long)getpid(), sum) < 0) {
            perror("child: write result");
            result = EXIT_FAILURE;
            break;
        }
    }

    if (ferror(stdin)) {
        perror("child: read");
        result = EXIT_FAILURE;
    }
    if (dprintf(STDOUT_FILENO, "[child %ld] finished\n",
                (long)getpid()) < 0) {
        perror("child: write finished");
        result = EXIT_FAILURE;
    }
    return result;
}
