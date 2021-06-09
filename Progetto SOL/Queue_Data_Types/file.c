#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include "utility.h"
#include "generic_queue.h"
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

	printf("Path del file: %s\n", f.filePath);
	printf("Dimensione(in B): %d\n", f.dim);
	//verifico se f.content è stato allocato o meno(non sto verificando se sia vuoto o meno)
	if(f.content == NULL)
		printf("Contenuto(NULL)\n");
	else
		printf("Contenuto: %s\n", f.content);
	printf("Timestamp: %s\n", buff);
	printf("Lockato: %d\n", f.isLocked);
	printf("Aperto: %d\n", f.isOpen);
	printf("Modificato: %d\n", f.modified);
	printf("Ultima operazione eseguita con successo sul file:\n");
	printf("\tTipo di operazione: %d\n", f.lastSucceedOp.opType);
	printf("\tFlags dell'operazione: %d\n", f.lastSucceedOp.optFlags);
	printf("\tDescrittore lato server (che indica il client):%d\n\n", f.lastSucceedOp.clientDescriptor);
}

void freeFile(void* s){
	MyFile *f = (MyFile*)s;

	if(f->content != NULL)
		free(f->content); //libero il contenuto
	free(f); //libero il nodo informativo
}

char* getFileNameFromPath(char path[]){
	char *token = strtok(path, "/");
	char *ris = NULL; //conterrà il nome del file
	int dim = 0;

	/*
		Tokenizzo la stringa fino ad arrivare all'ultimo token
		che mi rappresenterà il nome del file su cui opero
	*/
	while(token != NULL){
		ris = malloc(strlen(token) + 1);
		memset(ris, 0, strlen(token) + 1);
		strcpy(ris, token);
		dim = strlen(ris);
		ris[dim] = '\0';
		token = strtok(NULL, "/");
		//se non ho più token disponibili, l'ultimo salvato in ris sarà il nome del file
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
	//la cartella deve già esistere, altrimenti realpath tornerà NULL

	//mi salvo la cwd in modo da tornarvi una volta eseguito il salvataggio
	getcwd(previousCwd, 1024); 

	if(ris == NULL){
		printf("Cartella specificata non esistente\n");
		return -1;
	}

	if(chdir(absPath) == -1){
		printf("Errore chdir\n");
		return -1;
	}

	//estrapolo nome del file dal path
	char *fileName = getFileNameFromPath(f.filePath);

	/*
		Se il file non esiste lo creo con permessi di r/w;
		se invece esiste ne sovrascrivo il contenuto
	*/
	int file_fd = open(fileName, O_RDWR | O_CREAT | O_TRUNC, 0700);

	if(file_fd == -1){
		perror("Errore open!\n");
		chdir(previousCwd);
		return -1;
	}

	//scrivo il contenuto
	if((l = writen(file_fd, f.content, f.dim)) == -1){
		perror("Errore write!\n");
		chdir(previousCwd);
		return -1;
	}

	//chiudo il descrittore
	close(file_fd);

	//torno alla cwd
	if(chdir(previousCwd) == -1){
		return -1;
	}
	return 1;
}

int readFileContent(const char *pathname, char **fileContent){
	if(pathname == NULL) //pathname non può essere NULL
		return -1;

	char absPath[PATH_MAX]; //buffer dove otterrò il path assoluto del file, se esiste
	char recBuf[1024];
	memset(recBuf, 0, 1024);
	char *ris = realpath(pathname, absPath);

	if(ris == NULL) //ris == NULL significa che pathname non è stato trovato(file inesistente)
		return -1;

	int act_dim = 1, precDim;
	int fileDim = 0;
	int charToAdd;

	FILE *fp = fopen(absPath, "rb");//leggo file in modalità binaria

    // controllo se il file esiste o meno
    if (fp == NULL)
    {
        printf("File non trovato!\n");
        return -1;
    }

    fseek(fp, 0L, SEEK_END); //uso libreria I/O per ottenere la lunghezza del contenuto del file

    long int expectedFileSize = ftell(fp);

    fclose(fp);

	//alloco buffer di ricezione del contenuto del file
	*fileContent = (char*) calloc(expectedFileSize + 1, sizeof(char));

	int file_fd = open(absPath, (O_RDWR | O_APPEND));

	if(file_fd == -1){
		perror("Errore open per lettura del contenuto del file!\n");
		return -1;
	}

	//leggo il contenuto del file
	fileDim = readn(file_fd, *fileContent, expectedFileSize);

	close(file_fd);

	return fileDim; //ritorno il numero dei byte letti(-1 in caso di errore di lettura)
}

//traduzione da path relativo ad assoluto
void getAbsPathFromRelPath(char *pathname, char absPath[], int len){
    if(pathname == NULL || strcmp(pathname, "") == 0){
		strcpy(absPath, ""); //errore sui parametri di input
        return;
	}
    char currWorkDir[1024]; 
	char pathBackup[1024];
    getcwd(currWorkDir, 1024);
    strncpy(pathBackup, pathname, strlen(pathname)); //backup del pathname, modificato da strtok

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
            strcat(absPath, "/");
            strncat(absPath, pathBackup, strlen(pathBackup)); //appendo al path, il nome del file
        }else
			strcpy(absPath, ""); //errore ho sforato il massimo spazio disponibile
        return;
    }

    while(token != NULL){
        fileName = malloc(strlen(token) + 1);
		memset(fileName, 0, strlen(token) + 1);
        strncpy(fileName, token, strlen(token));
        //per ogni token cambio cwd nella cartella indicata dal token e mi salvo il precedente
        token = strtok(NULL, "/");
        if(token != NULL){
			if(chdir(fileName) == -1){
				strcpy(absPath, "");
				//provo a tornare alla cwd, segnalando l'errore: se non ci riesco ho errore fatale
				if(chdir(currWorkDir) == -1){ 
					perror("Errore fatale in cambio di directory!\n");
					exit(EXIT_FAILURE);
				}
            	return;
       	 	}
            free(fileName);
		}
    }

    //una volta finito il ciclo metto in un buffer la cwd e ci appendo il nome del file(quello sarà il mio path assoluto)
    getcwd(absPath, len);
    if(strlen(absPath) + strlen(fileName) + 2 < len){
        strcat(absPath, "/");
        strncat(absPath, fileName, strlen(fileName));
    }else{
		strcpy(absPath, ""); //errore, path troppo lungo
	}

    if(chdir(currWorkDir) == -1){ //torno a cwd iniziale
		perror("Errore cambio cwd!\n");
		exit(EXIT_FAILURE);
	} 
}