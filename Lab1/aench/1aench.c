/*
 * A enhanced
 * The second process send the first a signal that asks it to terminate.
 * H.: who is the parent (the first or the second) in your scheme?
 * the signal is sent autonomously or upon reqest by the user? or after a while? use SIGUSRx signals.
 *
 * son asks father to terminate
 */


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

int fd;

int writeLog(int fd, pid_t pid, const char* msg){

	char msgLog [256];

	if (sprintf (msgLog, "[%d] PID:%d %s", (int)time(NULL), pid, msg) <0){
		perror("writeLog");
		return -1;
	}

	if (write(fd, msgLog, strlen(msgLog)) < 0 ){
		perror ("writeLog");
		return -1;
	}

	return 0;
}

void sig_handler(int signumber){

	if (signumber == SIGUSR1){
		writeLog(fd, getpid(), "father: received signal , terminating \n");
		printf("pid:%d father: received signal , terminating \n", getpid());
		close(fd);
		exit(0);
	}

	if (signumber == SIGINT){
		//nothing, it is the son who handle this
	}
}

void sig_handlerchild(int signumber){

	if (signumber == SIGINT){

		writeLog(fd, getpid(), "son: received crtl+c, sending signal to father \n");
		printf("pid:%d son: received crtl+c, sending signal to father \n", getpid());
		if( kill(getppid(), SIGUSR1) < 0 ){
			writeLog(fd, getpid(), "kill error/n");
			perror("son kill to father error");
			close(fd);
			exit(-1);
		}

		writeLog(fd, getpid(), "son: signal sended\n");
		printf("pid:%d son: signal sended\n", getpid());
		close(fd);
		exit(0);

	}
}


int main(int argc, char **argv)
{
	pid_t pid;

	//create if not exist with permission -rw-r--r--
	fd = open("logfileAench", O_WRONLY|O_APPEND|O_CREAT, 0644);
	if (fd < 0){
		perror("open error");
		return -1;
	}
	write(fd, "[START]\n", 8);

	pid = fork();
	if (pid < 0) {
		writeLog(fd, getpid(), "Fork error\n");
		perror("Fork error");
		close(fd);
		return -1;
	}

	if(pid == 0){ //son
		writeLog(fd, getpid(), "I am son\n");
		printf("pid:%d I am son\n", getpid());
		if (signal (SIGINT, sig_handlerchild) == SIG_ERR){
			writeLog(fd, getpid(), "Signal error son\n");
			perror("Signal error son");
			close(fd);
			return -1;
		}

		while (1) {
			writeLog(fd, getpid(), "son: waiting for signal by user\n");
			printf("pid:%d son: waiting for signal by user\n", getpid());
			sleep(1);
		}


	} else { //father

		writeLog(fd, getpid(), "I am father\n");
		printf("pid:%d I am father\n", getpid());
		if (signal (SIGINT, sig_handler) == SIG_ERR){
			writeLog(fd, getpid(), "SignalINT error father\n");
			perror("SignalINT error father");
			close(fd);
			return -1;
		}
		if (signal (SIGUSR1, sig_handler) == SIG_ERR){
			writeLog(fd, getpid(), "SignalUSR1 error father\n");
			perror("SignalUSR1 error father");
			close(fd);
			return -1;
		}

		while (1) {
			writeLog(fd, getpid(), "father waiting for signal by son\n");
			printf("pid:%d father:waiting for signal by son %d \n", getpid(), pid);
			sleep(1);
		}
	}

	//should never be here
	return 0;
}
