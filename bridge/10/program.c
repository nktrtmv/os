#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 200
#define MSG_KEY 1234

typedef struct message {
    long msg_type;
    char msg_data[BUFFER_SIZE];
    int msg_size;
    int msg_found;
} Message;

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

    int msg_id = msgget(MSG_KEY, IPC_CREAT | 0666);

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

        while (!found) {
            Message msg;
            msgrcv(msg_id, &msg, sizeof(msg) - sizeof(long), 1, 0);

            if (msg.msg_size > 0) {
                findSequence(n, msg.msg_data, msg.msg_size, &found, result);
                msg.msg_type = 2;
                msg.msg_found = found;
                if (found) {
                    strncpy(msg.msg_data, result, n);
                }
                msgsnd(msg_id, &msg, sizeof(msg) - sizeof(long), 0);
            } else {
                break;
            }
        }

    } else {  // Parent process
        int fd2 = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd2 < 0) {
            perror("Error opening output file");
            exit(1);
        }

        char buffer[BUFFER_SIZE];
        int read_bytes;

        while ((read_bytes = read(fd1, buffer, BUFFER_SIZE)) > 0) {
            Message msg;
            msg.msg_type = 1;
            strncpy(msg.msg_data, buffer, read_bytes);
            msg.msg_size = read_bytes;
            msgsnd(msg_id, &msg, sizeof(msg) - sizeof(long), 0);

            msgrcv(msg_id, &msg, sizeof(msg) - sizeof(long), 2, 0);

            if (msg.msg_found) {

                write(fd2, msg.msg_data, n);
                break;
            }
        }

        // Отправляем сообщение с нулевым размером, чтобы завершить процесс-ребенок
        Message msg;
        msg.msg_type = 1;
        msg.msg_size = 0;
        msgsnd(msg_id, &msg, sizeof(msg) - sizeof(long), 0);

        close(fd1);
        close(fd2);
    }

    // Удаляем очередь сообщений
    if (pid > 0) {
        wait(NULL);
        msgctl(msg_id, IPC_RMID, NULL);
    }

    return 0;
}
