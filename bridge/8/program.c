#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 5000

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s n input_file output_file\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    char *input_file = argv[2];
    char *output_file = argv[3];

    char *pipe1_name = "pipe1";
    mkfifo(pipe1_name, 0666);

    pid_t pid1 = fork();
    if (pid1 == 0) {
        // Child process 1
        int fd_input = open(input_file, O_RDONLY);
        if (fd_input == -1) {
            perror("Error opening input file");
            return 1;
        }

        int pipe1_fd = open(pipe1_name, O_WRONLY);

        char buffer[BUFFER_SIZE];
        ssize_t bytes_read = read(fd_input, buffer, BUFFER_SIZE);
        write(pipe1_fd, buffer, bytes_read);

        close(pipe1_fd);
        close(fd_input);
        exit(0);
    }

    char *pipe2_name = "pipe2";
    mkfifo(pipe2_name, 0666);

    pid_t pid2 = fork();
    if (pid2 == 0) {
        // Child process 2
        int pipe1_fd = open(pipe1_name, O_RDONLY);
        int pipe2_fd = open(pipe2_name, O_WRONLY);

        char buffer[BUFFER_SIZE];
        ssize_t bytes_read = read(pipe1_fd, buffer, BUFFER_SIZE);
        close(pipe1_fd);

        char result[BUFFER_SIZE] = {0};
        ssize_t result_size = 0;

        for (ssize_t i = 0; i < bytes_read - n + 1; i++) {
            int is_decreasing = 1;
            for (int j = 1; j < n; j++) {
                if (buffer[i + j] >= buffer[i + j - 1]) {
                    is_decreasing = 0;
                    break;
                }
            }

            if (is_decreasing) {
                for (int j = 0; j < n; j++) {
                    result[result_size++] = buffer[i + j];
                }
                break;
            }
        }

        write(pipe2_fd, result, result_size);
        close(pipe2_fd);
        exit(0);
    }

    // Parent process
    int pipe2_fd = open(pipe2_name, O_RDONLY);

    int fd_output = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_output == -1) {
        perror("Error opening output file");
        return 1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(pipe2_fd, buffer, BUFFER_SIZE);
    close(pipe2_fd);
    write(fd_output, buffer, bytes_read);

    close(fd_output);

    // Удаление именованных каналов
    unlink(pipe1_name);
    unlink(pipe2_name);

    return 0;
}
