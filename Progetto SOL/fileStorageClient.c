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

extern GenericQueue toSendRequestQueue; //coda di richieste da dare al server
ClientConfigInfo c; //struttura che mantiene informazioni di configurazione del client

void printMainRequestInfo(int ris, MyRequest *actReq){
	printf("Stampo informazioni post esecuzione della richiesta:\n");
	printf("\tTipo operazione(flags): %d(%d)\n", actReq->type, actReq->flags);
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
		if(actReq == NULL)
			return -1;
		printf("\n");
		switch(actReq->type){
			case WRITE_FILE:
				printf("Inizio richiesta WRITE FILE\n"); 
				ris = writeFile(actReq->request_content, NULL);
				if(c.printEnable){
					printMainRequestInfo(ris, actReq);
				}
				printf("Fine richiesta WRITE FILE\n");
				break;
			case READ_FILE:
				printf("Inizio richiesta READ FILE\n");
				ris = readFile(actReq->request_content, (void**)&readDataBuffer, &readDataSize);
				if(c.printEnable){
					printMainRequestInfo(ris, actReq);
					if(ris >= 0)
						printf("\tByte letti: %d\n\n", readDataSize);
				}
				printf("Fine richiesta READ FILE\n");
				if(ris < 0)
					break;

				//creo buffer con "puppa!"
				//concatenavo buffer al contenunto letto
				//scrivevo un file in append con questo contenuto 
				MyFile toSave;
				memset(&toSave, 0, sizeof(toSave));
				strcpy(toSave.filePath, actReq->request_content);
				toSave.content = malloc(sizeof(char) * (readDataSize + 1));
				if(toSave.content == NULL)
					break;
				strcpy(toSave.content, readDataBuffer);
				toSave.dim = readDataSize;
				toSave.content[toSave.dim] = '\0';
				if(saveFile(toSave, c.saveReadFileDir) == -1){
					printf("Errore salvataggio file!");
				}
				free(toSave.content);
				free(readDataBuffer);
				readDataSize = 0;
				break;
			case READ_N_FILE:
				printf("Inizio richiesta READ_N_FILE\n");
				ris = readNFiles(atoi(actReq->request_content), c.saveReadFileDir);
				if(c.printEnable){
					printMainRequestInfo(ris, actReq);
					if(ris >= 0)
						printf("\tNumero di file letti: %d\n\n", ris);
				}
				printf("Fine richiesta READ_N_FILE\n");
				break;
			case REMOVE_FILE:
				printf("Inizio richiesta REMOVE FILE\n");
				ris = removeFile(actReq->request_content);
				if(c.printEnable){
					printMainRequestInfo(ris, actReq);
				}
				printf("Fine richiesta REMOVE FILE\n");
				break;
			case OPEN_FILE:
				printf("Inizio richiesta OPEN FILE\n");
				ris = openFile(actReq->request_content, actReq->flags);
				if(c.printEnable){
					printMainRequestInfo(ris, actReq);
				}
				printf("Fine richiesta OPEN FILE\n");
				break;
			case CLOSE_FILE:
				printf("Inizio richiesta CLOSE FILE\n");
				ris = closeFile(actReq->request_content);
				if(c.printEnable){
					printMainRequestInfo(ris, actReq);
				}
				printf("Fine richiesta CLOSE FILE\n");
				break;
			default: printf("Per mandare altri tipi di richieste, usare esplicitamente \
				la server API\n");
				break;
		}
		freeRequest(actReq);
		msleep(c.requestInterval);
	}
	return 0;
}

int main(int argc, char *const *argv){
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
	printConfigInfo(c); // c mi conterrà le informazioni di configurazione del client

	getToSendRequestsFromCmd(argc, argv);

	int a = openConnection(c.socketName, c.requestInterval, ts); //apro la connessione



	int result = sendRequests();

	int x = closeConnection(c.socketName);
	printf("Esito terminazione connesione: %d\n", x);
	return 0;
}