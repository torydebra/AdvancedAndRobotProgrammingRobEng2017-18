/*
 * B
 *
 * Create two processes.
 *
 * The first inputs an array of integer values from standard input or a file, sends to the second via pipe;
 * the second process sorts the array and prints results.
 * H.: read data one by one, it's simpler; use a scheme similar to threads,
 * as to say one process forks in sequence to generate processes "first" and "second", and waits for completion of both.
 *
 *
 * the input file must terminate with -1 (which is not counting as integer to sort)
 *
 * to compile must include writeLog.o
 *
 * */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "writeLog.h"

/*
 * argv[1] : input file
 * argv[2] : executable of first son
 * argv[3] : executable of second son
 */
int main(int argc, char **argv)
{
	//create if not exist with permission -rw-r--r--
	int fdLog = open("logfileB", O_WRONLY|O_APPEND|O_CREAT, 0644);
	if (fdLog < 0){
		perror("open error");
		return -1;
	}
	write(fdLog, "[START]\n", 8);

	if (argc < 4){ //first argo is name of executable, second the input file
		printf("too few args use is: inputFile, first son, second son\n");
		writeLog(fdLog, getpid(), "too few args use is: inputFile, first son, second son\n");
		write(fdLog, "[END]\n\n", 7);
		close(fdLog);
		return -1;
	}
	
	//verify valid input file
	int fdInput = open(argv[1], O_WRONLY);
	if(fdInput < 0){
		perror(argv[1]);
		writeLog(fdLog, getpid(), "Input file not valid\n");
		write(fdLog, "[END]\n\n", 7);
		close(fdLog);
		return -1;
	}
	close(fdInput);

	int child1_status;
	int child2_status;
	int pipesFd[2];
	int pipeStatus;

	pipeStatus = pipe(pipesFd);
	if (pipeStatus < 0){
		perror("pipe error");
		writeLog(fdLog, getpid(),"error creating pipe");
		write(fdLog, "[END]\n\n", 7);
		close(fdLog);
		return -1;
	}

	char pipeFdcharFirst[15];
	char pipeFdcharSecond[15];
	char logFdchar[15];
	sprintf(pipeFdcharFirst, "%d", pipesFd[1]);
	sprintf(pipeFdcharSecond, "%d", pipesFd[0]);
	sprintf(logFdchar, "%d", fdLog);
	char *args_1[] = {"first", argv[1], pipeFdcharFirst, pipeFdcharSecond, logFdchar, (char *) 0};
	char *args_2[] = {"second", argv[1], pipeFdcharFirst, pipeFdcharSecond, logFdchar, (char *) 0};

	pid_t pid = fork();

	if (pid < 0) {
		writeLog(fdLog, getpid(),"error forking first\n");
		perror("Fork error");
		write(fdLog, "[END]\n\n", 7);
		close(pipesFd[0]);
		close(pipesFd[1]);
		close(fdLog);
		return -1;
	}

	if(pid == 0){ //son first
		int res = execve(argv[2], args_1, NULL);
		if(res < 0) {
			writeLog(fdLog, getpid(),"error execve first\n");
			perror("error execve first");
			close(pipesFd[0]);
			close(pipesFd[1]);
			close(fdLog);
			return -1;
		}
	}

	//father that has to fork again for second son
	pid = fork();

	if (pid < 0) {
		writeLog(fdLog, getpid(),"error forking second\n");
		perror("Fork error");
		write(fdLog, "[END]\n\n", 7);
		close(pipesFd[0]);
		close(pipesFd[1]);
		close(fdLog);
		return -1;
	}

	if(pid == 0){ //son second
		int res = execve(argv[3], args_2, NULL);
		if(res < 0) {
			writeLog(fdLog, getpid(),"error execve second\n");
			perror("error execve second");
			close(pipesFd[0]);
			close(pipesFd[1]);
			close(fdLog);
			return -1;
		}
	}

	// father don't use the pipe
	close(pipesFd[0]);
	close(pipesFd[1]);
	writeLog(fdLog, getpid(),"father waiting for sons\n");
	printf("Father: wait for sons\n");
	wait(&child1_status);
	wait(&child2_status);
	printf("Father: son1 terminated with status %d\n\tson2 terminated with status %d\n", child1_status, child2_status);
	char msg[160];
	if (sprintf (msg, "Father: son1 terminated with status %d\n\tson2 terminated with status %d\n", child1_status, child2_status) <0){
		perror("main");
		close(fdLog);
		return -1;
	}

	writeLog(fdLog, getpid(), msg);
	write(fdLog, "[END]\n\n", 7);
	close(fdLog);
	return 0;
}

