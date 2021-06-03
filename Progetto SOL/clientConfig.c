#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include<limits.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<dirent.h>
#include<fcntl.h>
#include "clientConfig.h"
#include "generic_queue.h"
#include "file.h"
#include "request.h"
#include "utility.h"

extern GenericQueue toSendRequestQueue;

void printAvailableOptions(){
	printf("Available options:\n");
	printf("\t-f socketName\n");
	printf("\t-w dirName[,n]\n");
	printf("\t-W file1[,file2]\n");
	printf("\t-r file1[,file2]\n");
	printf("\t-R [n]\n");
	printf("\t-d dirName\n");
	printf("\t-t time\n");
	printf("\t-c file1[,file2]\n");
	printf("\t-p\n");
}

long isNumber(const char* s) {
   char* e = NULL;
   //printf("Mi hai dato stringa %s\n", s);
   long val = strtol(s, &e, 0);
   if (errno == ERANGE){
   	return -2;
   }
   if (e != NULL && *e == (char)0) 
   		return val;
   
   return -1;
}

ClientConfigInfo getConfigInfoFromCmd(int argc, const char* argv[]){
	char o;
	ClientConfigInfo conf;
	int foundT = 0;
	int foundF = 0;
	int foundD = 0;
	int foundr = 0;
	int foundR = 0;


	conf.printEnable = 0;
	optind = 1; //resetto l'indice di getopt in modo da poter scandire le opzioni più volte

	while((o = getopt(argc, argv, ALL_OPTIONS)) != -1){
		//printf("Opzione: %c\n", o);
		switch(o){
			case 't': if((conf.requestInterval = (int) isNumber(optarg)) < 0){
						  perror("Wrong argument format for option -t");
						  exit(EXIT_FAILURE);
					  }
					  foundT = 1;
					  break;
			case 'f': if(foundF){
						perror("Can not repeat option -f");
						exit(EXIT_FAILURE);
					  }
					  foundF = 1;
					  //printf("Argomento di -f: %s\n", optarg);
					  strcpy(conf.socketName, optarg);
					  //if(strlen(conf.socketName) < 1023)
					  	//conf.socketName[strlen(conf.socketName) - 1] = '\0';
					  break;

			case 'p': if(conf.printEnable){
						perror("Can not repeat option -p");
						exit(EXIT_FAILURE);
					  }
					  conf.printEnable = 1;
					  break;
			case 'd': if(foundD) //se ho più volte l'opzione -d prendo solo l'ultima occorrenza
						clearBuffer(conf.saveReadFileDir, strlen(conf.saveReadFileDir));
					  foundD = 1;
					  strcpy(conf.saveReadFileDir, optarg);
					  //myStrNCpy(conf.saveReadFileDir, optarg, strlen(optarg));
					  break;
			case 'R': foundR = 1; //cercare qua -r e -R servono solo per verificare la correttezza della richiesta
					  break;
			case 'r': foundr = 1;
					  break;
			case '?': perror("Invalid option(s)");
					  exit(EXIT_FAILURE);
					  break;
			default: break;
		}
	}

	if(foundT == 0)
		conf.requestInterval = 0;
	if(foundD == 0)
		clearBuffer(conf.saveReadFileDir, 1024);
	else{
		if(foundR == 0 && foundr == 0){ //c'è l'opzione -d ma non c'è né -r né -R -> ERRORE
			perror("Option -d must be used with option -r or -R\n");
			exit(EXIT_FAILURE);
		}
	}
	if(foundF == 0){ //necessario il nome del socket
		perror("Required socket name\n");
		exit(EXIT_FAILURE);
	}
	return conf;
}

void printConfigInfo(ClientConfigInfo c){
	printf("Config parameters:\n");
	printf("\tprintEnable: %d\n", c.printEnable);
	printf("\tsocketName: %s\n", c.socketName);
	printf("\tsaveReadFileDir: %s\n", c.saveReadFileDir);
	printf("\trequestInterval: %ld\n", c.requestInterval);
}

int buildInsertRequest(int type, char *arg, int request_flags){
	char *token = strtok(arg, ",");
	char buf[PATH_MAX];
	int ris = 0;
	char *fileToFind;
	char *filePath;

	while(token != NULL){
		MyRequest *r = malloc(sizeof(MyRequest));
		memset(r, 0, sizeof(MyRequest));
		r->type = type;
		r->flags = request_flags;
		//fileToFind = getFileNameFromPath(token);
		memset(buf, 0, sizeof(buf));

		/*
			Se devo scrivere un file, in quanto sarà stato lo stesso client
			a crearlo ed aprirlo; il contenuto da scrivere dovrà
			essere presente nel file system del client.
			In caso contrario, ciò non è strettamente necessario
			quindi va semplicemente tradotto il path relativo
			in assoluto.
		*/
		if(type == WRITE_FILE){
			filePath = realpath(token, buf); //ottengo path assoluto dato quello relativo
			if(filePath == NULL){
				printf("Non ho trovato file %s\n", token);
				ris = -1; //flag con cui segnalo l'errore e tale richiesta non verrà spedita
			}
		}else{
			getAbsPathFromRelPath(token, buf, PATH_MAX);
			if(strcmp(buf, "") == 0)
				ris = -1;
		}

		if(ris == 0){
			strncpy(r->request_content, buf, strlen(buf));
			if(push(&toSendRequestQueue, (void*)r) == -1){
				perror("errore push!");
				exit(EXIT_FAILURE);
			}
		}
		
		ris = 0;
		token = strtok(NULL, ",");
	}
	return 0;
}

