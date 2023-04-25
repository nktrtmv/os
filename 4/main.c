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



#define GARDEN_X 3
#define GARDEN_Y 3
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

void sem_wait(int semid) {
 struct sembuf parent_buf = {
    .sem_num = 0,
    .sem_op = -1,
    .sem_flg = 0
  };
  if (semop(semid, & parent_buf, 1) < 0) {
    printf("Can\'t sub 1 from semaphor\n");
    exit(-1);
  }
}

void sem_post(int semid) {
 struct sembuf parent_buf = {
    .sem_num = 0,
    .sem_op = 1,
    .sem_flg = 0
  };
  if (semop(semid, & parent_buf, 1) < 0) {
    printf("Can\'t add 1 to semaphor\n");
    exit(-1);
  }  
}


void worker(shared_memory* shmptr, int index, int work_time)  {
  	srand(time(NULL) ^ (getpid()<<16));
  	printf("I am worker #%d\n", index);
  	int x, y;
  	int dir = 1;
  	if (index == 0) {
		x = 0;
		y = 0;
  	} else {
		x = GARDEN_X - 1;
  		y = GARDEN_Y - 1;
  	}
  	shmptr->garden[x][y] = WORKED;
  	while (keepRunning) {
  		if (index == 0) {
  			y += dir;
  			if (dir == 1 && y >= GARDEN_Y) {
  				y -= dir;
  				x++;
  				dir *= -1;
  			}
  			if (dir == -1 && y < 0) {
  				y -= dir;
  				x++;
  				dir *= -1;
  			}
  			if (x >= GARDEN_X) {
  				printf("[0] Finished\n");
  				break;
  			}
  		} else {
  			x -= dir;
  			if (dir == 1 && x < 0) {
  				y--;
  				x += dir;
  				dir *= -1;
  			}
  			if (dir == -1 && x >= GARDEN_X) {
  				y--;
  				x += dir;
  				dir *= -1;
  			}
  			if (y < 0) {
  				printf("[1] Finished\n");
  				break;
  			}
  		}
  		printf("[%d] Going to [%d][%d]\n", index, x, y);
  		sem_wait(shmptr->sem_id);
  		int value = shmptr->garden[x][y];
  		sem_post(shmptr->sem_id);
  		if (value > 0) {
  			printf("[%d] Waiting for worker # %d to finish\n", index, shmptr->garden[x][y]);
  			
  			do {
  				sleep(1);
  			} while(shmptr->garden[x][y] > 0);
  		}
  		sem_wait(shmptr->sem_id);
  		if (value == CLEAR){ 
  			shmptr->garden[x][y] = index + 1;
  			sem_post(shmptr->sem_id);
  			printf("[%d] Working on cell [%d][%d]\n", index, x, y);
  			sleep(work_time);
  			sem_wait(shmptr->sem_id);
  			shmptr->garden[x][y] = WORKED;
  			sem_post(shmptr->sem_id);
  			printf("[%d] Finished working on cell [%d][%d]\n", index, x, y);
  		} else {
  			printf("[%d] Nothing to do [%d][%d]\n", index, x, y);
  			sem_post(shmptr->sem_id);
  			sleep(MOVE_TIME);
  		}
  	}
  exit(0);
}



int main(int argc, char ** argv) {
	if (argc < 3) {
	printf("usage: ./main <1st work time> <2nd work time>");
	return 1;
	}
	signal(SIGINT, intHandler);
	int work_times[] = {atoi(argv[1]), atoi(argv[2])};
	key_t key = ftok(argv[0], 0);
	shared_memory *shm;
	int semid;
	int shmid;

	int workers[2];

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

	for (int i = 0; i < 2; i++) {
		workers[i] = fork();
		if (workers[i] == 0) {
	  		worker(shm, i, work_times[i]);
		}
	}

	while(keepRunning){
		sleep(1);
	}


	for (int i = 0; i < 2; i++) {
		kill(workers[i], SIGTERM);
		printf("Killing child #%d\n", workers[i]);
	}

	if (semctl(semid, 0, IPC_RMID, 0) < 0) {
	printf("Can\'t delete semaphore\n");
		return -1;
	}
	shmdt(shm);
	shmctl(shmid, IPC_RMID, NULL);
	return 0;
}