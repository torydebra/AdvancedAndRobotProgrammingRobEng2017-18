/*
 * A
 * Create two processes.
 * The first waits until the second dies and returns an exit status from it.
 * Hints: why terminates? when? can it terminate with different exit status?
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

int writeLog(int fd, pid_t pid, const char* msg){

	char msgLog [256];

	if (sprintf (msgLog, "[%d] PID:%d %s", (int)time(NULL), pid, msg) <0){
		perror("writeLog: ");
		return -1;
	}

	if (write(fd, msgLog, strlen(msgLog)) < 0 ){
		perror ("writeLog: ");
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	pid_t pid;
	int child_status;
	int fd;

	//create if not exist with permission -rw-r--r--
	fd = open("logfileA", O_WRONLY|O_APPEND|O_CREAT, 0644);
	if (fd < 0){
		perror("open error: ");
		return -1;
	}
	write(fd, "[START]\n", 8);

	pid = fork();
	if (pid < 0) {
		writeLog(fd, getpid(), "Fork error");
		perror("Fork error: ");
		close(fd);
		return -1;
	}

	if(pid == 0){ //son
		writeLog(fd, getpid(), "I am son\n");
		printf("pid:%d I am son\n", getpid());
		sleep(2);
		close(fd);
		return 0;

	} else { //father
		char msg[80];

		if (sprintf (msg, "father waiting for son %d \n", pid) <0){
			perror("main: ");
			close(fd);
			return -1;
		}
		writeLog(fd, getpid(), msg);
		printf("pid:%d %s", getpid(), msg);
		pid_t son = wait(&child_status);

		strcpy(msg, "");
		if (sprintf (msg, "father: terminated child %d with status: %d \n", son, child_status) <0){
			perror("main: ");
			close(fd);
			return -1;
		}
		writeLog(fd, getpid(), msg);
		printf("pid:%d %s", getpid(), msg);
	}

	write(fd, "[END]\n\n", 7);
	close (fd);
	return 0;
}

