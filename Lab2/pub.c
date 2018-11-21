/*
 * pub.c
 *
 * publisher send strings through pipe to mediator
 * strings are numered (1 2 3 4...) to better understand the order they are sent/received. 
 * After the last string is sent, pub starts again to send from the first string
 *
 * pub1 send lower case, pub2 send upper case
 *
 * It ends execution when main send SIGUSR1
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "writeLog.h"

//frequencies of publishers
#define SECSLEEP1 3
#define SECSLEEP2 1

int pubId;
int fdLog;
int pipeToMediatorWrite;
char **mess;

void sig_handler(int signumber){

	if (signumber == SIGUSR1){
		char msgLog[50];
		sprintf (msgLog, "Pub%d: received signal by main, terminating\n", pubId);
		printf("%s", msgLog);
		writeLog(fdLog, getpid(), msgLog);
		free(mess);
		close(pipeToMediatorWrite);
		close(fdLog);
		exit(0);

	} else if (signumber == SIGINT){
		//nothing, main handle this
	}
}

/*
 * argc = 4
 * 		name of son ("pub")
 * 		number of pub
 * 		pipeToMediator
 * 		fd for log file
 * */
int main(int argc, char **argv)
{
	if (argc < 4){
		printf("Pub: %d is too few arg\n", argc);
		close(pipeToMediatorWrite);
		close(fdLog);
		return(-1);
	}

	pubId = atoi(argv[1]); //to differentiate pub1 from pub2
	pipeToMediatorWrite = atoi(argv[2]);
	fdLog = atoi(argv[3]);

	int secSleep = 0;
	int nMess;

	if (pubId == 1){ //pub1 lower case
		printf("Pub1: I am pub1\n");
		writeLog(fdLog, getpid(), "Pub1: I am pub1\n");
		mess = (char**)malloc(5*sizeof(char*));
		for(int i = 0; i < 5; i++){
			mess[i] = (char*)malloc(50*sizeof(char));
		}
		sprintf(mess[0], "1pippo1");
		sprintf(mess[1], "2pluto2");
		sprintf(mess[2], "3giovanno3");
		sprintf(mess[3], "4ugo4");
		sprintf(mess[4], "5sigismondo5");
		nMess = 5;

		secSleep=SECSLEEP1;

	} else { //pub2 upper case
		printf("Pub2: I am pub2\n");
		writeLog(fdLog, getpid(), "Pub2: I am pub2\n");
		mess = (char**)malloc(7*sizeof(char*));
		for(int i = 0; i < 7; i++){
			mess[i] = (char*)malloc(50*sizeof(char));
		}

		sprintf(mess[0], "1ASDASD1");
		sprintf(mess[1], "2TRUTO2");
		sprintf(mess[2], "3BOH3");
		sprintf(mess[3], "4MAH4");
		sprintf(mess[4], "5BEH5");
		sprintf(mess[5], "6CHHISSA6");
		sprintf(mess[6], "7MARGIONI7");
		nMess = 7;

		secSleep = SECSLEEP2;
	}

	int i = 0;
	char msgLog[50];

	if (signal (SIGUSR1, sig_handler) == SIG_ERR){
		sprintf(msgLog, "Pub%d: setting SignalUSR1 error", pubId);
		writeLog(fdLog, getpid(), msgLog);
		writeLog(fdLog, getpid(), "\n");
		perror(msgLog);
		free(mess);
		close(pipeToMediatorWrite);
		close(fdLog);
		return -1;
	}
	// also SIGINT to do nothing when user press ctrl+c (main handle this)
	if (signal (SIGINT, sig_handler) == SIG_ERR){
		sprintf(msgLog, "Pub%d: setting SignalINT error", pubId);
		writeLog(fdLog, getpid(), msgLog);
		writeLog(fdLog, getpid(), "\n");
		perror(msgLog);
		free(mess);
		close(pipeToMediatorWrite);
		close(fdLog);
		return -1;
	}

	//pubs for ever
	while(1){

		int len = strlen(mess[i])+1;
		char toSend[len];
		//add space at the end of string to differentiare them in pipe
		sprintf(toSend, "%s%s", mess[i], " ");
		int count = write(pipeToMediatorWrite, &(toSend[0]), strlen(toSend));

		if (count < 0){
			sprintf (msgLog, "Pub%d: error write %s", pubId, toSend);
			perror(msgLog);
			writeLog(fdLog, getpid(), strcat(msgLog, "\n"));
			break;

		} else {
			sprintf (msgLog, "Pub%d: written '%s' in pipe\n", pubId, &(mess[i][0]));
			printf("%s", msgLog);
			writeLog(fdLog, getpid(), msgLog);
		}

		i = (i+1)%nMess;
		strcpy(msgLog,"");
		sleep(secSleep);
	}

	//only here if while is not set not infinite
	free(mess);
	close(pipeToMediatorWrite);
	close(fdLog);
	return 0;
}

