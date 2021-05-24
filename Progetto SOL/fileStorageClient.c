#include<stdio.h>
#include<stdlib.h>
#include<limits.h>
#include<string.h>
#include<time.h>
#include<errno.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<sys/types.h>
#include "request.h"
#include "file.h"
#include "utility.h"
#include "clientConfig.h"
#include "generic_queue.h"
#include "serverAPI.h"

ClientConfigInfo c; // struttura che mi conterrà le informazioni di configurazione del client
GenericQueue toSendRequestQueue; //coda di richieste da dare al server

void printMainRequestInfo(int ris, MyRequest *actReq){
	printf("Stampo informazioni post esecuzione della richiesta:\n");
	printf("\tTipo operazione: %d\n", actReq->type);
	printf("\tFile di riferimento: %s\n", actReq->request_content);
	printf("\tEsito: %d\n", ris);
	if(ris < 0)
		printf("\tCodice di errore: %d\n", errno);
}

int sendRequests(){
	size_t readDataSize;
	char *readDataBuffer;
	int ris;

	while(isEmpty(toSendRequestQueue) == 0){
		MyRequest *actReq = (MyRequest*) pop(&toSendRequestQueue);
		printf("Prendo richiesta\n");
		if(actReq == NULL)
			return -1;
		
		switch(actReq->type){
			case WRITE_FILE:
				//ris = openFile(actReq->request_content, O_CREAT); 
				ris = writeFile(actReq->request_content, NULL);
				if(c.printEnable){
					printMainRequestInfo(ris, actReq);
				}
				break;
			case READ_FILE:
				ris = readFile(actReq->request_content, (void**)&readDataBuffer, &readDataSize);
				if(c.printEnable){
					printMainRequestInfo(ris, actReq);
					if(ris >= 0)
						printf("\tByte letti: %d\n\n", readDataSize);
				}
				break;
			case READ_N_FILE:
				ris = readNFiles(atoi(actReq->request_content), c.saveReadFileDir);
				if(c.printEnable){
					printMainRequestInfo(ris, actReq);
					if(ris >= 0)
						printf("\tNumero di file letti: %d\n\n", ris);
				}
				break;
			case REMOVE_FILE:
				ris = removeFile(actReq->request_content);
				if(c.printEnable){
					printMainRequestInfo(ris, actReq);
				}
				break;
			default: printf("Per mandare altri tipi di richieste, usare esplicitamente \
				la server API\n");
				break;
		}
		msleep(c.requestInterval);
	}
	return 0;
}

int main(int argc, char const *argv[]){
	char o;
	struct timespec ts; //struttura che indica la massima attesa per la connessione nella openConnection

	ts.tv_sec = 10; //10 secondi 
	ts.tv_nsec = 0; // 0 nanosecondi

	toSendRequestQueue = createQueue(&requestComparison, &requestPrint, &freeRequest);

	//se c'è -h stampo lo usage message e termino l'esecuzione
	while((o = getopt(argc, argv, ALL_OPTIONS)) != -1){
		switch(o){
			case 'h': printAvailableOptions();
					  exit(EXIT_SUCCESS);
					  break;
			default: break;
		}
	}
	int z, y, t;
	c = getConfigInfoFromCmd(argc, argv); //parso il cmd cercando solo le opzioni di config
	printConfigInfo(c);

	getToSendRequestsFromCmd(argc, argv);
	//printQueue(toSendRequestQueue);

	int a = openConnection(c.socketName, c.requestInterval, ts); //apro la connessione

	//int result = sendRequests();
	//toSendRequestQueue = getToSendRequestsFromCmd(argc, argv);

	//printf("Connesso al server = %d\n errno = %d\n", a, errno);

	z = openFile("fileDaLeggere.txt", O_CREAT);
	printf("%d %d\n", z, errno);

	z = openFile("fileDaLeggere2.txt", O_CREAT);
	printf("%d %d\n", z, errno);

	z = openFile("fileDaLeggere3.txt", O_CREAT);
	printf("%d %d\n", z, errno);

	z = openFile("fileDaLeggere4.txt", O_CREAT);
	printf("%d %d\n", z, errno);

	z = openFile("fileDaLeggere5.txt", O_CREAT);
	printf("%d %d\n", z, errno);

	//z = openFile("fileDaLeggere.txt", O_CREAT);
	//printf("%d %d\n", z, errno);

	z = writeFile("fileDaLeggere.txt", NULL);
	printf("%d %d\n", z, errno);

	z = writeFile("fileDaLeggere2.txt", NULL);
	printf("%d %d\n", z, errno);

	//z = writeFile("fileDaLeggere3.txt", NULL);
	//printf("%d %d\n", z, errno);

	/*z = writeFile("fileDaLeggere.txt", NULL);
	printf("%d %d\n", z, errno);*/
	//srand(time(NULL));
	sleep(20);

	int x = closeConnection(c.socketName);
	printf("Esito terminazione connesione: %d\n", x);
	return 0;
}