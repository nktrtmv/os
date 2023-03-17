#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 200

void findSequence(int n, char *buffer, int size, int *found, char *result) {
    for (int i = 0; i < size - n + 1; i++) {
        int count = 0;
        for (int j = 0; j < n - 1; j++) {
            if (buffer[i + j] > buffer[i + j + 1]) {
                count++;
            } else {
                break;
            }
        }
        if (count == n - 1) {
            *found = 1;
            strncpy(result, buffer + i, n);
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <n> <input_file> <output_file>\n", argv[0]);
        exit(1);
    }

    int n = atoi(argv[1]);
    char *input_file = argv[2];
    char *output_file = argv[3];

    mkfifo("pipe1", 0666);
    mkfifo("pipe2", 0666);

    int fd1 = open(input_file, O_RDONLY);
    if (fd1 < 0) {
        perror("Error opening input file");
        exit(1);
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("Error creating child process");
        exit(1);
    }

    if (pid == 0) {  // Child process
        close(fd1);

        char buffer[BUFFER_SIZE];
        int found = 0;
        char result[n];

        int pipe1 = open("pipe1", O_RDONLY);
        int pipe2 = open("pipe2", O_WRONLY);

        while (!found) {
            int read_bytes = read(pipe1, buffer, BUFFER_SIZE);
            if (read_bytes > 0) {
                findSequence(n, buffer, read_bytes, &found, result);
                write(pipe2, &found, sizeof(int));
                if (found) {
                    write(pipe2, result, n);
                }
            } else {
                break;
            }
        }

        close(pipe1);
        close(pipe2);

    } else {  // Parent process
        int fd2 = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd2 < 0) {
            perror("Error opening output file");
            exit(1);
        }

        int pipe1 = open("pipe1", O_WRONLY);
        int pipe2 = open("pipe2", O_RDONLY);

        char buffer[BUFFER_SIZE];
        int read_bytes;

        while ((read_bytes = read(fd1, buffer, BUFFER_SIZE)) > 0) {
            write(pipe1, buffer, read_bytes);

            int found;
            read(pipe2, &found, sizeof(int));
            if (found) {
                char result[n];
                read(pipe2, result, n);
                write(fd2, result, n);
                break;
            }
        }

        close(fd1);
        close(fd2);
        close(pipe1);
        close(pipe2);
    }

    unlink("pipe1");
    unlink("pipe2");

    return 0;
}
