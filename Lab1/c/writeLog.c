#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
