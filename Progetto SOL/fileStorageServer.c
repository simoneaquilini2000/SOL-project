#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<unistd.h>
#include<fcntl.h>
#include<signal.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<sys/un.h>
#include<sys/types.h>
#include "utility.h"
#include "generic_queue.h"
#include "file.h"
#include "request.h"
#include "descriptor.h"

int lfd; //listen socket file descriptor

//flag che dice se è arrivato sigHup
int sigHupFlag = 0;

//mutex per accedere ai flag sopra citati
pthread_mutex_t flagAccess = PTHREAD_MUTEX_INITIALIZER;

//struttura che memorizza parametri di config del server
static serverInfo s; 

//rappresenta il file di log 
static FILE *logFile; //da non implementare per giugno

//singola coda che rappresenta la cache(ogni singolo elemento è un MyFile)
static GenericQueue fileCache;

//Coda dove ogni elemento è una coppia <client_pid, descrittore> che rappresenta i client connessi
static GenericQueue connections;

//Coda concorrente di comunicazione dalla quale i workers prenderanno le richieste da esaudire
static GenericQueue requests;

//Mutex che i thread workers usano per accedere in ME alla coda delle richieste
static pthread_mutex_t requestsQueueMutex = PTHREAD_MUTEX_INITIALIZER;

//Var. cond. dove i thread workers attendono se la coda di richieste è vuota
static pthread_cond_t waitingForRequestsList = PTHREAD_COND_INITIALIZER;

//Mutex che i thread workers usano per accedere in ME alla cache di file
static pthread_mutex_t fileCacheMutex = PTHREAD_MUTEX_INITIALIZER;

//pipe tramite il quale gli workers rimetteranno il manager in ascolto sul descrittore usato per la comunicazione
static int p[2];

/*
	flag per regolare l'accesso dei workers alla pipe:
		celleLibere = 0 -> workers attendono che manager legga la pipe prima di scrivere il loro descrittore
		celleLibere = 1 -> worker può scrivere in ME sulla pipe
*/
static int celleLibere = 1;

//mutex per accesso alla pipe di comunicazione tra workers e manager
static pthread_mutex_t pipeAccessMutex = PTHREAD_MUTEX_INITIALIZER;

//lista di attesa dei workers che devono "restituire" il descrittore usato per la comunicazione al manager
static pthread_cond_t pList = PTHREAD_COND_INITIALIZER;

//lista di attesa di nuovi dati per il manager
static pthread_cond_t cList = PTHREAD_COND_INITIALIZER;

/*
	Aggiorno il set dei descrittori attivi in vista della prossima chiamata di select
*/
int updateActiveFds(fd_set set, int fd_num){
	int i, max = 0;

	for(i = 0; i < fd_num; i++){
		if(FD_ISSET(i, &set)){
			if(i > max)
				max = i;
		}
	}
	return max;
}

