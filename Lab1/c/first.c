/*
 * C
 *
 * first process read file input and put in pipe1.
 * Then wait for second to sort and read (and print) result from pipe2
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "writeLog.h"

/*
 * argc must be 7
 * argv (in this order): nameExecutable, input text, fd of first pipe for read,
 * 						 fd of first pipe for write, fd of second pipe for read,
 					     fd of second pipe for write, fd of log file	  
 */
int main(int argc, char **argv)
{
	int number;
	int pipe1Read = atoi(argv[2]);
	int pipe1Write = atoi(argv[3]);
	int pipe2Read = atoi(argv[4]);
	int pipe2Write = atoi(argv[5]);
	int fdLog = atoi(argv[6]);
	char msg[160]; //for log messages
	int arraySize = 0; //number of integer in array so first knows when has to stop reading from pipe2
	close(pipe1Read);
	close(pipe2Write);

	FILE* inputFile = fopen (argv[1],"r");
	if (inputFile == NULL){
		writeLog(fdLog, getpid(), "First: error opening inputFile\n");
		perror("fopen error");
		close(pipe2Read);
		close(pipe1Write);
		close(fdLog);
		return -1;
	}

	while(fscanf(inputFile, "%d", &number) != EOF){
		arraySize++;
		printf("First: number read from file: %d\n", number);
		if (sprintf (msg, "First: number read from file: %d\n", number) <0){
			perror("First: ");
			close(pipe2Read);
			close(pipe1Write);
			close(fdLog);
			return -1;
		}
		writeLog(fdLog, getpid(), msg);
		strcpy(msg, ""); //reset log message

		int written = write(pipe1Write, &number, sizeof(number));
		if (written < 0){
			perror("error writing on pipe1");
			writeLog(fdLog, getpid(), "error writing on pipe1: \n");
			close(pipe2Read);
			close(pipe1Write);
			close(fdLog);
			return -1;
		}

		printf("First: number put in pipe1: %d\n", number);
		if (sprintf (msg, "First: number put in pipe1: %d\n", number) <0){
			perror("First");
			close(pipe2Read);
			close(pipe1Write);
			close(fdLog);
			return -1;
		}
		writeLog(fdLog, getpid(), msg);
		strcpy(msg, ""); //reset log message
		//sleep(1); //to see that first stop execution for second

	}
	arraySize--; //to not count the terminating integer -1
	fclose(inputFile);
	printf ("First: end to put number in pipe1\n");
	writeLog(fdLog, getpid(), "First: end to put number in pipe1\n");
	close(pipe1Write);


	//First now have to read ordered array from pipe2

	//no signal needed because if pipe empty calling read() first block itself
	//and it knows when integers are finished because it knows
	//how many there are (he reads them from inputfile) (arraySize)
	int num =0;
	int sorted[arraySize];
	for(int i = 0; i<arraySize; i++){
		if(read(pipe2Read, &num, sizeof(int)) < 0){
			perror("First: error reading from pipe2");
			writeLog(fdLog, getpid(), "First: error reading from pipe2\n");
			close(pipe2Read);
			close(fdLog);
			return -1;
		}
		sorted[i] = num;
		printf("First: number read from pipe2: %d\n", num);
		sprintf (msg, "First: number read from pipe2: %d\n", num);
		writeLog(fdLog, getpid(), msg);
		strcpy(msg, ""); //reset log message
	}

	printf ("First: end to read ordered numbers from pipe2\n");
	writeLog(fdLog, getpid(), "Second: end to read ordered numbers from pipe2\n");
	close(pipe2Read);

	printf ("First: Sorted result:\n");
	writeLog(fdLog, getpid(), "First: Sorted result:\n");
	for (int j=0; j<arraySize; j++){
		printf("%d ", sorted[j]);
		sprintf (msg, "%d\n", sorted[j]);
		writeLog(fdLog, getpid(), msg);
		strcpy(msg, ""); //reset log message
	}
	printf("\n");

	close (fdLog);
	return 0;
}

