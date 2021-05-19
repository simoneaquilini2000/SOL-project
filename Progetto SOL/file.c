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
//#include "request.h"
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
	printf("Modified: %d\n", f.modified);
	printf("Last successful operation:\n");
	printf("\tOperation type: %d\n", f.lastSucceedOp.opType);
	printf("\tOptional flags: %d\n", f.lastSucceedOp.optFlags);
	printf("\tServer to Client descriptor: %d\n\n", f.lastSucceedOp.clientDescriptor);
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

	char *fileName = getFileNameFromPath(f.filePath);
	FILE *toWrite = fopen(fileName, "w");

	if(toWrite == NULL){
		perror("Errore open!\n");
		chdir(previousCwd);
		return -1;
	}

	if((l = fwrite(f.content, sizeof(char), f.dim, toWrite)) == -1){
		perror("Errore write!\n");
		chdir(previousCwd);
		return -1;
	}

	if(fclose(toWrite) != 0){
		perror("Errore close!\n");
		chdir(previousCwd);
		return -1;
	}

	if(chdir(previousCwd) == -1){
		return -1;
	}
	return 1;
}

char* readFileContent(const char *pathname, char **fileContent){
	if(pathname == NULL)
		return NULL;

	char absPath[PATH_MAX];
	char recBuf[1024];
	char *ris = realpath(pathname, absPath);

	//printf("Leggo file: %s\n", absPath);

	if(ris == NULL) //ris == NULL significa che pathname non è stato trovato
		return NULL;

	*fileContent = malloc(1);
	int act_dim = 1, precDim;
	int charToAdd;

	FILE *toRead = fopen(absPath, "rb");

	while((charToAdd = fread(recBuf, sizeof(char), 1024, toRead)) > 0){
		//printf("Ho letto %d caratteri\n", charToAdd);
		precDim = strlen(*fileContent) + 1;
		while(precDim + charToAdd >= act_dim){
			//printf("Rialloco\n");
			*fileContent = realloc(*fileContent, sizeof(char) * (2 * act_dim));
			if(*fileContent == NULL){
				fclose(toRead);
				return NULL;
			}
			act_dim *= 2;
		}
		strncat(*fileContent, recBuf, charToAdd);
		fileContent[strlen(*fileContent)] = '\0';
	}

	fclose(toRead);

	//printf("Contenuto: %s\n", fileContent);

	return *fileContent;
}