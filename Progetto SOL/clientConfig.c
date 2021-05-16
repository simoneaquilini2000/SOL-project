#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include<limits.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<dirent.h>
#include "clientConfig.h"
#include "generic_queue.h"
//#include "file.h"
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

	conf.printEnable = 0;
	optind = 1;

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
			case 'd': if(foundD)
						clearBuffer(conf.saveReadFileDir, strlen(conf.saveReadFileDir));
					  foundD = 1;
					  strcpy(conf.saveReadFileDir, optarg);
					  //myStrNCpy(conf.saveReadFileDir, optarg, strlen(optarg));
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
	if(foundF == 0){
		perror("Required socket name");
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

int buildInsertRequest(int type, char *arg){
	char *token = strtok(arg, ",");
	char buf[PATH_MAX];
	char *fileToFind;
	char *filePath;

	while(token != NULL){
		MyRequest *r = malloc(sizeof(MyRequest));
		r->type = type;
		//fileToFind = getFileNameFromPath(token);
		filePath = realpath(token, buf); //ottengo path assoluto dato quello relativo
		if(filePath == NULL){
			printf("Non ho trovato file %s\n", token);
		}else{
			strncpy(r->request_content, buf, strlen(buf));
			if(push(&toSendRequestQueue, (void*)r) == -1){
				perror("errore push!");
				exit(EXIT_FAILURE);
			}
		}
		token = strtok(NULL, ",");
	}
	return 0;
}

int buildReadNRequest(int type, char *arg){
	long ris;
	MyRequest *toAdd = malloc(sizeof(MyRequest));
	
	toAdd->type = type;

	if(arg == NULL){
		ris = 0;
		char toSend[10];
		sprintf(toSend, "%d", ris);
		toSend[strlen(toSend)] = '\0';
		printf("Argomento non specificato quindi inserisco %s\n", toSend);
		strncpy(toAdd->request_content, toSend, strlen(toSend));
	}else{
		//printf("Argomento = %s\n", arg);
		int arg_size = strlen(arg);
		char buf[arg_size + 1];
		strncpy(buf, arg, arg_size);
		//printf("Buffer = %s\n", buf);
		ris = atoi(buf);
		//printf("ris = %d\n", ris);
		if(ris < 0){
			printf("MALE!\n");
			return -1;
		}
		strncpy(toAdd->request_content, arg, strlen(arg));
	}

	//printf("Ho modificato il contenuto = %s\n", toAdd->request_content);

	if(push(&toSendRequestQueue, toAdd) == -1){
		perror("errore push!");
		exit(EXIT_FAILURE);
	}
	return 0;
}

int navigateFileSystem(char *rootPath, int n, int flag){
	if(flag && n <= 0)
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


	//printf("")

	errno = 0;
	while((actFile = readdir(currentDir)) != NULL && errno == 0){
		if(stat(actFile->d_name, &fileInfo) == -1)
			return readFiles;

		if(strcmp(actFile->d_name, ".") != 0 && strcmp(actFile->d_name, "..") != 0){
			if(S_ISDIR(fileInfo.st_mode)){
				getcwd(cWorkDir, 1024);
				readFiles += navigateFileSystem(actFile->d_name, n - readFiles, flag);
				if(chdir(cWorkDir) == -1){
					printf("Errore cambio working directory!\n");
					exit(EXIT_FAILURE);
				}
			}else{
				if((flag && readFiles < n) || !flag){
					MyRequest *r = malloc(sizeof(MyRequest));
					r->type = WRITE_FILE;
					//printf("Inserisco richiesta WRITE_FILE per il file %s\n", actFile->d_name);
					if(realpath(actFile->d_name, buf) != NULL){
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

int buildMultipleWriteRequest(int type, char* arg){
	char *dir = strtok(arg, ",");
	char buf[PATH_MAX];

	//printf("Inizio -w\n");

	if(realpath(dir, buf) == NULL){
		printf("La cartella specificata non esiste\n");
		return -1;
	}

	char *sFile = strtok(NULL, ",");
	int n, flag;

	if(sFile == NULL){
		n = 0;
		flag = 0;
	}else{
		n = atoi(sFile);
		if(n == 0)
			flag = 0;
		else
			flag = 1;
	}

	int writtenRequests = navigateFileSystem(buf, n, flag);
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
		/*printf("Ho letto %c\n", o);
		if(optarg != NULL)
			printf("Ed optarg = %s\n", optarg);
		else
			printf("Argomento non specificato\n");*/
		switch(o){
			case 'w': buildMultipleWriteRequest(WRITE_FILE, optarg);
					  break;
			case 'r': buildInsertRequest(READ_FILE, optarg);
					  break;
			case 'W': buildInsertRequest(WRITE_FILE, optarg);
					  break;
			case 'R': //printf("Ciao\n");
					  buildReadNRequest(READ_N_FILE, optarg);
					  //printf("Ciao2\n");
					  break;
			case 'c': buildInsertRequest(REMOVE_FILE, optarg);
					  break;
			case '?': printf("Invalid option(s)\n");
					  exit(EXIT_FAILURE);
					  break;
			default: break;
		}
	}
}