/*
	Thread manager: imposta il listen socket per l'ascolto ed
	inserisce ogni richiesta pendente nella coda delle richieste
	che verranno soddisfatte dai thread worker
*/
static void* managerThreadActivity(void* args){
	int fd_num = 0, index, l, descToAdd;
	MyRequest toAdd;
	fd_set active_fds, rdset;
	sigset_t set;
	struct sockaddr_un socket_address;
	char buf[1024];

	//sigfillset(&set);
	//pthread_sigmask(SIG_SETMASK, &set, NULL);

	//settaggio listen socket
	memset(&socket_address, 0, sizeof(socket_address));
	socket_address.sun_family = AF_UNIX;
	strncpy(socket_address.sun_path, s.socketPath, sizeof(socket_address.sun_path) - 1);
	lfd = socket(AF_UNIX, SOCK_STREAM, 0);
	unlink(socket_address.sun_path);
	if(bind(lfd, (struct sockaddr*) &socket_address, sizeof(socket_address)) == -1){
		perror("Errore bind!");
		exit(EXIT_FAILURE);
	}
	listen(lfd, SOMAXCONN);

	FD_ZERO(&active_fds); //init del set dei descrittori attivi
	FD_SET(lfd, &active_fds);
	if(lfd > fd_num) fd_num = lfd; //fd_num terrà il massimo indice di descrittore

	pthread_mutex_lock(&pipeAccessMutex);
	FD_SET(p[0], &active_fds);
	if(p[0] > fd_num)
		fd_num = p[0];
	pthread_mutex_unlock(&pipeAccessMutex);

	printf("Socket aperto per ricevere connessioni\n");
	while(1){
		rdset = active_fds;
		if(select(fd_num + 1, &rdset, NULL, NULL, NULL) == -1){
			perror("Errore select!");
			exit(EXIT_FAILURE);
		}else{
			for(index = 0; index <= fd_num; index++){
				if(FD_ISSET(index, &rdset)){ //se tale descrittore è pronto esaudisco la richiesta
					if(index == lfd){ //listen socket pronto(l'accept non si blocca)
						MyDescriptor *newConn = malloc(sizeof(MyDescriptor));
						newConn->cfd = accept(lfd, NULL, 0);
						FD_SET(newConn->cfd, &active_fds);
						if(newConn->cfd > fd_num) fd_num = newConn->cfd;
						if(push(&connections, (void *) newConn) == -1){ //inserisco nuova connessione
							perror("Errore inserimento nella coda di connessioni!");
							exit(EXIT_FAILURE);
						}
					}else{ //non sono nel listen socket ma su uno dei client già connessi o sulla pipe
						pthread_mutex_lock(&pipeAccessMutex);
						if(index == p[0]){
							//while(celleLibere == 1)
							//	pthread_cond_wait(&cList, &pipeAccessMutex);
							if((l = readn(p[0], &descToAdd, sizeof(int))) == -1){
								perror("Errore read da pipe!");
								exit(EXIT_FAILURE);
							}
							printf("Manager: Ho letto %d dalla pipe(%d bytes)\n", descToAdd, l);
							celleLibere = 1;
							FD_SET(descToAdd, &active_fds);
							if(descToAdd > fd_num) 
								fd_num = descToAdd;
							//FILE *f = fdopen(p[0], "r");
							//fflush(f);
							pthread_cond_signal(&pList);
							//fd_num = updateActiveFds(active_fds, fd_num);
							pthread_mutex_unlock(&pipeAccessMutex);
						}else{
							pthread_mutex_unlock(&pipeAccessMutex);
							//memset(&toAdd, 0, sizeof(MyRequest));
							if((l = readn(index, &toAdd, sizeof(MyRequest))) == -1){
								perror("Errore read!");
								exit(EXIT_FAILURE);
							}
							//printf("Eseguo stampa richiesta ricevuta\n");
							//requestPrint((void*)&toAdd);
							//printf("FINE stampa richiesta ricevuta\n");
							if(l == 0){ //EOF sul descrittore di indice index
								printf("Ho letto %d bytes\n", l);
								clearBuffer(buf, 1024);
								close(index);
								FD_CLR(index, &active_fds);
								fd_num = updateActiveFds(active_fds, fd_num);
								pop(&connections);
							}else{ //Inserisco in ME una nuova richiesta da soddisfare e la segnalo
								/*MyRequest badCase;
								memset(&badCase, 0, sizeof(MyRequest));
								int result = memcmp(&toAdd + 4, &badCase +4, sizeof(MyRequest) - 4);
								if(result == 0){
									printf("Malissimo!\n");
									continue;
								}*/	
								toAdd.comm_socket = index;
								//if(toAdd.type == APPEND_FILE){
								FD_CLR(index, &active_fds); //sospendo l'ascolto del manager su tale descrittore finchè il worker non avrà finito il suo lavoro
								fd_num = updateActiveFds(active_fds, fd_num);
								//}
								//printf("%d\n", index);
								//requestPrint((void*) &toAdd);
								MyRequest *req = malloc(sizeof(MyRequest));
								req = memcpy(req, &toAdd, sizeof(MyRequest)); //copio la richiesta letta all indirizzo di req
								//requestPrint((void*) req);
								//pthread_mutex_lock(&flagAccess);
								//int isSigHup = (sigHupFlag == 1);
								//pthread_mutex_unlock(&flagAccess);
								pthread_mutex_lock(&requestsQueueMutex);
								//if(!isSigHup){
									if(push(&requests, (void *) req) == -1){
										pthread_mutex_unlock(&requestsQueueMutex);
										perror("Errore push!");
										exit(EXIT_FAILURE);
									}
								//}
								//printf("Stato coda attuale:\n");
								//printQueue(requests);
								pthread_cond_signal(&waitingForRequestsList);
								pthread_mutex_unlock(&requestsQueueMutex);
								//free(&toAdd);
							}
						}
					}
				}
			}
		}
	}
}

