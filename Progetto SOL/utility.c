#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<math.h>
#include<string.h>
#include<errno.h>
#include "utility.h"

/*
	funzione di attesa in millisecondi(rientrante)
*/
void msleep(int msec){
	struct timespec ts, tm;

	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * pow(10, 6);

	do{
		nanosleep(&ts, &tm);
	}while(errno == EINTR);
}

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