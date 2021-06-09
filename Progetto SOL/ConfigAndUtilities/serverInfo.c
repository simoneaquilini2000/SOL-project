#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<math.h>
#include<errno.h>
#include "utility.h"
#include "serverInfo.h"

serverInfo startConfig(const char* sockPath){
	FILE *f;
	int i = 0, j;
	char buf[1024]; //buffer di ricezione di una stringa
	char errMess[1024]; //contiene eventuale messaggio di errore
	char **params = calloc(N_INFO, sizeof(char*));
	serverInfo s;
	memset(&s, 0, sizeof(s));

	f = fopen(sockPath, "r"); //leggo dal file di configurazione
	if(f == NULL){ //errore fatale nel caso di non aperutra del file
		perror("Error during config file opening!");
		exit(EXIT_FAILURE);
	}

	while(fgets(buf, 1024, f) != NULL && i < N_INFO){
		int k = 0;
		//tronco la stringa alla prima occorrenza di '\n' o '\r'
		while(buf[k] != '\n' && buf[k] != '\r') k++;
		buf[k] = '\0';
		params[i] = malloc(sizeof(char) * (strlen(buf) + 1));
		strncpy(params[i], buf, strlen(buf) + 1); //inserisco informazioni in params
		i++;
	}

	if(i == N_INFO){
		strncpy(s.socketPath, params[0], strlen(params[0])); //ottengo socket path
		s.nWorkers = atoi(params[1]); //ottengo nWorkers
		s.nMaxFile = atoi(params[2]); //ottengo nMaxFile
		s.maxStorageSpace = atoi(params[3]); //ottengo maxStorageSpace
		strncpy(s.logFilePath, params[4], strlen(params[4]));
		myStrNCpy(s.logFilePath, s.logFilePath, strlen(s.logFilePath));
	}else{ //non ho letto N_INFO righe quindi mancano informazioni di configurazione
		sprintf(errMess, "Wrong config file format: it must have at least %d lines to read\n", N_INFO);
		perror(errMess);
        exit(EXIT_FAILURE);
	}

	//se ho inserito dati di tipo sbagliato ho errore fatale
	if(s.maxStorageSpace <= 0 || s.nMaxFile <= 0 || s.nWorkers <= 0){
		perror("maxStorageSpace, nMaxFile and nWorkers must be > 0");
		exit(EXIT_FAILURE);
	}

	//libero memoria allocata
	for(j = 0; j < i; j++){
		free(params[j]);
	}

	free(params);

	fclose(f); //chiudo il file
	return s;
}

void printConfig(serverInfo s){
    printf("Stampo parametri di configurazione del file storage:\n");
    printf("\tMassima dimensione(in byte): %d\n", s.maxStorageSpace);
    printf("\tMassima quantita di file memorizzabile: %d\n", s.nMaxFile);
    printf("\tNumero di thread workers: %d\n", s.nWorkers);
    printf("\tPath del socket: %s\n", s.socketPath);
    printf("\tPath del file di log: %s\n\n", s.logFilePath);
}

void printServerStats(serverStats s){
	double maxSpaceReached = s.fileCacheMaxStorageSize /((1.00) * pow(10, 6)); //converto da B a MB
    printf("Stampo statistiche del file storage:\n");
	printf("\tMassima dimensione (in MB) raggiunta dalla file cache: %.6f\n", maxSpaceReached);
    printf("\tMassima quantita di file memorizzata: %d\n", s.fileCacheMaxSize);
    printf("\tNumero di volte in cui ho invocato l'algoritmo di rimpiazzamento: %d\n\n", s.replaceAlgInvokeTimes);
}