int executeOpenFile(MyRequest r){
	pthread_mutex_lock(&fileCacheMutex);
	int risFind = findElement(fileCache, r.request_content);
	//printf("Ricerca: %d\n", risFind);
	pthread_mutex_unlock(&fileCacheMutex);

	int result;
	int l;

	if(r.flags == O_CREAT && risFind > 0)
		result = -1;
	else if(r.flags != O_CREAT && risFind < 0)
		result = -2;
	else{
		result = 0;
		if(r.flags == O_CREAT && risFind < 0){
			MyFile f;
			
			f.isOpen = 1;
			f.timestamp = time(NULL);
			f.isLocked = 0;
			f.modified = 0;
			//printf("sono qui1\n");
			strncpy(f.filePath, r.request_content, strlen(r.request_content));
			if(strlen(f.filePath) < 4096)
				f.filePath[strlen(r.request_content)] = '\0';
			//printf("Nuovo filepath = %s\n", f.filePath);
			//printf("sono qui2\n");
			f.content = malloc(25 * sizeof(char)); //dimensione base
			clearBuffer(f.content, sizeof(f.content));
			f.dim = 0;
			//filePrint((void*)&f);
			MyFile *p = malloc(sizeof(MyFile));
			memcpy(p, &f, sizeof(MyFile));
			pthread_mutex_lock(&fileCacheMutex);
			push(&fileCache, (void*)p);
			pthread_mutex_unlock(&fileCacheMutex);
			//crea un file, mettilo in coda con il flag di open = 1
		}else if(r.flags != O_CREAT && risFind > 0){
			pthread_mutex_lock(&fileCacheMutex);
			GenericNode *corr = fileCache.queue.head;
			MyFile f1;
			strncpy(f1.filePath, r.request_content, strlen(r.request_content));

			while(corr != NULL){
				if(fileCache.comparison(corr->info, (void*)&f1) == 1){
					MyFile *toOpen = (MyFile*)(corr->info);
					toOpen->isOpen = 1;
					break;
				}
				corr = corr->next;
			}
			pthread_mutex_unlock(&fileCacheMutex);
			// cerca il nodo corrispondente al file e setta il flag di open = 1
		}
	}
	l = writen(r.comm_socket, &result, sizeof(int));
	if(l == -1)
		return -1;
	return 0;
}

int executeReadFile(MyRequest r){
	pthread_mutex_lock(&fileCacheMutex);
	int risFind = findElement(fileCache, r.request_content);
	pthread_mutex_unlock(&fileCacheMutex);

	char *b;
	int l;
	int res;

	if(risFind < 0){
		res = -2;
		if(writen(r.comm_socket, &res, sizeof(res)) == -1)
			return -1;
		return 0;
	}

	pthread_mutex_lock(&fileCacheMutex);
	GenericNode *corr = fileCache.queue.head;
	MyFile f1, toRead;
	strncpy(f1.filePath, r.request_content, strlen(r.request_content));

	while(corr != NULL){
		if(fileCache.comparison(corr->info, (void*)&f1) == 1){
			toRead = *(MyFile*)(corr->info);
			if(toRead.isOpen == 0){
				res = -1;
				break;
			}
			b = malloc(toRead.dim + 1);
			strncpy(b, toRead.content, toRead.dim);
			b[toRead.dim] = '\0';
			res = 0;
			break;
		}
		corr = corr->next;
	}
	pthread_mutex_unlock(&fileCacheMutex);

	if((l = writen(r.comm_socket, &res, sizeof(int))) == -1)
		return -1;
	if(res == 0){
		int lung = strlen(b);

		if((l = writen(r.comm_socket, &lung, sizeof(int))) == -1)
			return -1;

		if((l = writen(r.comm_socket, b, lung)) == -1)
			return -1;
	}
	return 0;
}

