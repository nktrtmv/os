#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
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
	sem_t sem;
 	int garden[GARDEN_X][GARDEN_Y];
}shared_memory;



void intHandler(int dummy) {
    printf("SIGINT\n");
    keepRunning = 0;
}

int main(int argc, char ** argv) {
	signal(SIGINT, intHandler);
	char memn[] = "shared-memory";
	char sem_name[] = "semaphore";
	int mem_size = sizeof(shared_memory);
	int shm;
	sem_t *semm;

	// СОздать память
	if ((shm = shm_open(memn, O_CREAT | O_RDWR, 0666)) == -1) {
	  printf("Object is already open\n");
	  perror("shm_open");
	  return 1;
	} else {
	  printf("Object is open: name = %s, id = 0x%x\n", memn, shm);
	}
	if (ftruncate(shm, mem_size) == -1) {
	  printf("Memory sizing error\n");
	  perror("ftruncate");
	  return 1;
	} else {
	  printf("Memory size set and = %d\n", mem_size);
	}

	//получить доступ к памяти
	void* addr = mmap(0, mem_size, PROT_WRITE, MAP_SHARED, shm, 0);
	if (addr == (int * ) - 1) {
	  printf("Error getting pointer to shared memory\n");
	  return 1;
	}

	shared_memory* shmem = addr;
	if ((semm = sem_open(sem_name, O_CREAT, 0666, 1)) < 0) {
    	printf("Error creating semaphore!\n");
      	return 1;
  	}
  	shmem->sem = *semm;

	for (int i = 0; i < GARDEN_X; i++) {
		for (int j = 0; j < GARDEN_Y; j++) {
			shmem->garden[i][j] = CLEAR;
		}
	}

	for (int i = 0; i < OBSTACLE_COUNT; i++) {
		shmem->garden[rand() % GARDEN_X][rand() % GARDEN_Y] = OBSTACLE;
	}

	shmem->garden[0][0] = 1;
	shmem->garden[GARDEN_X - 1][GARDEN_Y - 1] = 2;


	for (int i = 0; i < GARDEN_X; i++) {
		for (int j = 0; j < GARDEN_Y; j++) {
			printf("[%d] ", shmem->garden[i][j]);
		}
		printf("\n");
	}

	while(keepRunning){
		sleep(1);
	}

	if (sem_unlink(sem_name) == -1) {
    	perror ("sem_unlink"); exit (1);
  	}
	close(shm);
	if(shm_unlink(memn) == -1) {
		printf("Shared memory is absent\n");
		perror("shm_unlink");
		return 1;
	}
	return 0;
}