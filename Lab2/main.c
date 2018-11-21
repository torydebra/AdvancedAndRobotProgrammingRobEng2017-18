/*
 * main.c
 * Initialize program:
 *  - creates log file
 * 	- creates pipes
 * 	- creates 6 sons: 2 Pub, 3 Sub, 1 Mediator
 *
 * 	after forking a son, it closes pipes which that son will not use with
 * 	closeSomePipes() function, then execve() his executable
 *
 * then wait for user to end with sigINT (ctrl+c) to kill all sons with SIGUSR1
 * and terminate the program when all sons have returned
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
#include <string.h>
#include "writeLog.h"

int pipePub1ToMediator[2];
int pipePub2ToMediator[2];
int pipeSub1ToMediator[2];
int pipeSub2ToMediator[2];
int pipeSub3ToMediator[2];
int pipeMediatorToSub1[2];
int pipeMediatorToSub2[2];
int pipeMediatorToSub3[2];
int fdLog;
pid_t pidMediator;
pid_t pidPub1;
pid_t pidPub2;
pid_t pidSub1;
pid_t pidSub2;
pid_t pidSub3;

void closeAllPipes(){

	close(pipePub1ToMediator[0]);
	close(pipePub2ToMediator[0]);
	close(pipeSub1ToMediator[0]);
	close(pipeSub2ToMediator[0]);
	close(pipeSub3ToMediator[0]);
	close(pipeMediatorToSub1[0]);
	close(pipeMediatorToSub2[0]);
	close(pipeMediatorToSub3[0]);
	close(pipePub1ToMediator[1]);
	close(pipePub2ToMediator[1]);
	close(pipeSub1ToMediator[1]);
	close(pipeSub2ToMediator[1]);
	close(pipeSub3ToMediator[1]);
	close(pipeMediatorToSub1[1]);
	close(pipeMediatorToSub2[1]);
	close(pipeMediatorToSub3[1]);
	return;
}

void closeSomePipes(char* name, char* id){

	if (strcmp(name, "mediator") == 0){
		//0 read    1 write
		//sub DONT use that:
		close(pipeMediatorToSub1[0]);
		close(pipeMediatorToSub2[0]);
		close(pipeMediatorToSub3[0]);
		close(pipePub1ToMediator[1]);
		close(pipePub2ToMediator[1]);
		close(pipeSub1ToMediator[1]);
		close(pipeSub2ToMediator[1]);
		close(pipeSub3ToMediator[1]);

	} else if (strcmp(name, "pub") == 0){

		close(pipePub1ToMediator[0]);
		close(pipePub2ToMediator[0]);
		close(pipeSub1ToMediator[0]);
		close(pipeSub2ToMediator[0]);
		close(pipeSub3ToMediator[0]);
		close(pipeMediatorToSub1[0]);
		close(pipeMediatorToSub2[0]);
		close(pipeMediatorToSub3[0]);
		close(pipeSub1ToMediator[1]);
		close(pipeSub2ToMediator[1]);
		close(pipeSub3ToMediator[1]);
		close(pipeMediatorToSub1[1]);
		close(pipeMediatorToSub2[1]);
		close(pipeMediatorToSub3[1]);

		if (strcmp(id, "1") == 0){ //pub1
			close(pipePub2ToMediator[1]);

		} else { //pub2
			close(pipePub1ToMediator[1]);
		}

	} else { //sub

		close(pipePub1ToMediator[0]);
		close(pipePub2ToMediator[0]);
		close(pipeSub1ToMediator[0]);
		close(pipeSub2ToMediator[0]);
		close(pipeSub3ToMediator[0]);
		close(pipePub1ToMediator[1]);
		close(pipePub2ToMediator[1]);
		close(pipeMediatorToSub1[1]);
		close(pipeMediatorToSub2[1]);
		close(pipeMediatorToSub3[1]);

		if (strcmp(id, "1") == 0){ //sub1
			close(pipeMediatorToSub2[0]);
			close(pipeMediatorToSub3[0]);
			close(pipeSub2ToMediator[1]);
			close(pipeSub3ToMediator[1]);

		} else if (strcmp(id, "2") == 0){ //sub2
			close(pipeMediatorToSub1[0]);
			close(pipeMediatorToSub3[0]);
			close(pipeSub1ToMediator[1]);
			close(pipeSub3ToMediator[1]);

		} else { //sub3
			close(pipeMediatorToSub1[0]);
			close(pipeMediatorToSub2[0]);
			close(pipeSub1ToMediator[1]);
			close(pipeSub2ToMediator[1]);
		}
	}
}


pid_t forking(char *executable, char *name, char *args[]){

	char msg[100];
	pid_t pid = fork();

	if (pid < 0) {
		sprintf (msg, "MAIN: error forking %s", name);
		writeLog(fdLog, getpid(), msg);
		perror(msg);
		write(fdLog, "\n[END]\n\n", 7);
		closeAllPipes();
		close(fdLog);
		exit (-1);
	}

	if(pid == 0){ //son

		closeSomePipes(args[0], args[1]);
		int res = execve(executable, args, NULL);
		if(res < 0) {
			sprintf (msg, "%s: error execve %s", name, executable);
			writeLog(fdLog, getpid(), strcat(msg, "\n"));
			perror(msg);
			exit (-1);
		}
	}
	return pid;
}

void sig_handler(int signumber){

	if (signumber == SIGINT){ //ctrl+c
		writeLog(fdLog, getpid(), "MAIN: received signal by user, terminating sons\n");
		printf("MAIN: received signal by user, terminating sons\n");
		if( kill(pidPub1, SIGUSR1) < 0 || kill(pidPub2, SIGUSR1) < 0 ||
			kill(pidSub1, SIGUSR1) < 0 || kill(pidSub2, SIGUSR1) < 0 || kill(pidSub3, SIGUSR1) < 0 ||
			kill(pidMediator, SIGUSR1) < 0){
			writeLog(fdLog, getpid(), "Father: kill error\n");
			perror("Father: kill error");
			write(fdLog, "[END]\n\n", 7);
			close(fdLog);
			exit(-1);
		}
	}
}

/*
 * argc = 4 :
 * 		name of this executable
 * 		name of executable of pub
 * 		name of executable of mediator
 * 		name of executable of sub
 * */
