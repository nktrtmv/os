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

    int fd_input = open(input_file, O_RDONLY);
    if (fd_input == -1) {
        perror("Error opening input file");
        return 1;
    }

    int pipe_fd[2];
    pipe(pipe_fd);

    pid_t pid;

    if ((pid = fork()) == 0) {
        // Child process
        close(pipe_fd[0]);

        char buffer[BUFFER_SIZE];
        ssize_t bytes_read = read(fd_input, buffer, BUFFER_SIZE);
        close(fd_input);

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

        write(pipe_fd[1], result, result_size);
        close(pipe_fd[1]);
        exit(0);
    }

    // Parent process
    close(pipe_fd[1]);

    int fd_output = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_output == -1) {
        perror("Error opening output file");
        return 1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(pipe_fd[0], buffer, BUFFER_SIZE);
    close(pipe_fd[0]);
    write(fd_output, buffer, bytes_read);

    close(fd_input);
    close(fd_output);

    return 0;
}
