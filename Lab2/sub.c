/*
 * sub.c
 *
 * sub1 subscribed to pub1
 * sub2 subscribed to pub1 and pub2
 * sub3 subscribed to pub2
 *
 * sub sent request to mediator to ask for data.
 * Then read all data sent through pipe. At maximum it consists
 * of only one string for each pub the sub is subscribed to 
 * (only sub2 can read two string at time because it is the only one subscribed to both pub)
 * If no data, it waits on read() function
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

//frequencies of subscibers
#define SECSLEEP1 1
#define SECSLEEP2 5
#define SECSLEEP3 3

int subId;
int fdLog;
int pipeToMediatorWrite;
int pipeFromMediatorRead;

void sig_handler(int signumber){

	if (signumber == SIGUSR1){
		char msgLog[50];
		sprintf (msgLog, "Sub%d: received signal by main, terminating\n", subId);
		printf("%s", msgLog);
		writeLog(fdLog, getpid(), msgLog);
		close (pipeFromMediatorRead);
		close (pipeToMediatorWrite);
		close(fdLog);
		exit(0);

	} else if (signumber == SIGINT){
		//nothing, main handle this
	}
}

/*
 * argc = 5
 * 		name of son ("sub")
 * 		number of sub
 * 		pipeToMediator
 * 		pipeFromMediator
 * 		fd for log file
 * */
int main(int argc, char **argv)
{
	if (argc < 5){
		printf("sub: %d is too few arg\n", argc);
		return(-1);
	}

	subId = atoi(argv[1]); //to differentiate subs
	pipeToMediatorWrite = atoi(argv[2]);
	pipeFromMediatorRead = atoi(argv[3]);
	fdLog = atoi(argv[4]);

	char request = 'a'; //to mediator to request data
	char msgLog[50]; //for log message

	int secSleep = 0;
	switch (subId) {

		case 1 :
			printf("Sub1: I am sub1\n");
			writeLog(fdLog, getpid(), "Sub1: I am sub1\n");
			secSleep = SECSLEEP1;
			break;
		case 2 :
			printf("Sub2: I am sub2\n");
			writeLog(fdLog, getpid(), "Sub2: I am sub2\n");
			secSleep = SECSLEEP2;
			break;
		case 3 :
			printf("Sub3: I am sub3\n");
			writeLog(fdLog, getpid(), "Sub3: I am sub3\n");
			secSleep = SECSLEEP3;
			break;
	}

	if (signal (SIGUSR1, sig_handler) == SIG_ERR){
		sprintf(msgLog, "Sub%d: setting SignalUSR1 error", subId);
		writeLog(fdLog, getpid(), msgLog);
		writeLog(fdLog, getpid(), "\n");
		perror(msgLog);
		close (pipeFromMediatorRead);
		close (pipeToMediatorWrite);
		close(fdLog);
		return -1;
	}
	if (signal (SIGINT, sig_handler) == SIG_ERR){
		sprintf(msgLog, "Sub%d: setting SignalINT error", subId);
		writeLog(fdLog, getpid(), msgLog);
		writeLog(fdLog, getpid(), "\n");
		perror(msgLog);
		close (pipeFromMediatorRead);
		close (pipeToMediatorWrite);
		close(fdLog);
		return -1;
	}

	while (1){

		//sub ready, send request to mediator
		int countWrite = write(pipeToMediatorWrite, &(request), sizeof(char));
		if (countWrite < 0){
			sprintf (msgLog, "Sub%d error sendind request to mediator", subId);
			perror(msgLog);
			writeLog(fdLog, getpid(), strcat(msgLog, "\n"));
			continue; //to next loop to try again the write()
		}

		sprintf(msgLog, "Sub%d: sent request\n", subId);
		printf("%s", msgLog);
		writeLog(fdLog, getpid(), msgLog);

		char buff[50] = "";
		// if pipe empty wait mediator to put some data in it
		int countRead = read(pipeFromMediatorRead, &buff, sizeof(buff));
		if(countRead < 0){
			sprintf(msgLog,"Sub%d error read", subId);
			perror(msgLog);
			writeLog(fdLog, getpid(), strcat(msgLog, "\n"));
		}

		//mediator can send a string for every pub the sub is subscribed, so
		//we need to separate different strings
		if (countRead > 0){
			char *message;
			char *str = strdup(buff); //strsep want char* and not char[50]
			
			//to divide strings if they are more than one (they are divided by space)
			while ( (message = strsep(&str, " ")) ){

				if (strlen(message) == 0){
					break;
				}
				sprintf(msgLog, "Sub%d: read %s\n", subId, message);
				printf("%s", msgLog);
				writeLog(fdLog, getpid(), msgLog);
			}
		}

		strcpy(msgLog,"");
		sleep (secSleep);
	}

	//only here if while is not set not infinite
	close (pipeFromMediatorRead);
	close (pipeToMediatorWrite);
	close(fdLog);
	return 0;
}
