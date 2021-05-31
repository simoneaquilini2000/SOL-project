#include<stdio.h>
#include<stdlib.h>
#include<string.h>
//#include<limits.h>
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

	if(strcmp(a.filePath, b.filePath) == 0) //ogni file è identificato tramite il path assoluto
		return 1;
	return 0;
}

void filePrint(void* info){
	if(info == NULL)
		return;
	MyFile f;
	memset(&f, 0, sizeof(MyFile));

	f = *(MyFile*) info;
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

	if(f->content != NULL)
		free(f->content);
	//free(f); 
}

char* getFileNameFromPath(char path[]){
	//printf("Original path: %s\n", path);

	char *token = strtok(path, "/");
	char *ris;
	int dim = 0;

	/*
		Tokenizzo la stringa fino ad arrivare all'ultimo token
		che mi rappresenterà il nome del file su cui opero
	*/
	while(token != NULL){
		ris = malloc(strlen(token) + 1);
		memset(ris, 0, strlen(token));
		strncpy(ris, token, strlen(token));
		dim = strlen(ris);
		ris[dim] = '\0';
		token = strtok(NULL, "/");
		if(token != NULL)
			free(ris);
	}
	//printf("Real filename: %s\n", ris);
	return ris;
}

int saveFile(MyFile f, const char dirname[]){
	char previousCwd[1024];
	char absPath[PATH_MAX];
	int l;
	char *ris = realpath(dirname, absPath); 
	//la cartella deve già esistere, altrimenti realpath tornerà NULL

	getcwd(previousCwd, 1024);

	if(ris == NULL){
		printf("Cartella specificata non esistente\n");
		return -1;
	}
	if(chdir(absPath) == -1){
		printf("Errore chdir\n");
		return -1;
	}

	//printf("WOW2 = %s\n", f.filePath);

	char *fileName = getFileNameFromPath(f.filePath);

	printf("WOW = %s\n", fileName);

	int file_fd = open(fileName, O_RDWR | O_CREAT, 0700);

	if(file_fd == -1){
		perror("Errore open in sola scrittura!\n");
		return -1;
	}

	if((l = writen(file_fd, f.content, f.dim)) == -1){
		perror("Errore write!\n");
		chdir(previousCwd);
		return -1;
	}

	//printf("Dovevo scrivere %d caratteri, \
	 ma ho scritto %d caratteri cioè %s\n", f.dim, l, f.content);

	close(file_fd);
	/*FILE *toWrite = fopen(fileName, "wb");

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
	}*/

	if(chdir(previousCwd) == -1){
		return -1;
	}
	return 1;
}

int readFileContent(const char *pathname, char **fileContent){
	if(pathname == NULL)
		return NULL;

	char absPath[PATH_MAX];
	char recBuf[1024];
	memset(recBuf, 0, 1024);
	char *ris = realpath(pathname, absPath);

	//Leggo il contenuto del file(che si trova in absPath) e modifico fileContent

	if(ris == NULL) //ris == NULL significa che pathname non è stato trovato
		return -1;

	*fileContent = malloc(1);
	int act_dim = 1, precDim;
	int fileDim = 0;
	int charToAdd;

	FILE *fp = fopen(absPath, "r");

    // checking if the file exist or not
    if (fp == NULL)
    {
        printf("(%d) - File Not Found!\n", getpid());
        return -1;
    }

    fseek(fp, 0L, SEEK_END);

    long int expectedFileSize = ftell(fp);

    fclose(fp);

	*fileContent = (char*) calloc(expectedFileSize + 1, sizeof(char));

	int file_fd = open(absPath, (O_RDWR | O_APPEND));

	if(file_fd == -1){
		perror("Errore open in sola lettura!\n");
		return -1;
	}

	fileDim = readn(file_fd, *fileContent, expectedFileSize);

	//printf("Ho letto %d caratteri cioè %s\n", fileDim, *fileContent);

	/*while((charToAdd = readn(file_fd, recBuf, 1024)) > 0){
		printf("Ho letto %d caratteri\n", charToAdd);
		precDim = fileDim;
		 //aumento numero di byte letti
		//precDim = strlen(*fileContent) + 1;
		while(precDim + charToAdd >= act_dim){
			//printf("Rialloco\n");
			*fileContent = realloc(*fileContent, sizeof(char) * (2 * act_dim));
			if(*fileContent == NULL){
				close(file_fd);
				printf("BAB\n");
				return -1;
			}
			act_dim *= 2;
		}
		memcpy((*fileContent) + fileDim, recBuf, charToAdd);
		fileDim += charToAdd;
		memset(recBuf, 0, 1024);
		*fileContent[fileDim] = '\0';
	}

	printf("File dimension= %d\n", fileDim);

	close(file_fd);

	/*FILE *toRead = fopen(absPath, "rb");

	while((charToAdd = fread(recBuf, sizeof(char), 1024, toRead)) > 0){
		//printf("Ho letto %d caratteri\n", charToAdd);
		precDim = fileDim;
		 //aumento numero di byte letti
		//precDim = strlen(*fileContent) + 1;
		while(precDim + charToAdd >= act_dim){
			//printf("Rialloco\n");
			*fileContent = realloc(*fileContent, sizeof(char) * (2 * act_dim));
			if(*fileContent == NULL){
				fclose(toRead);
				return NULL;
			}
			act_dim *= 2;
		}
		memcpy((*fileContent) + fileDim, recBuf, charToAdd);
		fileDim += charToAdd;
		//fileContent[fileDim] = '\0';
	}

	fclose(toRead);*/

	//printf("Contenuto: %s\n", fileContent);

	return fileDim;
}

//traduzione da path relativo ad assoluto
void getAbsPathFromRelPath(char *pathname, char absPath[], int len){
    if(pathname == NULL || strcmp(pathname, "") == 0)
        return;
    char currWorkDir[1024], pathBackup[1024];
    getcwd(currWorkDir, 1024);
    strncpy(pathBackup, pathname, strlen(pathname));

    //salvo preventivamente la cwd in modo da poter tornare qui in modo sicuro

    if(pathname[0] == '/'){
        printf("Pathname è già un path assoluto\n");
		strncpy(absPath, pathname, strlen(pathname));
        return;
    }
    //guardo se pathname inizia con /, se sì ritorno perchè pathname è già assoluto

    char *token = strtok(pathname, "/");
    char *fileName;
    //divido stringa in base a carattere '/'

    if(strcmp(token, pathBackup) == 0){ //non ho '/' quindi ho un solo token
        getcwd(absPath, len);
        if(strlen(absPath) + strlen(pathBackup) + 2 < len){
            strncat(absPath, "/", 1);
            strncat(absPath, pathBackup, strlen(pathBackup));
        }
        return;
    }

    while(token != NULL){
        fileName = malloc(strlen(token) + 1);
		memset(fileName, 0, strlen(token) + 1);
        strncpy(fileName, token, strlen(token));
        //per ogni token cambio cwd nella cartella indicata dal token e mi salvo il precedente
        if(chdir(token) == -1){
            break;
        }
        token = strtok(NULL, "/");
        if(token != NULL)
            free(fileName);
    }
    //una volta finito il ciclo metto in un buffer la cwd e ci appendo il nome del file(quello sarà il mio path assoluto)
    getcwd(absPath, len);
    if(strlen(absPath) + strlen(fileName) + 2 < len){
        strncat(absPath, "/", 1);
        strncat(absPath, fileName, strlen(fileName));
    }

    chdir(currWorkDir); //torno a cwd iniziale
}