int buildReadNRequest(int type, char *arg, int request_flags){
	long ris;
	MyRequest *toAdd = malloc(sizeof(MyRequest));
	memset(toAdd, 0, sizeof(MyRequest));
	
	toAdd->type = type;

	if(arg == NULL){
		ris = 0;
		char toSend[10];
		sprintf(toSend, "%d", ris); //senza argomento la richiesta si trasforma in -R 0
		toSend[strlen(toSend)] = '\0';
		printf("Argomento non specificato quindi inserisco %s\n", toSend);
		strncpy(toAdd->request_content, toSend, strlen(toSend));
	}else{
		//printf("Argomento = %s\n", arg);
		int arg_size = strlen(arg);
		char buf[arg_size + 1];
		strncpy(buf, arg, arg_size);
		//printf("Buffer = %s\n", buf);
		//ris = atoi(buf);
		//printf("ris = %d\n", ris);
		strncpy(toAdd->request_content, arg, strlen(arg));
	}
	toAdd->flags = request_flags;

	//printf("Ho modificato il contenuto = %s\n", toAdd->request_content);

	if(push(&toSendRequestQueue, toAdd) == -1){
		perror("errore push!");
		exit(EXIT_FAILURE);
	}
	return 0;
}

int navigateFileSystem(char *rootPath, int n, int flag, int request_type, int request_flags){
	if(flag && n <= 0) //flag usato per indicare se c'è una quantità massima di file da leggere
		return 0;

	DIR *currentDir;
	struct dirent *actFile;
	struct stat fileInfo;
	int readFiles = 0;
	char buf[PATH_MAX], cWorkDir[1024];

	currentDir = opendir(rootPath);
	if(currentDir == NULL)
		return 0;

	if(chdir(rootPath) == -1)
		return 0;

	errno = 0;
	while((actFile = readdir(currentDir)) != NULL && errno == 0){
		if(stat(actFile->d_name, &fileInfo) == -1)
			return readFiles;

		if(strcmp(actFile->d_name, ".") != 0 && strcmp(actFile->d_name, "..") != 0){
			if(S_ISDIR(fileInfo.st_mode)){
				getcwd(cWorkDir, 1024);
				readFiles += navigateFileSystem(actFile->d_name, n - readFiles, flag, request_type, request_flags);
				if(chdir(cWorkDir) == -1){
					printf("Errore cambio working directory!\n");
					exit(EXIT_FAILURE);
				}
			}else{
				if((flag && readFiles < n) || !flag){
					MyRequest *r = malloc(sizeof(MyRequest));
					memset(r, 0, sizeof(MyRequest));
					r->type = request_type;
					r->flags = request_flags;
					//printf("Inserisco richiesta WRITE_FILE per il file %s\n", actFile->d_name);
					if(realpath(actFile->d_name, buf) != NULL){
						//per costruire richiesta ottengo path assoluto dei file
						strncpy(r->request_content, buf, strlen(buf));
						if(push(&toSendRequestQueue, (void*)r) == -1){
							perror("Errore push!\n");
							exit(EXIT_FAILURE);
						}
						readFiles++;
					}else{
						printf("Non ho trovato il path specificato!\n");
						free(r);
					}
				}
			}
		}
	}

	closedir(currentDir);
	return readFiles;
}

int buildMultipleWriteRequest(int type, char* arg, int request_flags){
	char *dir = strtok(arg, ","); //ottengo cartella da cui partire per manadare richieste di WRITE_FILE
	char buf[PATH_MAX];
	char currDir[PATH_MAX];

	//Gestisco l'opzione -w

	if(realpath(dir, buf) == NULL){
		printf("La cartella specificata non esiste\n");
		return -1;
	}

	char *sFile = strtok(NULL, ","); //ottengo eventuale n parametro aggiuntivo
	int n, flag;

	if(sFile == NULL){ //se n non è specificato, non ho limite sul numero di file da leggere
		n = 0;
		flag = 0;
	}else{
		n = atoi(sFile);
		if(n == 0)
			flag = 0;
		else
			flag = 1;
	}
	getcwd(currDir, PATH_MAX);
	int writtenRequests = navigateFileSystem(buf, n, flag, type, request_flags); //navigo FS ed inserisco le richieste
	chdir(currDir);
	//printf("Ora la cwd è: %s\n", currDir); //devo ristabilire cwd
	if(flag && writtenRequests != n)
		return -1;
	else if(flag && writtenRequests == n)
		return 0;
	else
		return writtenRequests;
}

void getToSendRequestsFromCmd(int argc, const char* argv[]){
	char o;

	optind = 1;
	while((o = getopt(argc, argv, ALL_OPTIONS)) != -1){
		switch(o){
			case 'w': buildMultipleWriteRequest(WRITE_FILE, optarg, 0);
					  break;
			case 'r': buildInsertRequest(READ_FILE, optarg, 0);
					  break;
			case 'W': buildInsertRequest(WRITE_FILE, optarg, 0);
					  break;
			case 'R': //printf("Ciao\n");
					  buildReadNRequest(READ_N_FILE, optarg, 0);
					  //printf("Ciao2\n");
					  break;
			case 'c': buildInsertRequest(REMOVE_FILE, optarg, 0);
					  break;
			case 'i': buildMultipleWriteRequest(OPEN_FILE, optarg, O_CREAT);
					  break;
			case 'o': buildInsertRequest(OPEN_FILE, optarg, 0);
					  break;
			case 'C': buildInsertRequest(CLOSE_FILE, optarg, 0);
					  break;
			case '?': printf("Invalid option(s)\n");
					  exit(EXIT_FAILURE);
					  break;
			default: break;
		}
	}
}