int main(int argc, char **argv)
{
	//create if not exist with permission -rw-r--r--
	fdLog = open("logfile", O_WRONLY|O_APPEND|O_CREAT, 0644);
	if (fdLog < 0){
		perror("MAIN: open logfile error");
		return -1;
	}
	write(fdLog, "[START]\n", 8);

	if (argc < 4){
		printf("MAIN: Too few args use is: pub, mediator, sub\n");
		writeLog(fdLog, getpid(), "MAIN: Too few args use is: pub, mediator, sub\n");
		write(fdLog, "[END]\n\n", 7);
		closeAllPipes();
		close(fdLog);
		return -1;
	}

	int pub1_status;
	int pub2_status;
	int mediator_status;
	int sub1_status;
	int sub2_status;
	int sub3_status;


	if (pipe(pipePub1ToMediator) < 0 || pipe(pipePub2ToMediator) < 0 ||
	    pipe(pipeSub1ToMediator) < 0 || pipe(pipeSub2ToMediator) < 0 ||
	    pipe(pipeSub3ToMediator) < 0 || pipe(pipeMediatorToSub1) < 0 ||
	    pipe(pipeMediatorToSub2) < 0 || pipe(pipeMediatorToSub3) < 0 ){

		perror("MAIN: error creating one or more pipes");
		writeLog(fdLog, getpid(),"MAIN: error creating one or more pipes\n");
		write(fdLog, "[END]\n\n", 7);
		close(fdLog);
		closeAllPipes();
		return -1;
	}

	// to passing file descriptor of pipes to sons
	char charPipePub1ToMediatorRead[15]; //int can be store in same memory size of a char[15]
	char charPipePub2ToMediatorRead[15];
	char charPipeSub1ToMediatorRead[15];
	char charPipeSub2ToMediatorRead[15];
	char charPipeSub3ToMediatorRead[15];
	char charPipeMediatorToSub1Read[15];
	char charPipeMediatorToSub2Read[15];
	char charPipeMediatorToSub3Read[15];
	char charPipePub1ToMediatorWrite[15];
	char charPipePub2ToMediatorWrite[15];
	char charPipeSub1ToMediatorWrite[15];
	char charPipeSub2ToMediatorWrite[15];
	char charPipeSub3ToMediatorWrite[15];
	char charPipeMediatorToSub1Write[15];
	char charPipeMediatorToSub2Write[15];
	char charPipeMediatorToSub3Write[15];
	char logFdchar[15];

	sprintf(charPipePub1ToMediatorRead, "%d", pipePub1ToMediator[0]);
	sprintf(charPipePub2ToMediatorRead, "%d", pipePub2ToMediator[0]);
	sprintf(charPipeSub1ToMediatorRead, "%d", pipeSub1ToMediator[0]);
	sprintf(charPipeSub2ToMediatorRead, "%d", pipeSub2ToMediator[0]);
	sprintf(charPipeSub3ToMediatorRead, "%d", pipeSub3ToMediator[0]);
	sprintf(charPipeMediatorToSub1Read, "%d", pipeMediatorToSub1[0]);
	sprintf(charPipeMediatorToSub2Read, "%d", pipeMediatorToSub2[0]);
	sprintf(charPipeMediatorToSub3Read, "%d", pipeMediatorToSub3[0]);
	sprintf(charPipePub1ToMediatorWrite, "%d", pipePub1ToMediator[1]);
	sprintf(charPipePub2ToMediatorWrite, "%d", pipePub2ToMediator[1]);
	sprintf(charPipeSub1ToMediatorWrite, "%d", pipeSub1ToMediator[1]);
	sprintf(charPipeSub2ToMediatorWrite, "%d", pipeSub2ToMediator[1]);
	sprintf(charPipeSub3ToMediatorWrite, "%d", pipeSub3ToMediator[1]);
	sprintf(charPipeMediatorToSub1Write, "%d", pipeMediatorToSub1[1]);
	sprintf(charPipeMediatorToSub2Write, "%d", pipeMediatorToSub2[1]);
	sprintf(charPipeMediatorToSub3Write, "%d", pipeMediatorToSub3[1]);
	sprintf(logFdchar, "%d", fdLog);

	//second argument is for differentiate pub/sub 1 2 3
	char *args_pub1[] = {"pub", "1", charPipePub1ToMediatorWrite, logFdchar, (char *) 0};// args want this last element
	char *args_pub2[] = {"pub", "2", charPipePub2ToMediatorWrite, logFdchar, (char *) 0};
	char *args_sub1[] = {"sub", "1", charPipeSub1ToMediatorWrite,
		charPipeMediatorToSub1Read, logFdchar, (char *) 0};
	char *args_sub2[] = {"sub", "2", charPipeSub2ToMediatorWrite,
		charPipeMediatorToSub2Read, logFdchar, (char *) 0};
	char *args_sub3[] = {"sub", "3", charPipeSub3ToMediatorWrite,
		charPipeMediatorToSub3Read, logFdchar, (char *) 0};
	char *args_mediator[] = {"mediator", charPipePub1ToMediatorRead,
		charPipePub2ToMediatorRead, charPipeSub1ToMediatorRead,
		charPipeSub2ToMediatorRead, charPipeSub3ToMediatorRead,
		charPipeMediatorToSub1Write, charPipeMediatorToSub2Write,
		charPipeMediatorToSub3Write, logFdchar, (char *) 0};

	pidMediator = forking(argv[2], "mediator", args_mediator);
	pidPub1 = forking(argv[1], "pub1", args_pub1);
	pidPub2 = forking(argv[1], "pub2", args_pub2);
	pidSub1 = forking(argv[3], "sub1", args_sub1);
	pidSub2 = forking(argv[3], "sub2", args_sub2);
	pidSub3 = forking(argv[3], "sub3", args_sub3);

	//Main doesn't use pipes
	closeAllPipes();

	//By default sons have infinite loop so the only way
	//to terminate the program is with crtl+c
	writeLog(fdLog, getpid(),"MAIN: waiting for terminating sons or signal by user\n");
	printf("MAIN: waiting for terminating sons or signal by user\n");
	if (signal (SIGINT, sig_handler) == SIG_ERR){
		writeLog(fdLog, getpid(), "MAIN: setting SignalINT error\n");
		perror("MAIN: setting SignalINT error");
		close(fdLog);
		return -1;
	}
	waitpid(pidMediator, &mediator_status, 0);
	waitpid(pidPub1, &pub1_status, 0);
	waitpid(pidPub2, &pub2_status, 0);
	waitpid(pidSub1, &sub1_status, 0);
	waitpid(pidSub2, &sub2_status, 0);
	waitpid(pidSub3, &sub3_status, 0);

	char msg[512];
	if (sprintf (msg, ("MAIN: pub1 terminated with status %d\n"
		"\tpub2 terminated with status %d\n"
		"\tsub1 terminated with status %d\n"
		"\tsub2 terminated with status %d\n"
		"\tsub3 terminated with status %d\n"
		"\tmediator terminated with status %d\n"),
		pub1_status, pub2_status, sub1_status,
		sub2_status, sub3_status, mediator_status) <0){

		perror("MAIN: ");
		close(fdLog);
		return -1;
	}
	printf("%s", msg);
	writeLog(fdLog, getpid(), msg);
	write(fdLog, "[END]\n\n", 7);

	close(fdLog);
	return 0;
}
