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

GenericQueue toSendRequestQueue; //coda delle richieste da spedire al client

/*
	Stampo tutte le opzioni disponibili
*/
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
	printf("\t-o file1,[,file2]\n"); //aggiuntiva
	printf("\t-i dirName[,n]\n"); //aggiuntiva
	printf("\t-C file1,[,file2]\n"); //aggiuntiva
	printf("\t-p\n");
}

ClientConfigInfo getConfigInfoFromCmd(int argc, char* const* argv){
	char o;
	ClientConfigInfo conf;
	int foundT = 0;
	int foundF = 0;
	int foundD = 0;
	int foundr = 0;
	int foundR = 0;

	memset(&conf, 0, sizeof(conf));


	conf.printEnable = 0;
	optind = 1; //resetto l'indice di getopt in modo da poter scandire le opzioni più volte

	while((o = getopt(argc, argv, ALL_OPTIONS)) != -1){
		switch(o){
			case 't': 
				//per l'opzione t prendo solo l'ultima occorrenza
				if((conf.requestInterval = (int) isNumber(optarg)) < 0){
					perror("Wrong argument format for option -t");
					exit(EXIT_FAILURE);
				}
				foundT = 1;
				break;
			case 'f': 
				// non posso ripetere l'opzione f, se ciò accade la richiesta è malformata
				if(foundF){
					perror("Can not repeat option -f");
					exit(EXIT_FAILURE);
				}
				foundF = 1;
				strcpy(conf.socketName, optarg);
				break;

			case 'p':
				//non posso ripetere l'opzione -p, quindi vale quanto detto per -f 
				if(conf.printEnable){
					perror("Can not repeat option -p");
					exit(EXIT_FAILURE);
				}
				conf.printEnable = 1;
				break;
			case 'd': 
				if(foundD) //se ho più volte l'opzione -d prendo solo l'ultima occorrenza
					clearBuffer(conf.saveReadFileDir, strlen(conf.saveReadFileDir));
				foundD = 1;
				strcpy(conf.saveReadFileDir, optarg);
				break;
			case 'R': 
				foundR = 1; //cercare qua -r e -R servono solo per verificare la correttezza della richiesta
				break;
			case 'r': 
				foundr = 1;
				break;
			case '?': 
				//opzione invalida, richiesta malformata
				perror("Invalid option(s)");
				exit(EXIT_FAILURE);
				break;
			default: break; //ignoro opzioni non di configurazione
		}
	}

	if(foundT == 0)
		conf.requestInterval = 0; //non ho intervallo tra una richiesta e l'altra
	if(foundD == 0)
		clearBuffer(conf.saveReadFileDir, 1024); //file letti non verrano salvati
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
	/*
		In quanto anche la getAbsPathFromRelPath tokenizza una stringa
		(in tal caso usa '/' come separatore) è necessario salvarsi
		lo stato della stringa arg da spezzettare in modo da evitare
		effetti indesiderati 
	*/
	char *tokenBackup; //tiene lo stato di tokenizzazione di arg
	char *token = strtok_r(arg, ",", &tokenBackup); //token corrente di arg
	char buf[PATH_MAX];
	int ris = 0;

	while(token != NULL){
		MyRequest *r = malloc(sizeof(MyRequest)); //creo richiesta
		if(r == NULL){ //errore malloc, memoria inconsistente quindi errore fatale 
			perror("Malloc error!\n");
			exit(EXIT_FAILURE);
		}
		memset(r, 0, sizeof(MyRequest));
		r->type = type;
		r->flags = request_flags;
		memset(buf, 0, sizeof(buf));

		/*
			Se devo scrivere un file, in quanto sarà stato lo stesso client
			a crearlo ed aprirlo; il contenuto da scrivere dovrà
			essere presente nel file system del client(ciò verrà gestito tramite l'API).
			In caso contrario, ciò non è strettamente necessario
			quindi va semplicemente tradotto il path relativo
			in assoluto.
		*/
		getAbsPathFromRelPath(token, buf, PATH_MAX);
		
		if(strcmp(buf, "") == 0){ //errore nella traduzione da path relativo ad assoluto
			ris = -1;
		}

		if(ris == 0){ //inserisco richieste solo se non ci sono stati errori in traduzione
			strncpy(r->request_content, buf, strlen(buf));
			if(push(&toSendRequestQueue, (void*)r) == -1){ //errore fatale nella push
				perror("push error!");
				exit(EXIT_FAILURE);
			}
		}else{
			free(r); //se la richiesta non è stata inserita, libero la struttura per evitare leaks
		}
		
		ris = 0;
		token = strtok_r(NULL, ",", &tokenBackup); //prendo prossimo token
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
		//request_content conterrà il numero di file da leggere(0 se l'argomento non è specificato)
		strncpy(toAdd->request_content, toSend, strlen(toSend));
	}else{
		int arg_size = strlen(arg);
		char buf[arg_size + 1];
		strncpy(buf, arg, arg_size); //copio l'argomento specificato
		strncpy(toAdd->request_content, arg, strlen(arg));
	}
	toAdd->flags = request_flags; //aggiungo eventuali flags

	if(push(&toSendRequestQueue, toAdd) == -1){ //errore fatale nella push
		perror("errore push!");
		exit(EXIT_FAILURE);
	}
	return 0;
}

