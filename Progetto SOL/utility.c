#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include "utility.h"

void clearBuffer(char in[], int len){
	int i;

	for(i = 0; i < len; i++)
		in[i] = 0;
}

void myStrNCpy(char out[], char in[], int len){
	int i = 0;

	while(i < len && in[i] != '\n' && in[i] != '\r'){
		out[i] = in[i];
		i++;
	}

	if(i < len)
		out[i] = '\0';
}

serverInfo startConfig(const char* sockPath){
	FILE *f;
	int i = 0, j;
	char buf[1024], errMess[1024];
	char *params[N_INFO];
	serverInfo s;

	f = fopen(sockPath, "r");
	if(f == NULL){
		perror("Error during config file opening!");
		exit(EXIT_FAILURE);
	}

	while(fgets(buf, 1024, f) != NULL && i < N_INFO){
		myStrNCpy(buf, buf, strlen(buf));
		params[i] = malloc(sizeof(char) * (strlen(buf) + 1));
		strncpy(params[i], buf, strlen(buf));
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
	}


	if(s.maxStorageSpace <= 0 || s.nMaxFile <= 0 || s.nWorkers <= 0){
		perror("maxStorageSpace, nMaxFile and nWorkers must be > 0");
		exit(EXIT_FAILURE);
	}

	for(j = 0; j < i; j++){
		free(params[j]);
	}

	fclose(f);
	return s;
}

int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=read((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;   // EOF
        left    -= r;
	bufptr  += r;
    }
    return size;
}

int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=write((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;  
        left    -= r;
	bufptr  += r;
    }
    return 1;
}