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

	int a = openConnection(c.socketName, c.requestInterval, ts); //apro la connessione
	//toSendRequestQueue = getToSendRequestsFromCmd(argc, argv);

	//printf("Connesso al server = %d\n errno = %d\n", a, errno);

	z = openFile("fileDaLeggere1.txt", O_CREAT);
	printf("%d %d\n", z, errno);

	z = openFile("fileDaLeggere2.txt", O_CREAT);
	printf("%d %d\n", z, errno);

	z = openFile("fileDaLeggere3.txt", O_CREAT);
	printf("%d %d\n", z, errno);

	z = openFile("fileDaLeggere4.txt", O_CREAT);
	printf("%d %d\n", z, errno);

	z = openFile("fileDaLeggere5.txt", O_CREAT);
	printf("%d %d\n", z, errno);

	z = openFile("fileDaLeggere.txt", O_CREAT);
	printf("%d %d\n", z, errno);

	z = writeFile("fileDaLeggere.txt", NULL);
	printf("%d %d\n", z, errno);

	z = writeFile("fileDaLeggere2.txt", NULL);
	printf("%d %d\n", z, errno);

	z = writeFile("fileDaLeggere3.txt", NULL);
	printf("%d %d\n", z, errno);

	/*z = writeFile("fileDaLeggere.txt", NULL);
	printf("%d %d\n", z, errno);*/

	int x = closeConnection(c.socketName);
	printf("Esito terminazione connesione: %d\n", x);
	return 0;
}