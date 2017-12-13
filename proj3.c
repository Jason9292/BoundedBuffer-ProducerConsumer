/*
*Jason Mellinger U71145540
*/

#define _REENTRANT
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>		//needed to create boolean variable
#include <stdlib.h>		//needed to use exit funtions

/*key number*/
#define SHMKEY ((key_t) 1497)

//treat buffer as circular buffer
#define BUFFER_WRAP(x) x%15

//struct for shared memory
typedef struct
{
	char* value;
} shared_mem;  

//create pointers to struct
shared_mem *buf;

//declare semaphores globally because they are used in multiple functions
sem_t empty;
sem_t full;
sem_t mutex;

//declare globally because file is accessed in multiple functions
FILE* fp;
char newChar;

//producer thread
void* thread1(void *arg){
    	bool complete = false;					//variable to determine if EOF has been reached
	int i = 0;						//index variable of buffer wrap

    	while(!complete){					//while not at EOF
        	sem_wait(&empty);				//protect critical section
        	sem_wait(&mutex);
        	i++;

        	if(fscanf(fp,"%c",&newChar) != EOF){		//while not at end of file
            		buf->value[BUFFER_WRAP(i)] = newChar;	//increment buffer
        	}
		else{
			buf->value[BUFFER_WRAP(i)] = '*';	//add asterisk at end and break out of loop
            		complete = true;
        	}

        	sem_post(&mutex);
        	sem_post(&full);
    	}
}

//consumer thread
void* thread2(void *arg){
    	bool complete = false;		//variable to determine if producer has inputted * into the buffer yet
    	char val;
	int i = 0;
    
	while(!complete){
        	sem_wait(&full);
        	sem_wait(&mutex);
        	i++;

        	sleep(1);		//1 second sleep so consumer runs slower than producer

        	if((val = buf->value[BUFFER_WRAP(i)]) != '*'){		
            		printf("Consumer: %c\n",val);
        	}
		else{
			complete = true;				//if end of buffer exit from loop
		}

        	sem_post(&mutex);
        	sem_post(&empty);
    	}
}

int main(void){

	//open data file
    	fp = fopen("mytest.dat", "r");

    	int shmid;   

	char *shmadd;
    	shmadd = (char *) 0;

    	pthread_t tid1[1];    	/* process id for thread 1 */
    	pthread_t tid2[1];     	/* process id for thread 2 */
    	pthread_attr_t attr;   	/* attribute pointer */

    	/* Create and connect to a shared memory segment*/
    	if ((shmid = shmget (SHMKEY, sizeof(int), IPC_CREAT | 0666)) < 0){
      		perror ("shmget");
      		exit (1);
    	}

    	if ((buf = (shared_mem *) shmat (shmid, shmadd, 0)) == (shared_mem *) -1){
      		perror ("shmat");
      		exit (0);
    	}

	//create buffer of size 15
    	char buffer[15];
    	buf->value = buffer;

	//initialize semaphores
    	sem_init(&empty,0,15);
    	sem_init(&full,0,0);
    	sem_init(&mutex,0,1);

    	fflush(stdout);
    	/* Required to schedule thread independently.
    	Otherwise use NULL in place of attr. */
    	pthread_attr_init(&attr);
    	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);  
    	/* end to schedule thread independently */

    	//create threads
    	pthread_create(&tid1[0], &attr, thread1, NULL);
    	pthread_create(&tid2[0], &attr, thread2, NULL);

    	//wait for threads to finish
    	pthread_join(tid1[0], NULL);
    	pthread_join(tid2[0], NULL);

	//when threads finish program is over
    	printf("\t\t    End of simulation\n");

	//both threads finish time to destroy semaphores
    	sem_destroy(&empty);
    	sem_destroy(&full);
    	sem_destroy(&mutex);

    	//release shared memory
    	if ((shmctl (shmid, IPC_RMID, (struct shmid_ds *) 0)) == -1){
      		perror ("shmctl");
      		exit (-1);
    	}

	//close file
    	fclose(fp);
    	exit(0);    
}
