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
    kill(getpid(), SIGTERM);
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
		printf("Usage: ./worker <number> <work time>\n");
		return 1;
	}
	signal(SIGINT, intHandler);
	key_t key = ftok("garden.c", 0);
	shared_memory *shm;
	int semid;
	int shmid;

	if ((shmid = shmget(key, sizeof(shared_memory), 0666 | IPC_CREAT)) < 0) {
	printf("Can\'t create shmem\n");
	return -1;
	}
	if ((shm = (shared_memory*) shmat(shmid, NULL, 0)) == (shared_memory *) -1) {
	printf("Cant shmat!\n");
	return -1;
	} 
	worker(shm, atoi(argv[1]), atoi(argv[2]));
	return 0;
}