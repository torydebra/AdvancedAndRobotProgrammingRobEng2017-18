/*
 * C
 *
 * second process get number from pipe1 sort it and send them back through pipe2
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "writeLog.h"

//to use qsort() function
int compare (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

/*
 * argc must be 7
 * argv (in this order): nameExecutable, input text, fd of first pipe for read,
 * 						 fd of first pipe for write, fd of second pipe for read,
 					     fd of second pipe for write, fd of log file	  
 */
int main(int argc, char **argv)
{
	int numbers[100];
	int size = 0; //numbers of integer in the file
	int pipe1Read = atoi(argv[2]);
	int pipe1Write = atoi(argv[3]);
	int pipe2Read = atoi(argv[4]);
	int pipe2Write = atoi(argv[5]);
	int fdLog = atoi(argv[6]);
	char msg[160]; //for log messages
	close(pipe1Write);
	close(pipe2Read);

	int num = 0;
	while (num != -1){
		if(read(pipe1Read, &num, sizeof(int)) < 0){
			perror("Second: error reading from pipe1");
			writeLog(fdLog, getpid(), "Second: error reading from pipe1\n");
			close(pipe1Read);
			close(pipe2Write);
			close(fdLog);
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
	close(pipe1Read);

	int toSort[size-1]; //-1 because last element is -1 (to not sorting)
	for (int j=0; j<size-1; j++){ // to eliminate empty elements of array numbers which has dim 100
		toSort[j] = numbers[j];
	}

	qsort(toSort, size-1, sizeof(int), compare);

	printf ("Second: array sorted, ready to put it in pipe2\n");
	writeLog(fdLog, getpid(), "Second: array sorted, ready to put it in pipe2\n");

	//putting in pipe2
	for (int j=0; j<size-1; j++){
		num = toSort[j];
		int written = write(pipe2Write, &num, sizeof(num));
		if (written < 0){
			perror("Second: error writing on pipe2");
			writeLog(fdLog, getpid(), "Second: error writing on pipe2: \n");
			close(pipe2Write);
			close(fdLog);
			return -1;
		}
		printf("Second: number put in pipe2: %d\n", num);
		if (sprintf (msg, "Second: number put in pipe2: %d\n", num) <0){
			perror("Second: ");
			close(pipe2Write);
			close(fdLog);
			return -1;
		}
		writeLog(fdLog, getpid(), msg);
		strcpy(msg, ""); //reset log message

	}

	printf ("Second: end to put numbers in pipe2\n");
	writeLog(fdLog, getpid(), "Second: end to put numbers in pipe2\n");
	close(pipe2Write);
	close(fdLog);
	return 0;
}

