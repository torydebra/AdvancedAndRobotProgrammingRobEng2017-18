/*
 * D
 *
 * first process read user input from terminal and put in pipe1.
 * It stops when user send -1
 * Then wait for second to sort and read (and print) result from pipe2
 * Then again wait for another array from terminal
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

void sig_handler(int signumber){

	if (signumber == SIGUSR1){
		writeLog(fdLog, getpid(), "First: received signal by father, terminating\n");
		printf("First: received signal by father, terminating\n");
		close(pipe1Write);
		close(pipe2Read);
		close(fdLog);
		exit(0);
	}

	else if (signumber == SIGINT){
		//nothing, it is the father who handle this
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
	
	//first only writes on pipe1 and reads from pipe2
	close(pipe1Read);
	close(pipe2Write);

	char msg[160]; //for log messages
	int num;
	int dimArray; //number of integer in array so first knows when has to stop reading from pipe2
	int *sorted; //number ordered read from pipe2

	//setting signal
	if (signal (SIGUSR1, sig_handler) == SIG_ERR){
		writeLog(fdLog, getpid(), "First: SignalUSR1 error\n");
		perror("First: SignalUSR1 error");
		close(pipe1Write);
		close(pipe2Read);
		close(fdLog);
		return -1;
	}
	// also SIGINT to do nothing when user press ctrl+c (father handle this)
	if (signal (SIGINT, sig_handler) == SIG_ERR){
		writeLog(fdLog, getpid(), "First: SignalINT error\n");
		perror("First: SignalINT error");
		close(pipe1Write);
		close(pipe2Read);
		close(fdLog);
		return -1;
	}

	//infinite loop
	while(1){
		dimArray = 0; //resetting things
		num = 0;

		while (dimArray < MAXDIM && num != -1 ){
			dimArray ++;
			printf ("First: Enter next number (put -1 to end entering numbers) (ctrl+c to terminate program)......................\n");
			writeLog(fdLog, getpid(), "First: Enter next number.........\n");
			scanf ("%d", &num);

			int written = write(pipe1Write, &num, sizeof(num));
			if (written < 0){
				perror("First: error writing on pipe1");
				writeLog(fdLog, getpid(), "First: error writing on pipe1\n");
				close(pipe2Read);
				close(pipe1Write);
				close(fdLog);
				return -1;
			}
			printf("First: number put in pipe: %d\n", num);
			if (sprintf (msg, "First: number put in pipe1: %d\n", num) <0){
				perror("First error srpintf");
				close(pipe2Read);
				close(pipe1Write);
				close(fdLog);
				return -1;
			}

			writeLog(fdLog, getpid(), msg);
			strcpy(msg, ""); //reset log message
		}

		dimArray--; //to not count the terminating integer -1
		printf ("First: end to put number in pipe1\n");
		writeLog(fdLog, getpid(), "First: end to put number in pipe1\n");


		//First now have to read ordered array from pipe2

		sorted = (int*) malloc(dimArray * sizeof(int));

		//no signal needed because if pipe empty calling read() first block itself
		//and it knows when integers are finished because it knows
		//how many there are (he reads them from inputfile) (arraySize)
		for(int i = 0; i<dimArray; i++){
			if(read(pipe2Read, &num, sizeof(int)) < 0){
				perror("First: error reading from pipe2");
				writeLog(fdLog, getpid(), "First: error reading from pipe2\n");
				close(pipe1Write);
				close(pipe2Read);
				close(fdLog);
				free(sorted);
				return -1;
			}
			sorted[i] = num;
			printf("First: number read from pipe2: %d\n", num);
			sprintf (msg, "First: number read from pipe2: %d\n", num);
			writeLog(fdLog, getpid(), msg);
			strcpy(msg, ""); //reset log message
		}

		printf ("First: end to read ordered numbers from pipe2\n");
		writeLog(fdLog, getpid(), "First: end to read ordered numbers from pipe2\n");

		printf ("First: Sorted result:\n");
		writeLog(fdLog, getpid(), "First: Sorted result:\n");
		for (int j=0; j<dimArray; j++){
			printf("%d ", sorted[j]);
			sprintf (msg, "%d\n", sorted[j]);
			writeLog(fdLog, getpid(), msg);
			strcpy(msg, ""); //reset log message
		}
		free(sorted);
		printf("\nFirst: Starting another cycle....\n");
		writeLog(fdLog, getpid(), "First: Starting another cycle....\n");
	}

	//should never been here
	return -1;
}
