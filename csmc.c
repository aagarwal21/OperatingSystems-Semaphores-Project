// To compile: gcc csmc.c -lpthread -o csmc

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

int go = 1;
int helpTime, studentBuffer = 0, chairs, maxSleep = 3, numHelp;
sem_t studentLock, timeLock, studentWait, tutorWait, tutorNote, studentNote, idLock;

void *coordinatorFunction(void *vargp){
	while(go){	//Loop while we still have to help students
		sem_wait(&studentNote);	//Wait to be notified by both a student and tutor
		sem_wait(&tutorNote);

		sem_wait(&timeLock);	//Wait on timeLock twice because both the student and the tutor will have to read it before coordinator can change it again
		sem_wait(&timeLock);
		helpTime = (rand() % maxSleep) + 1;

		sem_post(&studentWait);
		sem_wait(&studentLock);	//We decrement studentBuffer here rather than in studentFunction so that tutorFunction is guaranteed to get an accurate count
		studentBuffer--;
		sem_post(&tutorWait);
	}
}

void *studentFunction(void *vargp){
	int id = *((int*)vargp);
	sem_post(&idLock);	//Because we are getting the id through a pointer, it can't be changed before studentFunction reads it
	int i;
	for(i = 0;i < numHelp;){	//Loop for the number of times they need help
		sem_wait(&studentLock);	//Get the lock for studentBuffer
		if (studentBuffer < chairs){
			studentBuffer++;
			printf("Student %d takes a seat. Waiting students = %d\n", id, studentBuffer);
			sem_post(&studentLock);	//Release the lock for studentBuffer

			sem_post(&studentNote);	//Notify the coordinator that this student is waiting
			sem_wait(&studentWait);	//Wait to be woken up by the coordinator

			int time = helpTime;	//Grab the amount of time the tutor will be helping for, then allow the coordinator to set the time for the next pair
			sem_post(&timeLock);
			sleep(time);	//Sleep for the appropriate amount of time
			i++;	//Increment i to show that the student has been helped once
		}
		else{
			sem_post(&studentLock);
			printf("Student %d found no empty seats will try again later.\n", id);
		}
		sleep(rand() % maxSleep + 1);	//Sleep for programming time
	}
}

void *tutorFunction(void *vargp){
	int id = *((int*)vargp);
	sem_post(&idLock);	//Same lock mechanism as above
	while(go){
		sem_post(&tutorNote);	//Notify coordinator that tutor is ready, then wait to be woken up
		sem_wait(&tutorWait);

		int time = helpTime;	//Get the amount of time to sleep for and release the timeLock
		sem_post(&timeLock);
		printf("Tutor %d helping student for %d seconds. Waiting students = %d\n", id, time, studentBuffer);
		sem_post(&studentLock);	//studentBuffer was locked up until this point so that the above printf would print an accurate number
		sleep(time);
	}
}

int main(int argc, char** argv){
	if (argc == 5) {
	int numStudents, numTutors, i;
	srand(time(0));
	char * endptr;

	sem_init(&studentLock, 0, 1);	//Initialize all the semaphores
	sem_init(&timeLock, 0, 2);
	sem_init(&studentWait, 0, 0);
	sem_init(&tutorWait, 0, 0);
	sem_init(&tutorNote, 0, 0);
	sem_init(&studentNote, 0, 0);
	sem_init(&idLock, 0, 0);

	pthread_t coordinator;
	pthread_t* students;
	pthread_t* tutors;

	numStudents = (int) strtol(argv[1], &endptr, 10);	//Read values from arguments/initialize pthread arrays
	if (endptr == argv[1] || numStudents <= 0) {
          printf("Invalid number for students\n");
	  exit(1);
	}
	numTutors = (int) strtol(argv[2], &endptr, 10);
	if (endptr == argv[2] || numTutors <= 0) {
          printf("Invalid number for tutors\n");
	  exit(1);
	}
	students = (pthread_t*) malloc(numStudents * sizeof(pthread_t));
	tutors = (pthread_t*) malloc(numTutors * sizeof(pthread_t));
	chairs = (int) strtol(argv[3], &endptr, 10);
	if (endptr == argv[3] || chairs <= 0) {
          printf("Invalid number for chairs\n");
	  exit(1);
	}
	numHelp = (int) strtol(argv[4], &endptr, 10);
	if (endptr == argv[4] || numHelp <= 0) {
          printf("Invalid number for help\n");
	  exit(1);
	}

	pthread_create(&coordinator, NULL, coordinatorFunction, NULL);	//Start up all the threads
	for (i = 0;i < numStudents;i++){
		pthread_create(&students[i], NULL, studentFunction, (void*)&i);
		sem_wait(&idLock);
	}
	for (i = 0;i < numTutors;i++){
		pthread_create(&tutors[i], NULL, tutorFunction, (void*)&i);
		sem_wait(&idLock);
	}

	for(i = 0;i < numStudents;i++){	//Wait for students to finish
		pthread_join(students[i], NULL);
	}
	go = 0;	//Signal coordinator and tutors the program is done

	free(students);
	free(tutors);
	}
	else 
	  printf("Error! Provide 4 args. [#students] [#tutors] [#chairs] [#help]\n"); 
}