int executeCloseFile(MyRequest r){
	pthread_mutex_lock(&fileCacheMutex);
	GenericNode *corr = fileCache.queue.head;
	pthread_mutex_unlock(&fileCacheMutex);
	MyFile f1, toRead;
	int res = -1, l;
	strncpy(f1.filePath, r.request_content, strlen(r.request_content));

	while(corr != NULL){
		pthread_mutex_lock(&fileCacheMutex);
		if(fileCache.comparison(corr->info, (void*)&f1) == 1){
			pthread_mutex_unlock(&fileCacheMutex);
			toRead = *(MyFile*)(corr->info);
			if(toRead.isOpen == 0){
				res = -2;
			}else{
				toRead.isOpen = 0;
				res  = 0;
			}
			break;
		}
		pthread_mutex_unlock(&fileCacheMutex);
		corr = corr->next;
	}
	

	if((l = writen(r.comm_socket, &res, sizeof(int))) == -1)
		return -1;
	return 0;
}

int executeRemoveFile(MyRequest r){
	pthread_mutex_lock(&fileCacheMutex);
	GenericNode *corr = fileCache.queue.head;
	pthread_mutex_unlock(&fileCacheMutex);
	MyFile f1, toDel;
	int res = -1, l;
	strncpy(f1.filePath, r.request_content, strlen(r.request_content));

	while(corr != NULL){
		pthread_mutex_lock(&fileCacheMutex);
		if(fileCache.comparison(corr->info, (void*)&f1) == 1){
			pthread_mutex_unlock(&fileCacheMutex);
			toDel = *(MyFile*)(corr->info);
			if(toDel.isLocked == 0){
				res = -2;
			}else{
				res  = 0;
				pthread_mutex_lock(&fileCacheMutex);
				deleteElement(&fileCache, (void*) &f1);
				pthread_mutex_unlock(&fileCacheMutex);
			}
			break;
		}
		pthread_mutex_unlock(&fileCacheMutex);
		corr = corr->next;
	}

	if((l = writen(r.comm_socket, &res, sizeof(int))) == -1)
		return -1;
	return 0;
}

