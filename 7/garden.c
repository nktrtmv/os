#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>



#define GARDEN_X 30
#define GARDEN_Y 30
#define OBSTACLE_COUNT 6
#define MOVE_TIME 1
#define CLEAR 0
#define OBSTACLE -2
#define WORKED -1


static volatile int keepRunning = 1;

typedef struct shared_memory {
	int sem_id;
 	int garden[GARDEN_X][GARDEN_Y];
}shared_memory;



void intHandler(int dummy) {
    printf("SIGINT\n");
    keepRunning = 0;
}

int main(int argc, char ** argv) {
	signal(SIGINT, intHandler);
	key_t key = ftok("garden.c", 0);
	shared_memory *shm;
	int semid;
	int shmid;

	if ((semid = semget(key, 1, 0666 | IPC_CREAT)) < 0) {
	printf("Can\'t create semaphore\n");
	return -1;
	}

	if ((shmid = shmget(key, sizeof(shared_memory), 0666 | IPC_CREAT)) < 0) {
	printf("Can\'t create shmem\n");
	return -1;
	}
	if ((shm = (shared_memory*) shmat(shmid, NULL, 0)) == (shared_memory *) -1) {
	printf("Cant shmat!\n");
	return -1;
	} 
	semctl(semid, 0, SETVAL, 1);

	for (int i = 0; i < GARDEN_X; i++) {
		for (int j = 0; j < GARDEN_Y; j++) {
			shm->garden[i][j] = CLEAR;
		}
	}

	for (int i = 0; i < OBSTACLE_COUNT; i++) {
		shm->garden[rand() % GARDEN_X][rand() % GARDEN_Y] = OBSTACLE;
	}

	shm->garden[0][0] = 1;
	shm->garden[GARDEN_X - 1][GARDEN_Y - 1] = 2;
	shm->sem_id = semid;


	for (int i = 0; i < GARDEN_X; i++) {
		for (int j = 0; j < GARDEN_Y; j++) {
			printf("[%d] ", shm->garden[i][j]);
		}
		printf("\n");
	}

	while(keepRunning){
		sleep(1);
	}

	if (semctl(semid, 0, IPC_RMID, 0) < 0) {
	printf("Can\'t delete semaphore\n");
		return -1;
	}
	shmdt(shm);
	shmctl(shmid, IPC_RMID, NULL);
	return 0;
}