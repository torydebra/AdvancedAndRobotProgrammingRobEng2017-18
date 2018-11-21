/*
 * mediator.c
 *
 * Read strings lower char from Pub1 and upperchar from Pub2
 * Read request from sub1 sub2 and sub3
 *
 * To read it uses select() to check if any data is avaiable from the read pipes
 * It keep track of request (with array of bool subReady[3]) from sub and
 * at the end of loop it send data if avaiable.
 * So, if data is not avaiable, the subReady[] remain 1 and data will be sent in
 * next loop (if it will be avaiable), and the sub remains in waiting on the read()
 * without the need to send another request.
 *
 * Data sent by publishers is stored in two different queues: messQueue1 and messQueue2,
 * both array of struct ElementQueue.
 * ElementQueue contains the message and three flags to know if the message is been sent
 * to subscribers
 * Queue is handled with pointers head, tail and size
 *
 * If queue is full, new message overwrites the oldest message (printing a info message)
 *
 * When a request from any sub arrives, it send a element of messQueue1
 * and/or a element of messQueue2 ("and" for only sub2 which is subscribed to both pub),
 * if sub has received all data, mediator sends nothing and print a info message, while the sub
 * will remain in waiting for some data from some pub it has subscribed to
 *
 * When a message is read by all the proper sub (ElementQueue.subRead all 1),
 * it will be deleted from the queue
 * (that is, head pointer incremented by one and message will be really
 * ovewritten when tail pointer will be on head position and a new element arrives)
 *
 * It ends execution when main send SIGUSR1
 *
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "writeLog.h"
#include <signal.h>
#define MAXDIMQUEUE 30

int fdLog;
int pipeFromPub1Read;
int pipeFromPub2Read;
int pipeFromSub1Read;
int pipeFromSub2Read;
int pipeFromSub3Read;
int pipeToSub1Write;
int pipeToSub2Write;
int pipeToSub3Write;

struct ElementQueue {

	char mess[50];
	int sub1Read;
	int sub2Read;
	int sub3Read;
};

//close pipes and also the log file
void closeAllPipes(){
	close(pipeFromPub1Read);
	close(pipeFromPub2Read);
	close(pipeFromSub1Read);
	close(pipeFromSub2Read);
	close(pipeFromSub3Read);
	close(pipeToSub1Write);
	close(pipeToSub2Write);
	close(pipeToSub3Write);
	close(fdLog);
}

void sig_handler(int signumber){

	if (signumber == SIGUSR1){
		printf("%s", "MEDIATOR: received signal by main, terminating\n");
		writeLog(fdLog, getpid(), "MEDIATOR: received signal by main, terminating\n");
		closeAllPipes();
		exit(0);

	} else if (signumber == SIGINT){
		//nothing, main handle this
	}
}

/* int sendMessageToSub (int subId, struct ElementQueue *elQueue, int *head, int *tail, int *size, int *elToRead, int pipe)
 *
 * subId: 1 2 3 sub
 * *elQueue: element of queue to send
 * *head: head of queue
 * *tail: tail of queue
 * *size: size of queue
 * *elToRead: position of next element to read in elId queue for sub subId
 * pipe: pipe to write to the sub subId
 *
 * return -1 if nothing to send for that sub else return 0
 * */