int executeAppendFile(MyRequest r){
	MyFile f1, toRead;
	int res = 0, l;
	int buf_size;
	char *buf;
	strncpy(f1.filePath, r.request_content, strlen(r.request_content));

	printf("Inizio richiesta APPEND_FILE\n");
	pthread_mutex_lock(&fileCacheMutex);
	int isPresent = findElement(fileCache, (void*)&f1);
	pthread_mutex_unlock(&fileCacheMutex);

	if(isPresent < 0){
		res = -1;
		if((l = writen(r.comm_socket, &res, sizeof(int))) == -1){
			printf("Non buono1\n");
			return -1;
		}
		return 0;
	}

	if((l = writen(r.comm_socket, &res, sizeof(int))) == -1){
		printf("Non buono2\n");
		return -1;
	}

	if((l = readn(r.comm_socket, &buf_size, sizeof(int))) == -1){
		printf("Non buono3\n");
		return -1;
	}
	printf("Ho ricevuto buf_size = %d\n", buf_size);

	buf = malloc((buf_size + 1) * sizeof(char));
	if(buf == NULL)
		return -1;
	memset(buf, 0, buf_size + 1);

	//printf("BENE\n");

	//printf("%d\n", r.comm_socket);
	//clearBuffer(buf, sizeof(buf));
	
	printf("Ho ripulito il buffer\n");
	if((l = readn(r.comm_socket, buf, buf_size * sizeof(char))) == -1){
		printf("Non buono4\n");
		return -1;
	}
	printf("N caratteri letti = %d\n", l);
	buf[l] = '\0';

	//printf("Ciao belli %.*s\n", l, buf);
	printf("Ho ricevuto buf = %s\n", buf);
	printf("Lunghezza buffer = %d\n", strlen(buf));

	pthread_mutex_lock(&fileCacheMutex);
	GenericNode* corr = fileCache.queue.head;
	pthread_mutex_unlock(&fileCacheMutex);

	while(corr != NULL){
		pthread_mutex_lock(&fileCacheMutex);
		if(fileCache.comparison(corr->info, (void*)&f1) == 1){
			MyFile *toAppend = (MyFile*) corr->info;
			pthread_mutex_unlock(&fileCacheMutex);
			if(toAppend->isOpen == 0){
				res = -2;
			}else{
				res = 0;
				//printf("Sono qui\n");
				int act_dim = strlen(toAppend->content) + 1;
				while(toAppend->dim + l >= act_dim){
					//printf("BAB\n");
					toAppend->content = realloc(toAppend->content, 2 * act_dim * sizeof(char));
					act_dim *= 2;
					if(toAppend->content == NULL){
						perror("Errore realloc");
					}
				}
				strncpy(toAppend->content, buf, l);
				toAppend->dim += l;
				toAppend->content[toAppend->dim] = '\0';
				toAppend->modified = 1;
				filePrint((void*)toAppend);
			}
			break;
		}
		pthread_mutex_unlock(&fileCacheMutex);
		corr = corr->next;
	}	

	if((l = writen(r.comm_socket, &res, sizeof(int))) == -1){
		printf("Fine richiesta APPEND_FILE con INsuccesso\n");
		return -1;
	}
	printf("Fine richiesta APPEND_FILE con successo\n");
	return 0;
}

/*
	Funzione di esecuzione di una singola richiesta
	che dovrà distinguerne il tipo
*/
static void executeRequest(MyRequest r){
	switch(r.type){
		case OPEN_FILE:
			if(executeOpenFile(r) == -1)
				perror("Errore esecuzione richiesta OPEN_FILE");
			else{
				/*pthread_mutex_lock(&fileCacheMutex);
				printQueue(fileCache);
				pthread_mutex_unlock(&fileCacheMutex);*/
			} 
			break;
		case READ_FILE:
			if(executeReadFile(r) == -1)
				perror("Errore esecuzione richiesta READ_FILE"); 
			break;
		case CLOSE_FILE:
			if(executeCloseFile(r) == -1)
				perror("Errore esecuzione richiesta CLOSE_FILE"); 
			break;
		case REMOVE_FILE:
			if(executeRemoveFile(r) == -1)
				perror("Errore esecuzione richiesta REMOVE_FILE"); 
			break;
		case APPEND_FILE:
			if(executeAppendFile(r) == -1)
				perror("Errore esecuzione richiesta APPEND_FILE");
			else{
				/*pthread_mutex_lock(&fileCacheMutex);
				printQueue(fileCache);
				pthread_mutex_unlock(&fileCacheMutex);*/
			}
			break;
		case READ_N_FILE: break;
		default: return;	
	}
}

void commSocketGiveBack(int comm_socket){
	int l;
	pthread_mutex_lock(&pipeAccessMutex);

	while(celleLibere == 0)
		pthread_cond_wait(&pList, &pipeAccessMutex);
	celleLibere = 0;
	if((l = write(p[1], &comm_socket, sizeof(int))) == -1){
		perror("Errore write!");
		exit(EXIT_FAILURE);
	}
	printf("Worker: Ho scritto %d nella pipe(%d bytes)\n", comm_socket, l);
	//pthread_cond_signal(&cList);
	pthread_mutex_unlock(&pipeAccessMutex);
}

