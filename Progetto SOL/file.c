#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>
#include<time.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include "utility.h"
#include "generic_queue.h"
#include "request.h"
#include "file.h"

int fileComparison(void* f1, void *f2){
	MyFile a = *(MyFile*) f1;
	MyFile b = *(MyFile*) f2;

	if(strcmp(a.filePath, b.filePath) == 0)
		return 1;
	return 0;
}

void filePrint(void* info){
	if(info == NULL)
		printf("BAB\n");
	MyFile f = *(MyFile*) info;
	char buff[20];

	//ottengo in buff una stringa nel formato Y-m-d H:M:S del timestamp
	strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&f.timestamp));

	printf("FilePath: %s\n", f.filePath);
	printf("Dimension: %d\n", f.dim);
	if(f.content == NULL)
		printf("Bad content\n");
	else
		printf("Content: %s\n", f.content);
	printf("Timestamp: %s\n", buff);
	printf("Locked: %d\n", f.isLocked);
	printf("Open: %d\n", f.isOpen);
	printf("Modified: %d\n\n", f.modified);
}

void freeFile(void* s){
	MyFile *f = (MyFile*)s;

	free(f->content);
	free(f); 
}

char* getFileNameFromPath(char path[]){
	char *token = strtok(path, "/");
	char *ris;

	while(token != NULL){
		ris = malloc(strlen(token) + 1);
		strncpy(ris, token, strlen(token));
		token = strtok(NULL, "/");
		if(token != NULL)
			free(ris);
	}

	return ris;
}

int saveFile(MyFile f, const char dirname[]){
	char previousCwd[1024];
	char absPath[PATH_MAX];
	int l;
	char *ris = realpath(dirname, absPath);

	getcwd(previousCwd, 1024);

	if(ris == NULL)
		return -1;
	if(chdir(absPath) == -1){
		return -1;
	}

	/*
		TO DO: decidere cosa fare in caso di
		scrittura di un file già esistente
	*/

	char *fileName = getFileNameFromPath(f.filePath);
	int toWrite = open(fileName, O_WRONLY | O_CREAT, 0700);

	if(toWrite == -1){
		perror("Errore open!\n");
		chdir(previousCwd);
		return -1;
	}

	if((l = writen(toWrite, f.content, f.dim)) == -1){
		perror("Errore write!\n");
		chdir(previousCwd);
		return -1;
	}

	if(close(toWrite) == -1){
		perror("Errore close!\n");
		chdir(previousCwd);
		return -1;
	}

	if(chdir(previousCwd) == -1){
		return -1;
	}
	return 1;
}