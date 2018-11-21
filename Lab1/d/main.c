/*
 *
 * D
 * As before, but cyclically: after the print out, the first starts again
 * (it is a simple client-server model: the first acts as user interface, the second as calculator).
 * To end the loop, enter a special command (or a special input) that make
 * the first process to terminate the second process (through signal) and to terminate itself.
 * H.: use SIGUSRx signals.
 *
 * father creates sons (fork+execve) then wait for them to terminate
 * when user sends SIGINT (ctrl+c) father sends a SIGUSR1 to first and second son
 * to terminate both, then also father terminates (after waited terminations of sons).
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "writeLog.h"

pid_t pid1;
pid_t pid2;
int fdLog;

void sig_handler(int signumber){

	if (signumber == SIGINT){ //ctrl+c
		writeLog(fdLog, getpid(), "Father: received signal by user, terminating sons\n");
		printf("Father: received signal by user, terminating sons\n");
		if( kill(pid1, SIGUSR1) < 0 ){
			writeLog(fdLog, getpid(), "Father: kill to first error\n");
			perror("Father: kill to first error");
			close(fdLog);
			exit(-1);
		}
		if( kill(pid2, SIGUSR1) < 0 ){
			writeLog(fdLog, getpid(), "Father: kill to second error\n");
			perror("Father: kill to second error");
			close(fdLog);
			exit(-1);
		}
	}
}


/*
 * argv[1] : executable of first son
 * argv[2] : executable of second son
 */
int main(int argc, char **argv)
{
	//create if not exist with permission -rw-r--r--
	fdLog = open("logfileD", O_WRONLY|O_APPEND|O_CREAT, 0644);
	if (fdLog < 0){
		perror("open error");
		return -1;
	}
	write(fdLog, "[START]\n", 8);
	
	if (argc < 3){ //first argo is name of executable
		printf("too few args use is: first son, second son\n");
		writeLog(fdLog, getpid(), "too few args use is: first son, second son\n");
		write(fdLog, "[END]\n\n", 7);
		close(fdLog);
		return -1;
	}


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
		//writeLog(fdLog, getpid(),"error creating pipe2");
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
	char *args_1[] = {"first", charPipe1Read, charPipe1Write,
		charPipe2Read, charPipe2Write, logFdchar, (char *) 0};// args want this last element
	char *args_2[] = {"second", charPipe1Read, charPipe1Write,
		charPipe2Read, charPipe2Write, logFdchar, (char *) 0};

	pid1 = fork();

	if (pid1 < 0) {
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

	if(pid1 == 0){ //son first
		int res = execve(argv[1], args_1, NULL);
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
	pid2 = fork();

	if (pid2 < 0) {
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

	if(pid2 == 0){ //son second
		int res = execve(argv[2], args_2, NULL);
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
	writeLog(fdLog, getpid(),"father waiting ctrl+c\n");
	printf("Father: waiting ctrl+c\n");

	if (signal (SIGINT, sig_handler) == SIG_ERR){
		writeLog(fdLog, getpid(), "Father: SignalINT error\n");
		perror("Father: SignalINT error");
		close(fdLog);
		return -1;
	}

	wait(&child1_status);
	wait(&child2_status);
	printf("Father: son1 terminated with status %d\n\tson2 terminated with status %d\n", child1_status, child2_status);
	char msg[160];
	if (sprintf (msg, "Father: son1 terminated with status %d\n\tson2 terminated with status %d\n", child1_status, child2_status) <0){
		perror("father error sprintf");
		close(fdLog);
		return -1;
	}

	writeLog(fdLog, getpid(), msg);
	write(fdLog, "[END]\n\n", 7);
	close(fdLog);
	return 0;
}
