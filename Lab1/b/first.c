/*
 * B
 *
 * first process read file input and put in pipe
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "writeLog.h"

/*
 * argc must be 5
 * argv (in this order): nameExecutable, input text, fd of pipe for write,
 * 						 fd of pipe for read, fd of log file	  
 */
int main(int argc, char **argv)
{
	int number;
	int pipesFd = atoi(argv[2]);
	int pipesFdsec = atoi(argv[3]);
	int fdLog = atoi(argv[4]);
	char msg[160]; //per log messages
	close(pipesFdsec); // first does not read

	FILE* inputFile = fopen (argv[1],"r");
	if (inputFile == NULL){
		writeLog(fdLog, getpid(), "First: error opening inputFile\n");
		perror("open error");
		close(pipesFd);
		close(fdLog);
		return -1;
	}

	while(fscanf(inputFile, "%d", &number) != EOF){
		printf("First: number read from file: %d\n", number);
		if (sprintf (msg, "First: number read from file: %d\n", number) <0){
			perror("First");
			close(pipesFd);
			close(fdLog);
			return -1;
		}
		writeLog(fdLog, getpid(), msg);
		strcpy(msg, ""); //reset log message

		int written = write(pipesFd, &number, sizeof(number));
		if (written < 0){
			perror("error writing on pipe");
			writeLog(fdLog, getpid(), "error writing on pipe: \n");
			close(pipesFd);
			close(fdLog);
			return -1;
		}

		printf("First: number put in pipe: %d\n", number);
		if (sprintf (msg, "First: number put in pipe: %d\n", number) <0){
			perror("First");
			close(pipesFd);
			close(fdLog);
			return -1;
		}
		writeLog(fdLog, getpid(), msg);
		strcpy(msg, ""); //reset log message
		//sleep(1); //to see that first stop execution for second

	}
	fclose(inputFile);
	printf ("First: end to put number in pipe\n");
	writeLog(fdLog, getpid(), "First: end to put number in pipe\n");
	close(pipesFd);
	close(fdLog);

	return 0;
}

