/*
 * C
 *
 * As before, but the second sends back the results to the first one who prints them.
 * H.: do not use a bidirectional pipe. Use two pipes.
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
	int fdLog = open("logfileC", O_WRONLY|O_APPEND|O_CREAT, 0644);
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
	int pipesFdFirstToSecond[2];
	int pipesFdSecondToFirst[2];

	if (pipe(pipesFdFirstToSecond) < 0){
		perror("pipe1 error");
		writeLog(fdLog, getpid(),"error creating pipe1");
		write(fdLog, "[END]\n\n", 7);
		close(fdLog);
		return -1;
	}

	if (pipe(pipesFdSecondToFirst) < 0){
		perror("pipe2 error");
		writeLog(fdLog, getpid(),"error creating pipe2");
		write(fdLog, "[END]\n\n", 7);
		close(pipesFdFirstToSecond[0]);
		close(pipesFdFirstToSecond[1]);
		close(fdLog);
		return -1;
	}

	//pipe1: first write to second
	char charPipe1Read[15];
	char charPipe1Write[15];
	//pipe2: second write to first
	char charPipe2Read[15];
	char charPipe2Write[15];
	char logFdchar[15];
	sprintf(charPipe1Read, "%d", pipesFdFirstToSecond[0]);
	sprintf(charPipe1Write, "%d", pipesFdFirstToSecond[1]);
	sprintf(charPipe2Read, "%d", pipesFdSecondToFirst[0]);
	sprintf(charPipe2Write, "%d", pipesFdSecondToFirst[1]);
	sprintf(logFdchar, "%d", fdLog);
	char *args_1[] = {"first", argv[1], charPipe1Read, charPipe1Write,
		charPipe2Read, charPipe2Write, logFdchar, (char *) 0};// args want this last element
	char *args_2[] = {"second", argv[1], charPipe1Read, charPipe1Write,
		charPipe2Read, charPipe2Write, logFdchar, (char *) 0};

	pid_t pid = fork();

	if (pid < 0) {
		writeLog(fdLog, getpid(),"error forking first\n");
		perror("Fork error");
		write(fdLog, "[END]\n\n", 7);
		close(pipesFdFirstToSecond[0]);
		close(pipesFdFirstToSecond[1]);
		close(pipesFdSecondToFirst[0]);
		close(pipesFdSecondToFirst[1]);
		close(fdLog);
		return -1;
	}

	if(pid == 0){ //son first
		int res = execve(argv[2], args_1, NULL);
		if(res < 0) {
			writeLog(fdLog, getpid(),"error execve first\n");
			perror("error execve first");
			close(pipesFdFirstToSecond[0]);
			close(pipesFdFirstToSecond[1]);
			close(pipesFdSecondToFirst[0]);
			close(pipesFdSecondToFirst[1]);
			close(fdLog);
			return -1;
		}
	}

	//father that has to fork again for second son
	pid = fork();

	if (pid < 0) {
		writeLog(fdLog, getpid(),"error forking second\n");
		perror("Fork error: ");
		write(fdLog, "[END]\n\n", 7);
		close(pipesFdFirstToSecond[0]);
		close(pipesFdFirstToSecond[1]);
		close(pipesFdSecondToFirst[0]);
		close(pipesFdSecondToFirst[1]);
		close(fdLog);
		return -1;
	}

	if(pid == 0){ //son second
		int res = execve(argv[3], args_2, NULL);
		if(res < 0) {
			writeLog(fdLog, getpid(),"error execve second\n");
			perror("error execve second");
			close(pipesFdFirstToSecond[0]);
			close(pipesFdFirstToSecond[1]);
			close(pipesFdSecondToFirst[0]);
			close(pipesFdSecondToFirst[1]);
			close(fdLog);
			return -1;
		}
	}

	//father doesn't use the two pipes
	close(pipesFdFirstToSecond[0]);
	close(pipesFdFirstToSecond[1]);
	close(pipesFdSecondToFirst[0]);
	close(pipesFdSecondToFirst[1]);
	writeLog(fdLog, getpid(),"father waiting for sons\n");
	printf("Father: wait for sons\n");
	wait(&child1_status);
	wait(&child2_status);
	printf("Father: son1 terminated with status %d\n\tson2 terminated with status %d\n", child1_status, child2_status);
	char msg[160];
	if (sprintf (msg, "Father: son1 terminated with status %d\n\tson2 terminated with status %d\n", child1_status, child2_status) <0){
		perror("main: ");
		close(fdLog);
		return -1;
	}

	writeLog(fdLog, getpid(), msg);
	write(fdLog, "[END]\n\n", 7);
	close(fdLog);
	return 0;
}
