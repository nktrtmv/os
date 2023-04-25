#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
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
	sem_t sem;
 	int garden[GARDEN_X][GARDEN_Y];
}shared_memory;



void intHandler(int dummy) {
    printf("SIGINT\n");
    keepRunning = 0;
    kill(getpid(), SIGTERM);
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
  		sem_wait(&shmptr->sem);
  		int value = shmptr->garden[x][y];
  		sem_post(&shmptr->sem);
  		if (value > 0) {
  			printf("[%d] Waiting for worker # %d to finish\n", index, shmptr->garden[x][y]);
  			
  			do {
  				sleep(1);
  			} while(shmptr->garden[x][y] > 0);
  		}
  		sem_wait(&shmptr->sem);
  		if (value == CLEAR){ 
  			shmptr->garden[x][y] = index + 1;
  			sem_post(&shmptr->sem);
  			printf("[%d] Working on cell [%d][%d]\n", index, x, y);
  			sleep(work_time);
  			sem_wait(&shmptr->sem);
  			shmptr->garden[x][y] = WORKED;
  			sem_post(&shmptr->sem);
  			printf("[%d] Finished working on cell [%d][%d]\n", index, x, y);
  		} else {
  			printf("[%d] Nothing to do [%d][%d]\n", index, x, y);
  			sem_post(&shmptr->sem);
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
	char memn[] = "shared-memory";
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
	worker(shmem, atoi(argv[1]), atoi(argv[2]));
	return 0;
}