/*
	Funzione che rappresenta l'attività di un singolo
	worker thread
*/
static void* workerThreadActivity(void* args){
	int tid = *(int*) args;
	sigset_t set;
	MyRequest r; //struttura dove memorizzare la richiesta estratta dalla coda

	//printf("Sono worker %d\n", tid);

	//sigfillset(&set);
	//pthread_sigmask(SIG_SETMASK, &set, NULL);

	while(1){
		pthread_mutex_lock(&requestsQueueMutex); //accesso in ME
		//printf("La coda è vuota? %d\n", isEmpty(requests));
		while(isEmpty(requests) == 1){ //coda vuota
			//printf("ATTENDO\n");
			pthread_cond_wait(&waitingForRequestsList, &requestsQueueMutex);
		}
		//printf("VADO AD ESTRARRE RICHIESTA\n");
		//r = *(MyRequest*) pop(&requests); //la coda non è vuota quindi sicuramente r != NULL
		//printf("faccio POP\n");
		//printf("Sono worker %d faccio POP\n", tid);
		//printf("Stampo coda PRIMA di pop di worker %d:\n", tid);
		//printQueue(requests);
		MyRequest *p = (MyRequest*) pop(&requests);
		//printf("Stampo coda DOPO pop di worker %d:\n", tid);
		//printQueue(requests);
		pthread_mutex_unlock(&requestsQueueMutex);

		//printf("IsNull = %d\n", (p == NULL));
		if(p == NULL || p == 0)
			printf("Coda vuota, attendi\n");
		else{
			printf("Worker: stampo richiesta estratta dalla coda\n");
			requestPrint((void*) p);
			executeRequest(*p);
			commSocketGiveBack(p->comm_socket);
		}
		
		printf("fine POP\n");
		/*
		printf("Stampo richiesta estratta\n");
		//requestPrint((void*) &r);
		//requestPrint((void*) p);
		printf("Stampo resto della coda:\n");
		pthread_mutex_lock(&requestsQueueMutex);
		printQueue(requests);
		pthread_mutex_unlock(&requestsQueueMutex);*/
		
		//executeRequest(*p);
	}
}

int main(int argc, char const *argv[]){
	int i;
	pthread_t manage_tid;
	pthread_t *workers;

	if(argc < 2){
		perror("Path del socket deve essere passato da linea di comando!");
		exit(EXIT_FAILURE);
	}

	s = startConfig(argv[1]); //parsing del file di config
	workers = malloc(s.nWorkers * sizeof(pthread_t)); //alloco i TID per i worker threads
	if(workers == NULL){
		perror("Errore malloc!");
		exit(EXIT_FAILURE);
	}
	//creo coda di descrittori che rappresentano le connessioni attuali
	connections = createQueue(&descriptorComparison, &descriptorPrint); 
	fileCache = createQueue(&fileComparison, &filePrint);  //creo cache di file
	requests = createQueue(&requestComparison, &requestPrint);

	if(pipe(p) < 0){
		perror("Errore creazione pipe\n");
		exit(EXIT_FAILURE);
	}

	//creo manager thread
	if(pthread_create(&manage_tid, NULL, &managerThreadActivity, NULL) == -1){ 
		perror("Errore creazione thread manager!");
		exit(EXIT_FAILURE);
	}

	//creo worker threads
	for(i = 0; i < s.nWorkers; i++){
		if(pthread_create(&workers[i], NULL, &workerThreadActivity, (void*) &i) == -1){ 
			perror("Errore creazione thread worker!");
			exit(EXIT_FAILURE);
		}
	}

	//sleep(5);
	//printf("Sto per inviare segnale");
	//kill(getpid(), SIGHUP);
	//join di tutti i threads
	for(i = 0; i < s.nWorkers; i++)
			pthread_join(workers[i], NULL);
	pthread_join(manage_tid, NULL);
	return 0;
}