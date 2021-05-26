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
	char buf[1024], errMess[1024];
	char **params = calloc(N_INFO, sizeof(char*));
	serverInfo s;
	memset(&s, 0, sizeof(s));

	f = fopen(sockPath, "r");
	if(f == NULL){
		perror("Error during config file opening!");
		exit(EXIT_FAILURE);
	}

	while(fgets(buf, 1024, f) != NULL && i < N_INFO){
		//perror("SOS!\n");
		//myStrNCpy(buf, buf, strlen(buf));
		//printf("Cosa ho letto dal file prima della formattazione: %s\n", buf);
		int k = 0;
		while(buf[k] != '\n' && buf[k] != '\r') k++;

		buf[k] = '\0';
		//printf("Cosa ho letto dal file: %s\n", buf);
		//printf("StrLen di buf = %d\n", strlen(buf));
		params[i] = malloc(sizeof(char) * (strlen(buf) + 1));
		//memset(params[i], 0, sizeof(char) * strlen(buf));
		strncpy(params[i], buf, strlen(buf) + 1);
		//printf("Ho inserito in params[%d]: %s\n", i, params[i]);
		i++;
	}

	if(i == N_INFO){
		strncpy(s.socketPath, params[0], strlen(params[0]));
		s.nWorkers = atoi(params[1]);
		s.nMaxFile = atoi(params[2]);
		s.maxStorageSpace = atoi(params[3]);
		strncpy(s.logFilePath, params[4], strlen(params[4]));
		myStrNCpy(s.logFilePath, s.logFilePath, strlen(s.logFilePath));
	}else{
		sprintf(errMess, "Wrong config file format: it must have at least %d lines to read\n", N_INFO);
		perror(errMess);
        exit(EXIT_FAILURE);
	}


	if(s.maxStorageSpace <= 0 || s.nMaxFile <= 0 || s.nWorkers <= 0){
		perror("maxStorageSpace, nMaxFile and nWorkers must be > 0");
		exit(EXIT_FAILURE);
	}

	for(j = 0; j < i; j++){
		free(params[j]);
	}

	free(params);

	fclose(f);
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
	double maxSpaceReached = s.fileCacheMaxStorageSize /((1.00) * pow(10, 6));
    printf("Stampo statistiche del file storage:\n");
    //printf("\tMassima dimensione (in byte) raggiunta dalla file cache: %d\n", s.fileCacheMaxStorageSize);
	printf("\tMassima dimensione (in MB) raggiunta dalla file cache: %.6f\n", maxSpaceReached);
    printf("\tMassima quantita di file memorizzata: %d\n", s.fileCacheMaxSize);
    printf("\tNumero di volte in cui ho invocato l'algoritmo di rimpiazzamento: %d\n\n", s.replaceAlgInvokeTimes);
}