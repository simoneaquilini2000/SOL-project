#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
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

ssize_t readn(int fd, void *ptr, size_t n) {  
   size_t   nleft;
   ssize_t  nread;
 
   nleft = n;
   while (nleft > 0) {
		//printf("Mi manca da leggere = %d\n", nleft);
     if((nread = read(fd, ptr, nleft)) < 0) {
        if (nleft == n) return -1; /* error, return -1 */
        else break; /* error, return amount read so far */
     } else if (nread == 0) {//printf("OK3\n"); 
	 break;} /* EOF */
     nleft -= nread;
	 //printf("Ho letto %d\n", nread);
	 //printf("Primo carattere = %d\n", (int) ((char*)ptr)[0]);
	 //for(int j = 0; j < nread; j++)
	 	//printf("%c\n", *((char*)ptr + j));
     ptr   += nread;
   }
   //printf("OK4\n");
   return (n - nleft); /* return >= 0 */
}

ssize_t writen(int fd, void *ptr, size_t n) {  
   size_t   nleft;
   ssize_t  nwritten;
 
   nleft = n;
   while (nleft > 0) {
	   //printf("Devo scrivere %s\n", (char*)ptr);
     if((nwritten = write(fd, ptr, nleft)) < 0) {
        if (nleft == n) return -1; /* error, return -1 */
        else break; /* error, return amount written so far */
     } else if (nwritten == 0) break; 
     nleft -= nwritten;
     ptr   += nwritten;
   }
   return(n - nleft); /* return >= 0 */
}