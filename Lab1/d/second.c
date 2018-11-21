/*
 * D
 *
 * second process get number from pipe1 sort it and send them back through pipe2
 *
 * if signal by father is received, it ends execution.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include "writeLog.h"
#define MAXDIM 100

int pipe1Read;
int pipe1Write;
int pipe2Read;
int pipe2Write;
int fdLog;

//to use qsort() function
int compare (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}


void sig_handler(int signumber){

	if (signumber == SIGUSR1){
		writeLog(fdLog, getpid(), "Second: received signal by father, terminating\n");
		printf("Second: received signal by father, terminating\n");
		close(pipe1Read);
		close(pipe2Write);
		close(fdLog);
		exit(0);
	}

	else if (signumber == SIGINT){
		//nothing, it is the first son who handle this
	}
}


/*
 * argc must be 6
 * argv (in this order): nameExecutable, fd of first pipe for read,
 * 						 fd of first pipe for write, fd of second pipe for read,
 *   					 fd of second pipe for write, fd of log file	  
 */
int main(int argc, char **argv)
{
	pipe1Read = atoi(argv[1]);
	pipe1Write = atoi(argv[2]);
	pipe2Read = atoi(argv[3]);
	pipe2Write = atoi(argv[4]);
	fdLog = atoi(argv[5]);

	int* numbers;
	int* toSort;
	int size; //numbers of integer in the file

	char msg[160]; //for log messages
	int num;
	//second only read from pipe1 and write on pipe2
	close(pipe1Write); 
	close(pipe2Read);

	//setting signal
	if (signal (SIGUSR1, sig_handler) == SIG_ERR){
		writeLog(fdLog, getpid(), "Second: SignalUSR1 error\n");
		perror("Second: SignalUSR1 error");
		close(pipe1Read);
		close(pipe2Write);
		close(fdLog);
		return -1;
	}
	// also SIGINT to do nothing when user press ctrl+c (first son handle this)
	if (signal (SIGINT, sig_handler) == SIG_ERR){
		writeLog(fdLog, getpid(), "Second: SignalINT error\n");
		perror("Second: SignalINT error");
		close(pipe1Read);
		close(pipe2Write);
		close(fdLog);
		return -1;
	}

	//infinite loop
	while(1){
		num = 0;
		size = 0;
		numbers = (int*) malloc(MAXDIM*sizeof(int));

		while (num != -1){
			if(read(pipe1Read, &num, sizeof(int)) < 0){
				perror("Second: error reading from pipe1");
				writeLog(fdLog, getpid(), "Second: error reading from pipe1\n");
				close(pipe1Read);
				close(pipe2Write);
				close(fdLog);
				free(numbers);
				return -1;
			}
			numbers[size] = num;
			printf("Second: number read from pipe1: %d\n", num);
			sprintf (msg, "Second: number read from pipe1: %d\n", num);
			writeLog(fdLog, getpid(), msg);
			strcpy(msg, ""); //reset log message
			size++;
		}

		printf ("Second: end to read numbers from pipe1\n");
		writeLog(fdLog, getpid(), "Second: end to read numbers from pipe1\n");

		toSort = malloc((size-1)*sizeof(int)); //-1 because last element is -1 (to not sorting)
		for (int j=0; j<size-1; j++){ // to eliminate empty elements of array numbers which has dim 100
			toSort[j] = numbers[j];
		}
		free(numbers);

		qsort(toSort, size-1, sizeof(int), compare);

		printf ("Second: array sorted, ready to put it in pipe2\n");
		writeLog(fdLog, getpid(), "Second: array sorted, ready to put it in pipe2\n");

		//putting in pipe2
		for (int j=0; j<size-1; j++){
			num = toSort[j];
			int written = write(pipe2Write, &num, sizeof(num));
			if (written < 0){
				perror("Second: error writing on pipe2");
				writeLog(fdLog, getpid(), "Second: error writing on pipe2\n");
				close(pipe1Read);
				close(pipe2Write);
				close(fdLog);
				free(toSort);
				return -1;
			}
			printf("Second: number put in pipe2: %d\n", num);
			if (sprintf (msg, "Second: number put in pipe2: %d\n", num) <0){
				perror("Second sprintf error");
				close(pipe1Read);
				close(pipe2Write);
				close(fdLog);
				free(toSort);
				return -1;
			}
			writeLog(fdLog, getpid(), msg);
			strcpy(msg, ""); //reset log message

		}

		printf ("Second: end to put numbers in pipe2\n");
		writeLog(fdLog, getpid(), "Second: end to put numbers in pipe2\n");

		free(toSort);
		printf("Second: Starting another cycle....\n");
	}


    // should never been here
	return -1;
}