int navigateFileSystem(char *rootPath, int n, int flag, int request_type, int request_flags){
	if(flag && n <= 0) //flag usato per indicare se c'è una quantità massima di file da leggere(1) o meno(0)
		return 0;

	DIR *currentDir; //directory corrente
	struct dirent *actFile;
	struct stat fileInfo;
	int readFiles = 0; //file letti dalla directory corrente
	char buf[PATH_MAX]; //tengo il path assoluto del file oggetto della richiesta da inserire
	char cWorkDir[1024]; //mantengo cwd per eventuali scansioni ricorsive

	currentDir = opendir(rootPath);

	//in caso di errori in apertura o cambio di cwd ritorno 0 file letti
	if(currentDir == NULL)
		return 0;

	if(chdir(rootPath) == -1)
		return 0;

	errno = 0;
	while((actFile = readdir(currentDir)) != NULL && errno == 0){
		if(stat(actFile->d_name, &fileInfo) == -1) //ottengo info sul file corrente
			return readFiles;

		if(strcmp(actFile->d_name, ".") != 0 && strcmp(actFile->d_name, "..") != 0){
			if(S_ISDIR(fileInfo.st_mode)){ //se trovo una cartella navigo il FS ricorsivamente
				getcwd(cWorkDir, 1024);
				readFiles += navigateFileSystem(actFile->d_name, n - readFiles, flag, request_type, request_flags);
				if(chdir(cWorkDir) == -1){ //se non riesco a tornare alla cwd ho errore fatale
					printf("Errore cambio working directory!\n");
					exit(EXIT_FAILURE);
				}
			}else{
				//se non ho raggiunto il limite oppure se il limite non c'era,
				// aggiungo richiesta riguardante il file specificato
				if((flag && readFiles < n) || !flag){
					MyRequest *r = malloc(sizeof(MyRequest));
					memset(r, 0, sizeof(MyRequest));
					r->type = request_type;
					r->flags = request_flags;
					if(realpath(actFile->d_name, buf) != NULL){
						//per costruire richiesta ottengo path assoluto dei file
						strncpy(r->request_content, buf, strlen(buf));
						if(push(&toSendRequestQueue, (void*)r) == -1){ //errore fatale nella push
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

	closedir(currentDir); //chiudo directory corrente
	return readFiles; //riorno numero di file letti dalla cwd
}

int buildMultipleWriteRequest(int type, char* arg, int request_flags){
	char *dir = strtok(arg, ","); //ottengo cartella da cui partire per mandare richieste di WRITE_FILE
	char buf[PATH_MAX];
	char currDir[PATH_MAX];

	/*
		La cartella specificata deve esistere
		altrimenti la richiesta risulterà scorretta
	*/
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
			flag = 0; //non ho limite nel numero di file da leggere
		else
			flag = 1; //ho un limite al numero di file da leggere se n != 0
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

void getToSendRequestsFromCmd(int argc, char* const* argv){
	char o; //tiene l'opzione corrente

	optind = 1; //resetto l'indice di getopt per eseguire correttamente la scansione
	while((o = getopt(argc, argv, ALL_OPTIONS)) != -1){
		switch(o){
			case 'w': buildMultipleWriteRequest(WRITE_FILE, optarg, 0);
					  break;
			case 'r': buildInsertRequest(READ_FILE, optarg, 0);
					  break;
			case 'W': buildInsertRequest(WRITE_FILE, optarg, 0);
					  break;
			case 'R': 
					  buildReadNRequest(READ_N_FILE, optarg, 0);
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