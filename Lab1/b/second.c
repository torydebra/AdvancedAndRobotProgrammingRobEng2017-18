/*
 * B
 *
 * second process get number from pipe and sort it
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
 * argc must be 5
 * argv (in this order): nameExecutable, input text, fd of pipe for write,
 * 						 fd of pipe for read, fd of log file	  
 */
int main(int argc, char **argv)
{

	int numbers[100];
	int size = 0; //numbers of integer in the file
	int pipesFd = atoi(argv[3]);
	int pipesFdfir = atoi(argv[2]);
	int fdLog = atoi(argv[4]);
	char msg[160]; //per log messages
	close(pipesFdfir); // second does not write

	int num = 0;
	while (num != -1){
		if( read(pipesFd, &num, sizeof(int)) < 0){
			perror("Second: error reading from pipe");
			writeLog(fdLog, getpid(), "Second: error reading from pipe\n");
			close(pipesFd);
			close(fdLog);
			return -1;
		}

		numbers[size] = num;
		printf("Second: number read from pipe: %d\n", num);
		sprintf (msg, "Second: number read from pipe: %d\n", num);
		writeLog(fdLog, getpid(), msg);
		strcpy(msg, ""); //reset log message
		size++;
	}
	printf ("Second: end to read numbers from pipe\n");
	writeLog(fdLog, getpid(), "Second: end to read numbers from pipe\n");
	close(pipesFd);

	int toSort[size-1];//-1 because last element is -1 (to not sorting)
	for (int j=0; j<size-1; j++){  // to eliminate empty elements of array numbers which has dim 100
		toSort[j] = numbers[j];
	}

	qsort(toSort, size-1, sizeof(int), compare);

	printf ("Second: Sorted result:\n");
	writeLog(fdLog, getpid(), "Second: Sorted result:\n");
	for (int j=0; j<size-1; j++){
		printf("%d ", toSort[j]);
		sprintf (msg, "%d\n", toSort[j]);
		writeLog(fdLog, getpid(), msg);
		strcpy(msg, ""); //reset log message
	}
	printf("\n");

	close(fdLog);
	return 0;
}