int sendMessageToSub (int subId, struct ElementQueue *elQueue, int *head, int *tail, int *size, int *elToRead, int pipe){

	if ((*elToRead) != -1){

		char msgLog[50] = "";

		int len = strlen((*elQueue).mess) +1;
		char toSend[len];
		sprintf(toSend, "%s%s", (*elQueue).mess, " "); //add space at the end

		if ( (write(pipe, &(toSend), strlen(toSend))) < 0 ){
			perror("MEDIATOR: error writing in pipe");
			writeLog(fdLog, getpid(), "MEDIATOR: error writing in pipe\n");
		}

		sprintf(msgLog, "MEDIATOR: write %s for sub %d\n", toSend, subId);
		printf("%s", msgLog);
		writeLog(fdLog, getpid(), msgLog);

		switch (subId){
			case 1 :
			(*elQueue).sub1Read = 1;
			break;

			case 2:
			(*elQueue).sub2Read = 1;
			break;

			case 3:
			(*elQueue).sub3Read = 1;
			break;
		}

		if ((*elQueue).sub1Read == 1 && (*elQueue).sub2Read == 1 && (*elQueue).sub3Read == 1){

			sprintf(msgLog, "MEDIATOR: all sub have read %s, deleting it\n", (*elQueue).mess);
			printf("%s", msgLog);
			writeLog(fdLog, getpid(), msgLog);
			(*head) = ((*head)+1)%MAXDIMQUEUE;
			(*size)--;
		}

		(*elToRead) = ((*elToRead)+1)%MAXDIMQUEUE;

		if (*elToRead >= *tail){
			//sub has no more elements to read for now
			(*elToRead) = -1;
		}
		return 0;

	} else {
		return -1;
	}
}

/* int putElementInQueue (char* message, struct ElementQueue *elQueue, int *tail, int *size, int *elToRead)
 *
 * *message: string to put in queue
 * *elQueue: element of queue to modify
 * *tail: tail of queue
 * *size: size of queue
 * *elToRead: position of next element to read in elId queue for sub subId
*/
int putElementInQueue (char* message, struct ElementQueue *elQueue, int *tail, int *size, int *elToRead){

	char msgLog[50] = "";

	sprintf(msgLog, "MEDIATOR: putting in queue: %s\n", message);
	printf("%s", msgLog);
	writeLog(fdLog, getpid(), msgLog);

	if (*size == MAXDIMQUEUE){

		printf("MEDIATOR: queue full, I am overwriting oldest data\n");
		writeLog(fdLog, getpid(), "MEDIATOR: queue full, I am overwriting oldest data\n");

	} else {
		(*size)++;
	}

	strcpy((*elQueue).mess, message);
	(*elQueue).sub1Read = 0;
	(*elQueue).sub2Read = 0;
	(*elQueue).sub3Read = 0;

	for (int j=0; j<3; j++){
		//if there was not element to read for subj
		//now element inserted is to read
		if (elToRead[j] == -1){
			elToRead[j] = *tail;
		}
	}
	*tail = ((*tail)+1)%MAXDIMQUEUE;
	return 1;
}


