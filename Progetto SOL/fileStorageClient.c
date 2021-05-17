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
#include "clientConfig.h"
#include "generic_queue.h"
#include "serverAPI.h"

ClientConfigInfo c; // struttura che mi conterrà le informazioni di configurazione del client
GenericQueue toSendRequestQueue; //coda di richieste da mandare al server

//static GenericQueue toSendRequests;

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

	/*char b[1024], backup[1024];

	strcpy(b, "/home/simone/programmazione/Sistemi_Operativi/Esercizi lezione 5/myFind.c");

	strcpy(backup, b);

	char* str = getFileNameFromPath(b);
	char* absPath = findFile("/home", str);
	if(absPath != NULL){
		printf("Ho trovato file %s ed il suo path è %s\n", str, absPath);
	}else
		printf("Non ho trovato %s\n", str);
	*/
	//return 0;

	int a = openConnection(c.socketName, c.requestInterval, ts); //apro la connessione
	//toSendRequestQueue = getToSendRequestsFromCmd(argc, argv);

	//printf("Connesso al server = %d\n errno = %d\n", a, errno);

	z = openFile("/tmp/albero.txt", O_CREAT);
	printf("%d %d\n", z, errno);
	/*z = openFile("/tmp/albero2.txt", O_CREAT);
	printf("%d %d\n", z, errno);
	z = openFile("/tmp/albero3.txt", O_CREAT);
	printf("%d %d\n", z, errno);
	z = openFile("/tmp/albero4.txt", O_CREAT);
	printf("%d %d\n", z, errno);
	z = openFile("/tmp/albero5.txt", O_CREAT);

	printf("%d %d\n", z, errno);*/

	sleep(2);
	//t= closeFile("/tmp/albero.txt");

	//printf("Chiudo file= %d %d\n", t, errno);

	//printf("BA\n");
	char buffer[40];
	int ris;

	char *recBuf;
	size_t recBufSize;

	memset(buffer, 0, sizeof(buffer));

	//strcpy(buffer, "Oggi e' bellissimo aiuto mamma mia!");
	//y = appendToFile("/tmp/albero.txt", buffer,	strlen(buffer), c.saveReadFileDir);
	//printf("BU\n");

	//y = removeFile("/tmp/albero.txt");

	//printf("%d %d\n", y, errno);

	ris = openFile("/tmp/albero2.txt", O_CREAT);
	printf("%d %d\n", ris, errno);

	sleep(1);

	ris = openFile("/tmp/albero3.txt", O_CREAT);
	printf("%d %d\n", ris, errno);

	sleep(1);

	ris = openFile("/tmp/albero4.txt", O_CREAT);
	printf("%d %d\n", ris, errno);

	//ris = readNFiles(2, c.saveReadFileDir);
	//printf("%d %d\n", ris, errno);

	sleep(2);

	strncpy(buffer, "Oggi e' brutto aiuto mamma mia!", strlen("Oggi e' brutto aiuto mamma mia!"));
	y = appendToFile("/tmp/albero.txt", buffer,	strlen(buffer), c.saveReadFileDir);
	//printf("BU\n");
	printf("%d %d\n", y, errno);

	z = readFile("/tmp/albero.txt", (void**) &recBuf, &recBufSize);
	printf("%d %d\n", z, errno);
	if(z == 0)
		printf("Ho ricevuto il buffer %s che ha size %d\n", recBuf, recBufSize);

	/*z = openFile("/tmp/albero10.txt", O_CREAT);
	printf("%d %d\n", z, errno);*/

	sleep(2);

	z = readNFiles(2, c.saveReadFileDir);

	sleep(1);

	int x = closeConnection(c.socketName);
	printf("Esito terminazione connesione: %d\n", x);
	return 0;
}