int main(int argc, char **argv)
{
	if (argc < 10){
		printf("MEDIATOR: %d is too few arg\n", argc);
		closeAllPipes();
		return(-1);
	}

	pipeFromPub1Read = atoi(argv[1]);
	pipeFromPub2Read = atoi(argv[2]);
	pipeFromSub1Read = atoi(argv[3]);
	pipeFromSub2Read = atoi(argv[4]);
	pipeFromSub3Read = atoi(argv[5]);
	pipeToSub1Write = atoi(argv[6]);
	pipeToSub2Write = atoi(argv[7]);
	pipeToSub3Write = atoi(argv[8]);
	fdLog = atoi(argv[9]);

	fd_set pipesSet;
	struct timeval tv;

	int subReady[3];
	for (int k=0; k<3; k++){
		subReady[k] = 0;
	}

	struct ElementQueue messQueue1[MAXDIMQUEUE];
	struct ElementQueue messQueue2[MAXDIMQUEUE];

	//queues things
	int head1 = 0;
	int tail1 = 0;
	int size1 = 0;
	int element1ToRead[3];
	int head2 = 0;
	int tail2 = 0;
	int size2 = 0;
	int element2ToRead[3];

	char msgLog[50] = "";

	//next element to read for j sub
	for (int i =0; i<3; i++){
		element1ToRead[i] = -1;
		element2ToRead[i] = -1;
	}

	//max fd among pipesFd, needed for select()
	int arr[8] = {pipeFromPub1Read, pipeFromPub2Read,
		pipeFromSub1Read, pipeFromSub2Read, pipeFromSub3Read,
		pipeToSub1Write, pipeToSub2Write, pipeToSub3Write};
	int maxFd = -1;
	for(int z=0; z<8; z++){
		if (maxFd < arr[z]){
			maxFd = arr[z];
		}
	}

	if (signal (SIGUSR1, sig_handler) == SIG_ERR){
		writeLog(fdLog, getpid(), "MEDIATOR: setting SignalUSR1 error\n");
		perror("MEDIATOR: setting SignalUSR1 error");
		closeAllPipes();
		return -1;
	}
	if (signal (SIGINT, sig_handler) == SIG_ERR){
		writeLog(fdLog, getpid(), "MEDIATOR: setting SignalUSR1 error\n");
		perror("MEDIATOR: setting SignalINT error");
		closeAllPipes();
		return -1;
	}

	while(1){

		//to reset every cycle because select() modify it
		FD_ZERO(&pipesSet);
		FD_SET(pipeFromPub1Read, &pipesSet);
		FD_SET(pipeFromPub2Read, &pipesSet);
		FD_SET(pipeFromSub1Read, &pipesSet);
		FD_SET(pipeFromSub2Read, &pipesSet);
		FD_SET(pipeFromSub3Read, &pipesSet);
		tv.tv_sec = 10;
		tv.tv_usec = 0;

		int retval = select(maxFd+1, &pipesSet, NULL, NULL, &tv);

		if (retval < 0){
			perror("MEDIATOR: error select");
			writeLog(fdLog, getpid(), "MEDIATOR: error select\n");
			closeAllPipes();
			return -1;
		}

		//signal that timeout passed but then continues execution
		if (retval == 0){ //timeout
			printf("MEDIATOR: timeout (10 sec)\n");
			writeLog(fdLog, getpid(), "MEDIATOR: timeout (10 sec)\n");
		}

		if (FD_ISSET(pipeFromPub1Read, &pipesSet)){

			char stringBuffer[50] = "";
			int count = read(pipeFromPub1Read, &stringBuffer, sizeof(stringBuffer));

			if(count < 0){
				perror("MEDIATOR: error read from pub1");
				writeLog(fdLog, getpid(), "MEDIATOR: error read from pub1\n");
				break; //other process may be still alive, no need to exit mediator
			}

			if (count > 0){
				sprintf(msgLog, "MEDIATOR: read from pub1 %s\n", stringBuffer);
				printf("%s", msgLog);
				writeLog(fdLog, getpid(), msgLog);
			}

			char *message;
			char *str = strdup(stringBuffer); //strsep want char* and not char[50]
			while ( (message = strsep(&str, " ")) ){

				if (strlen(message) == 0){
					break;
				}
				putElementInQueue(message, &(messQueue1[tail1]), &tail1, &size1, element1ToRead);
				messQueue1[tail1-1].sub3Read = 1; //sub3 never read from pub1
			}
		}

		if (FD_ISSET(pipeFromPub2Read, &pipesSet)){

			char stringBuffer[50] = "";
			int count = read(pipeFromPub2Read, &stringBuffer, sizeof(stringBuffer));

			if(count < 0){
				perror("MEDIATOR: error read from pub2");
				writeLog(fdLog, getpid(), "MEDIATOR: error read from pub2\n");
				break; //other process may be still alive, no need to exit mediator
			}

			if (count > 0){
				sprintf(msgLog, "MEDIATOR: read from pub2 %s\n", stringBuffer);
				printf("%s", msgLog);
				writeLog(fdLog, getpid(), msgLog);
			}

			char *message;
			char *str = strdup(stringBuffer); //strsep want char* and not char[50]
			while ( (message = strsep(&str, " ")) ){

				if (strlen(message) == 0){
					break;
				}
				putElementInQueue(message, &(messQueue2[tail2]), &tail2, &size2, element2ToRead);
				messQueue2[tail2-1].sub1Read = 1; //sub1 never read from pub2
			}
		}

		if (FD_ISSET(pipeFromSub1Read, &pipesSet)){

			char req[5] = "";
			int count = read(pipeFromSub1Read, &req, sizeof(req));

			if (count < 0){
				perror("MEDIATOR: error read from sub1");
				writeLog(fdLog, getpid(), "MEDIATOR: error read from sub1\n");
				break;
			}

			if (count > 0){
				printf("MEDIATOR: read request from sub1\n");
				writeLog(fdLog, getpid(), "MEDIATOR: read request from sub1\n");
				subReady[0] = 1;
			}
		}

		if (FD_ISSET(pipeFromSub2Read, &pipesSet)){

			char req[5] = "";
			int count = read(pipeFromSub2Read, &req, sizeof(req));

			if (count < 0){
				perror("MEDIATOR: error read from sub2");
				writeLog(fdLog, getpid(), "MEDIATOR: error read from sub2\n");
				break;
			}

			if (count > 0){
				printf("MEDIATOR: read request from sub2\n");
				writeLog(fdLog, getpid(), "MEDIATOR: read request from sub2\n");
				subReady[1] = 1;
			}
		}

		if (FD_ISSET(pipeFromSub3Read, &pipesSet)){

			char req[5] = "";
			int count = read(pipeFromSub3Read, &req, sizeof(req));

			if (count < 0){
				perror("MEDIATOR: error read from sub3");
				writeLog(fdLog, getpid(), "MEDIATOR: error read from sub3\n");
				break;
			}

			if (count > 0){
				printf("MEDIATOR: read request from sub3\n");
				writeLog(fdLog, getpid(), "MEDIATOR: read request from sub3\n");
				subReady[2] = 1;

			}
		}

		if (subReady[0] == 1) {
			if (sendMessageToSub(1, &(messQueue1[element1ToRead[0]]), &head1, &tail1, &size1,
				&(element1ToRead[0]), pipeToSub1Write) == -1){
				printf("MEDIATOR: nothing in buff1 to send to sub1\n");
				writeLog(fdLog, getpid(), "MEDIATOR: nothing in buff1 to send to sub1\n");
			} else {
				subReady[0] = 0;
			}
		}

		if (subReady[1] == 1) {

			if (sendMessageToSub(2, &(messQueue1[element1ToRead[1]]), &head1, &tail1, &size1,
				&(element1ToRead[1]), pipeToSub2Write) == -1){
				printf("MEDIATOR: nothing in buff1 to send to sub2\n");
				writeLog(fdLog, getpid(), "MEDIATOR: nothing in buff1 to send to sub2\n");
			} else {
				subReady[1] = 0;
			}

			if (sendMessageToSub(2, &(messQueue2[element2ToRead[1]]), &head2, &tail2, &size2,
				&(element2ToRead[1]), pipeToSub2Write) == -1){
				printf("MEDIATOR: nothing in buff2 to send to sub2\n");
				writeLog(fdLog, getpid(), "MEDIATOR: nothing in buff2 to send to sub2\n");
			} else {
				subReady[1] = 0;
			}
		}

		if (subReady[2] == 1){

			if (sendMessageToSub(3, &(messQueue2[element2ToRead[2]]), &head2, &tail2, &size2,
				&(element2ToRead[2]), pipeToSub3Write) == -1){
				printf("MEDIATOR: nothing in buff2 to send to sub3\n");
				writeLog(fdLog, getpid(), "MEDIATOR: nothing in buff1 to send to sub2\n");
			} else {
				subReady[2] = 0;
			}
		}
	}

	//only here if while is not set not infinite
	closeAllPipes();
	return 0;
}
