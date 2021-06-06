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
#include "serverInfo.h"

static int lfd; //listen socket file descriptor

//mutex per accedere in ME al listen socket(per chiuderlo, ad esempio)
static pthread_mutex_t lSocketMutex = PTHREAD_MUTEX_INITIALIZER;

//flag che dice se è arrivato sigHup
static int sigHupFlag = 0;

//flag che dice se è arrivato SIGINT o SIGQUIT
static int sigIntOrQuitFlag = 0;

//pipe con cui il signal handler dirà al manager di terminare la sua attività
static int signalPipe[2];

//mutex per accesso in ME alla signalPipe
static pthread_mutex_t signalPipeMutex = PTHREAD_MUTEX_INITIALIZER;

//struttura che memorizza parametri di config del server
static serverInfo s;

//struttura contenente informazioni statistiche riguardo la fileCache aggiornata
static serverStats serverStatistics;

//mutex di accesso in ME per aggiornamento delle statistiche del server
static pthread_mutex_t updateStatsMutex = PTHREAD_MUTEX_INITIALIZER;

//singola coda che rappresenta la cache(ogni singolo elemento è un MyFile)
static GenericQueue fileCache;

//Coda dove ogni elemento è un descrittore che rappresenta i client connessi
static GenericQueue connections;

//set dei descrittori attualmente attivi
static fd_set active_fds;

//mutex per accedere in ME alla coda delle connessioni ed al set dei descrittori
static pthread_mutex_t connectionsQueueMutex = PTHREAD_MUTEX_INITIALIZER;

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

/*
	Funzione che esegue la pulizia delle strutture usate(
	coda delle richieste, dei descrittori e dei file).
	Non ha bisogno della ME in quanto verrà eseguita solo
	dopo che i workers, il manager ed il signal handler
	thread saranno terminati
*/
void freeStructures(){
	close(p[0]);
	close(p[1]);
	close(signalPipe[0]);
	close(signalPipe[1]);
	freeQueue(&requests);
	freeQueue(&connections);
	freeQueue(&fileCache);
}

/*
	Funzione eseguita dal thread gestore dei segnali:
		-Previo mascheramento di SIGINT,SIGHUP e SIGQUIT,
		si mette in attesa di tali segnali con sigwait.
		- Una volta ricevuto uno di questi segnali, ne 
		scrive il codice numerico corrispondente in una pipe
		ascoltata dal manager che prenderà le dovute
		contromisure.
*/
static void* signalHandlerActivity(void* args){
	int sig_num;
	int l;
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGHUP);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGINT);
	pthread_sigmask(SIG_SETMASK, &set, NULL);
	sigwait(&set, &sig_num);

	//printf("Signal Handler: Ho ricevuto segnale %d\n", sig_num);

	pthread_mutex_lock(&signalPipeMutex);
	if((l = writen(signalPipe[1], &sig_num, sizeof(int))) == -1){
		perror("Errore scrittura su pipe!\n");
		exit(EXIT_FAILURE);
	}
	pthread_mutex_unlock(&signalPipeMutex);
	printf("Signal Handler: ho terminato la mia attività\n");
	return (void*)NULL;
}

/*
	Aggiorno il set dei descrittori attivi in vista della prossima chiamata di select,
	restituendone il massimo
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
	int fd_num = 0, index, l, descToAdd, sig_num;
	MyRequest toAdd, *req;
	MyDescriptor newConnDesc;
	fd_set rdset;
	sigset_t set;
	struct sockaddr_un socket_address;
	char buf[1024];

	//sigfillset(&set);
	//pthread_sigmask(SIG_SETMASK, &set, NULL);

	//settaggio listen socket
	memset(&socket_address, 0, sizeof(socket_address));
	socket_address.sun_family = AF_UNIX;
	strncpy(socket_address.sun_path, s.socketPath, sizeof(socket_address.sun_path) - 1);
	pthread_mutex_lock(&lSocketMutex);
	lfd = socket(AF_UNIX, SOCK_STREAM, 0);
	unlink(socket_address.sun_path);
	if(bind(lfd, (struct sockaddr*) &socket_address, sizeof(socket_address)) == -1){
		perror("Errore bind!");
		exit(EXIT_FAILURE);
	}
	listen(lfd, SOMAXCONN);

	int active = 1;

	pthread_mutex_lock(&connectionsQueueMutex);
	FD_ZERO(&active_fds); //init del set dei descrittori attivi
	FD_SET(lfd, &active_fds);
	if(lfd > fd_num) fd_num = lfd; //fd_num terrà il massimo indice di descrittoreù
	pthread_mutex_unlock(&connectionsQueueMutex);
	pthread_mutex_unlock(&lSocketMutex);

	pthread_mutex_lock(&pipeAccessMutex);
	pthread_mutex_lock(&connectionsQueueMutex);
	FD_SET(p[0], &active_fds); //ascolto pipe di comunicazione da worker(s) verso il manager
	if(p[0] > fd_num)
		fd_num = p[0];
	pthread_mutex_unlock(&connectionsQueueMutex);
	pthread_mutex_unlock(&pipeAccessMutex);

	pthread_mutex_lock(&signalPipeMutex);
	pthread_mutex_lock(&connectionsQueueMutex);
	FD_SET(signalPipe[0], &active_fds); //ascolto pipe di comunicazione da worker(s) verso il manager
	if(signalPipe[0] > fd_num)
		fd_num = signalPipe[0];
	pthread_mutex_unlock(&connectionsQueueMutex);
	pthread_mutex_unlock(&signalPipeMutex);

	printf("Socket aperto per ricevere connessioni\n");
	while(active){
		pthread_mutex_lock(&connectionsQueueMutex);
		rdset = active_fds;
		pthread_mutex_unlock(&connectionsQueueMutex);
		if(select(fd_num + 1, &rdset, NULL, NULL, NULL) == -1){
			perror("Errore select!");
			exit(EXIT_FAILURE);
		}else{
			//pthread_mutex_lock(&connectionsQueueMutex);
			for(index = 0; index <= fd_num; index++){
				//pthread_mutex_unlock(&connectionsQueueMutex);
				if(FD_ISSET(index, &rdset)){ //se tale descrittore è pronto esaudisco la richiesta
					if(index == lfd){ //listen socket pronto(l'accept non si blocca)
						//memset(&newConn, 0, sizeof(newConn));
						
						MyDescriptor *newConn = (MyDescriptor*) malloc(sizeof(MyDescriptor));
						memset(newConn, 0, sizeof(*newConn));
						memset(&newConnDesc, 0, sizeof(newConnDesc));
						newConnDesc.cfd = accept(lfd, NULL, 0);
						pthread_mutex_lock(&connectionsQueueMutex);
						FD_SET(newConnDesc.cfd, &active_fds);
						if(newConnDesc.cfd > fd_num) fd_num = newConnDesc.cfd;
						memcpy(newConn, &newConnDesc, sizeof(newConnDesc));
						int pushRes = push(&connections, (void *) newConn);
						pthread_mutex_unlock(&connectionsQueueMutex);
						if(pushRes == -1){ //se l'inserimento fallisce ho errore fatale
							free(newConn);
							perror("Errore inserimento nella coda di connessioni!");
							exit(EXIT_FAILURE);
						}
						//free(newConn);
					}else{ //non sono nel listen socket ma su uno dei client già connessi o sulle pipe
						pthread_mutex_lock(&signalPipeMutex);
						if(index == signalPipe[0]){
							if((l = readn(signalPipe[0], &sig_num, sizeof(int))) == -1){
								perror("Errore read da pipe!");
								exit(EXIT_FAILURE);
							}
							pthread_mutex_lock(&connectionsQueueMutex);
							FD_CLR(signalPipe[0], &active_fds);
							fd_num = updateActiveFds(active_fds, fd_num + 1);
							pthread_mutex_unlock(&connectionsQueueMutex);
							pthread_mutex_unlock(&signalPipeMutex);
							//printf("Manager: Ho ricevuto segnale %d\n", sig_num);
							if(sig_num == SIGINT || sig_num == SIGQUIT){
								//pthread_mutex_lock(&flagAccessMutex);
								active = 0;
								//pthread_mutex_unlock(&flagAccessMutex);
								pthread_mutex_lock(&requestsQueueMutex);
								sigIntOrQuitFlag = 1;
								pthread_cond_broadcast(&waitingForRequestsList);
								pthread_mutex_unlock(&requestsQueueMutex);
								break;
							}else if(sig_num == SIGHUP){
								//printf("Sono in ramo SIGHUP\n");
								pthread_mutex_lock(&connectionsQueueMutex);
								sigHupFlag = 1;
								close(lfd);
								FD_CLR(lfd, &active_fds);
								fd_num = updateActiveFds(active_fds, fd_num + 1);
								if(isEmpty(connections) == 1){
									//printf("BA\n");
									pthread_mutex_lock(&requestsQueueMutex);
									sigIntOrQuitFlag = 1;
									pthread_cond_broadcast(&waitingForRequestsList);
									pthread_mutex_unlock(&requestsQueueMutex);
									pthread_mutex_unlock(&connectionsQueueMutex);
									active = 0;
									break;
								}
								//printf("BO\n");
								pthread_mutex_unlock(&connectionsQueueMutex);
							}
						}else{
							pthread_mutex_unlock(&signalPipeMutex);
							pthread_mutex_lock(&pipeAccessMutex);
							if(index == p[0]){ //leggo dalla pipe
								if((l = readn(p[0], &descToAdd, sizeof(int))) == -1){
									perror("Errore read da pipe!");
									exit(EXIT_FAILURE);
								}
								celleLibere = 1; //abilito un worker in attesa a scrivere nella pipe
								pthread_mutex_lock(&connectionsQueueMutex);
								FD_SET(descToAdd, &active_fds); //riascolto il descrittore ricevuto dal worker
								if(descToAdd > fd_num) 
									fd_num = descToAdd;
								pthread_mutex_unlock(&connectionsQueueMutex);
								pthread_cond_signal(&pList);
								pthread_mutex_unlock(&pipeAccessMutex);
							}else{
								pthread_mutex_unlock(&pipeAccessMutex);
								memset(&toAdd, 0, sizeof(MyRequest)); //azzero struttura di ricezione richiesta
								if((l = readn(index, &toAdd, sizeof(MyRequest))) == -1){//leggo richiesta
									perror("Errore read!");
									exit(EXIT_FAILURE);
								}
								if(l == 0){ //EOF sul descrittore di indice index
									printf("Terminazione connessione\n");
									//clearBuffer(buf, 1024);
									close(index);
									pthread_mutex_lock(&connectionsQueueMutex);
									FD_CLR(index, &active_fds);
									fd_num = updateActiveFds(active_fds, fd_num + 1);
									//printf("Prima di terminare connessione:\n");
									//printQueue(connections);
									MyDescriptor d;
									d.cfd = index;
									deleteElement(&connections, (void*) &d); //elimino descrittore corrispondente dalla coda di connessioni
									//printf("Dopo termine connessione:\n");
									//printQueue(connections);
									//printf("BAB\n");
									if(isEmpty(connections) && sigHupFlag == 1){ //c'è SIGHUP e ho terminato di servire tutte le connessioni
										//printf("BUB\n");
										active = 0;
										pthread_mutex_lock(&requestsQueueMutex);
										sigIntOrQuitFlag = 1; //mi riconduco al caso di SIGINT in modo da terminare immediatamente
										pthread_cond_broadcast(&waitingForRequestsList);
										pthread_mutex_unlock(&requestsQueueMutex);
										pthread_mutex_unlock(&connectionsQueueMutex);
										break;
									}
									pthread_mutex_unlock(&connectionsQueueMutex);
								}else{ //Inserisco in ME una nuova richiesta da soddisfare e la segnalo
									toAdd.comm_socket = index;
									pthread_mutex_lock(&connectionsQueueMutex);
									FD_CLR(index, &active_fds); //sospendo l'ascolto del manager su tale descrittore finchè il worker non avrà finito il suo lavoro
									fd_num = updateActiveFds(active_fds, fd_num + 1);
									pthread_mutex_unlock(&connectionsQueueMutex);
									req = malloc(sizeof(MyRequest));
									memset(req, 0, sizeof(MyRequest));
									memcpy(req, &toAdd, sizeof(toAdd)); //copio la richiesta letta all indirizzo di req
									pthread_mutex_lock(&requestsQueueMutex);
									if(push(&requests, (void *) req) == -1){
										free(req);
										pthread_mutex_unlock(&requestsQueueMutex);
										perror("Errore push!");
										exit(EXIT_FAILURE);
									}
									pthread_cond_signal(&waitingForRequestsList);
									pthread_mutex_unlock(&requestsQueueMutex);
								}
							}
						}
					}
				}
				//pthread_mutex_lock(&connectionsQueueMutex);
			}
			//pthread_mutex_unlock(&connectionsQueueMutex);
		}
	}
	printf("Sono manager ho finito la mia attività\n");
	return (void*)NULL;
}

int verifyReplaceCondition(RequestType r, int data_amount){
	if(r == OPEN_FILE){
		/*
			modifica la cache size quindi ho rimpiazzamento se cacheSize + 1 > maxCacheSize
		*/
		pthread_mutex_lock(&fileCacheMutex);
		int actQueueSize = fileCache.size;
		pthread_mutex_unlock(&fileCacheMutex);
		pthread_mutex_lock(&updateStatsMutex);
		int condition = (actQueueSize + 1) > s.nMaxFile;
		pthread_mutex_unlock(&updateStatsMutex);

		if(condition == 1)
			return 1;
		return 0;
	}

	if(r == APPEND_FILE || r == WRITE_FILE){
		/*
			modifico la cache storage size quindi ho rimpiazzamento se
			actStorageSize + buf_size > maxCacheStorageSpace
		*/
		pthread_mutex_lock(&updateStatsMutex);
		int condition = serverStatistics.fileCacheActStorageSize + data_amount > s.maxStorageSpace;
		pthread_mutex_unlock(&updateStatsMutex);

		if(condition == 1)
			return 1;
		return 0;
	}
	return -1; //tipo di richiesta sbagliato
}

/*
	Verifica che ci siano le condizioni per eliminare file dalla cache
	e permettere operazioni di creazione, scrittura od estensione(
	verrano considerate solo richieste di tipo OPEN_FILE,
	WRITE_FILE ed APPEND_FILE in quanto le altre non prevedono
	l'incremento né della size né della storage size attuale della
	cache).
	Ritorna 1 se ci sono, 0 altrimenti(-1 se la funzione
	non è definita per il tipo di richiesta passatogli).
*/
int verifyReplacePossibility(RequestType t, char requestTarget[], int dataAmount){
	if(t == OPEN_FILE){
		/*
			Dato che questa richiesta modifica solo la size della cache,
			condizione necessaria e sufficiente per permettere l'eventuale creazione di un file
			è che ci sia almeno un file vittima che sia stato modificato(flag modified = 1)
		*/
		pthread_mutex_lock(&fileCacheMutex);
		GenericNode *corr = fileCache.queue.head;

		while(corr != NULL){
			MyFile *f = (MyFile*) corr->info;
			if(f->modified == 1){
				pthread_mutex_unlock(&fileCacheMutex);
				return 1;
			}
			corr = corr->next;
		}
		pthread_mutex_unlock(&fileCacheMutex);
		return 0;
	}
	
	if(t == WRITE_FILE || t == APPEND_FILE){
		/*
			Queste due richieste incrementano la actStorageSize della cache quindi
			condizione necessaria e sufficiente affinchè sia possibile scrivere o
			appendere un file è che actStorageSize + buf_size - SUM({f_i | f_i.modified == 1}) <= s.maxStorageSpace
			cioè che se incremento di buf_size la storage size attuale riesco a trovare
			una serie di file(precedentemente modificati) t.c. la loro eliminazione mi libera spazio
			sufficiente per la scrittura. Ovviamente in tale serie non ci deve essere il file su cui sto operando
		*/
		pthread_mutex_lock(&updateStatsMutex);
		int actCacheStorageSize = serverStatistics.fileCacheActStorageSize;
		int maxCacheStorageSize = s.maxStorageSpace;
		pthread_mutex_unlock(&updateStatsMutex);
		int victimFilesDimSum = 0;
		pthread_mutex_lock(&fileCacheMutex);
		GenericNode *corr = fileCache.queue.head;

		while(corr != NULL){
			MyFile *f = (MyFile*) corr->info;
			if(f->modified == 1 && strcmp(f->filePath, requestTarget) != 0)
				victimFilesDimSum += f->dim;
			corr = corr->next;
		}
		pthread_mutex_unlock(&fileCacheMutex);
		if(actCacheStorageSize + dataAmount - victimFilesDimSum <= maxCacheStorageSize)
			return 1;
		return 0;

	}
	return -1; //tipo di richiesta sbagliato
}

/*
	Algoritmo di rimpiazzamento(politica FIFO):
	-ritorna 1 -> non ci sono condizioni per il rimpiazzo
	-ritorna 0 -> rimpiazzamento effettuato con successo

	Nel peggiore dei casi si arresta quando ho ripulito
	tutta la cache, se è verificata la condizione che
	NumeroFileInCache > nMaxFileInCache oppure 
	actStorageSize > maxStorageSize
*/
int replacingAlgorithm(){
	pthread_mutex_lock(&fileCacheMutex);
	int actQueueSize = fileCache.size;
	//GenericQueue fileCacheBackup = fileCache;
	pthread_mutex_unlock(&fileCacheMutex);
	pthread_mutex_lock(&updateStatsMutex);
	int replaceCondition = (serverStatistics.fileCacheActStorageSize > s.maxStorageSpace || actQueueSize > s.nMaxFile);
	pthread_mutex_unlock(&updateStatsMutex);

	//verifico condizioni: se non ci sono le condizioni del rimpiazzo ritorno 1
	if(replaceCondition == 0){
		return 1;
	}
	//aumentare numero di volte in cui questo è stato invocato
	pthread_mutex_lock(&updateStatsMutex);
	serverStatistics.replaceAlgInvokeTimes++;
	pthread_mutex_unlock(&updateStatsMutex);

	/*
		finchè head != NULL && (size > maxSize || storageSize > maxStorageSize):
			- pop di file, se questo è stato modificato
			- decremento size e storageSize
			- freeFile
	*/
	/*pthread_mutex_lock(&fileCacheMutex);
	int isFcEmpty = isEmpty(fileCache);
	pthread_mutex_unlock(&fileCacheMutex);*/

	//al massimo scorro tutta la lista quind avrò indice i < sizeInizialeCache
	int initCacheSize = actQueueSize;
	int i = 0;

	pthread_mutex_lock(&fileCacheMutex);
	GenericNode *corr = fileCache.queue.head;
	GenericNode *corrBackup;

	while(i < initCacheSize && replaceCondition == 1){
		corrBackup = corr->next;
		MyFile *toDel = (MyFile*) corr->info;
		if(toDel == NULL){
			pthread_mutex_unlock(&fileCacheMutex);
			return -1;
		}
		if(toDel->modified == 1){
			printf("Ho selezionato come file vittima: %s\n", toDel->filePath);
			pthread_mutex_lock(&updateStatsMutex);
			serverStatistics.fileCacheActStorageSize -= toDel->dim;
			replaceCondition = (serverStatistics.fileCacheActStorageSize > s.maxStorageSpace || actQueueSize - 1 > s.nMaxFile);
			pthread_mutex_unlock(&updateStatsMutex);
			deleteElement(&fileCache, (void*) toDel);
			actQueueSize = fileCache.size ;
			//pthread_mutex_unlock(&fileCacheMutex);
		}
		corr = corrBackup;
		i++;
		/*pthread_mutex_lock(&fileCacheMutex);
		//MyFile *toDel = (MyFile*) pop(&fileCache);
		//isFcEmpty = isEmpty(fileCache);
		actQueueSize = fileCache.size;
		pthread_mutex_unlock(&fileCacheMutex);
		pthread_mutex_lock(&updateStatsMutex);
		serverStatistics.fileCacheActStorageSize -= toDel->dim;
		replaceCondition = (serverStatistics.fileCacheActStorageSize > s.maxStorageSpace || actQueueSize > s.nMaxFile);
		pthread_mutex_unlock(&updateStatsMutex);
		freeFile((void*) toDel);*/
	}
	pthread_mutex_unlock(&fileCacheMutex);

	//aggiorno eventuali max size o max storage size
	pthread_mutex_lock(&updateStatsMutex);
	if(actQueueSize > serverStatistics.fileCacheMaxSize)
		serverStatistics.fileCacheMaxSize = actQueueSize;
	if(serverStatistics.fileCacheActStorageSize > serverStatistics.fileCacheMaxStorageSize)
		 serverStatistics.fileCacheMaxStorageSize = serverStatistics.fileCacheActStorageSize;
	pthread_mutex_unlock(&updateStatsMutex);

	return 0;
}

/*
	Richiesta open ritorna:
	0 -> successo
	-1 -> errori di read/write
	
	2 fasi: ricerca preliminare del file(per verificarne la presenza in cache)
	ed azione conseguente in base ai flag della richiesta
*/
int executeOpenFile(MyRequest r){
	pthread_mutex_lock(&fileCacheMutex);
	int risFind = findElement(fileCache, r.request_content);
	pthread_mutex_unlock(&fileCacheMutex);

	int result;
	int replaceResult, replacePossibility, replaceCond;
	int l;

	if(r.flags == O_CREAT && risFind > 0)
		result = -1;
	else if(r.flags != O_CREAT && risFind < 0)
		result = -2;
	else{
		result = 0;
		if(r.flags == O_CREAT && risFind < 0){ //crea un file, mettilo in coda con il flag di open = 1
			MyFile f;
			
			//data_amount = 0 perchè sono in una richiesta di tipo OPEN_FILE
			replacePossibility = verifyReplacePossibility(r.type, r.request_content, 0);
			replaceCond = verifyReplaceCondition(r.type, 0);

			if(replaceCond == 1 && replacePossibility == 0){
				result = -4;
			}else{
				MyFile *p = malloc(sizeof(MyFile));
				memset(p, 0, sizeof(MyFile));
				p->isOpen = 1;
				p->timestamp = time(NULL);
				p->isLocked = 0;
				p->modified = 0;
				strncpy(p->filePath, r.request_content, strlen(r.request_content));
				p->filePath[strlen(r.request_content)] = '\0';
				p->content = NULL;
				//clearBuffer(p->content, 25);
				//memset(p->content, 0, 25 * sizeof(char));
				p->dim = 0;
				/*
					Aggiorno già l'ultima operazione effettuata con successo
					in quanto se la push fallisse vorrebbe dire che ho un problema
					legato alla memoria, il quale causerebbe un errore fatale
				*/
				p->lastSucceedOp.opType = r.type;
				p->lastSucceedOp.optFlags = r.flags;
				p->lastSucceedOp.clientDescriptor = r.comm_socket;
				
				//memcpy(p, &f, sizeof(MyFile));
				pthread_mutex_lock(&fileCacheMutex);
				if(push(&fileCache, (void*)p) == -1){
					perror("Errore push!\n");
					exit(EXIT_FAILURE);
				}
				//printf("Ho aggiunto file: stampo la nuova cache\n");
				//printQueue(fileCache);
				pthread_mutex_unlock(&fileCacheMutex);
				replaceResult = replacingAlgorithm();
				if(replaceResult == -1){
					perror("Nell'esecuzione dell'algoritmo di \
					rimpiazzamento si è verificata una violazione \
					dell'integrità della cache!\n"); //problema in ME ha creato inconsistenza nella cache
					exit(EXIT_FAILURE); //errore fatale
				}
				printf("Ho creato nuovo file: %s\n", p->filePath);
				printf("Esito algoritmo di rimpiazzamento: %d\n\n", replaceResult);
				if(replaceResult == 1){ //dopo inserimento in coda ristabilisco integrità del sistema
					//non ho rimosso files quindi aggiorno eventualmente maxSize della cache
					pthread_mutex_lock(&fileCacheMutex);
					int actQueueSize = fileCache.size;
					pthread_mutex_unlock(&fileCacheMutex);
					pthread_mutex_lock(&updateStatsMutex);
					if(actQueueSize > serverStatistics.fileCacheMaxSize)
						serverStatistics.fileCacheMaxSize = actQueueSize;
					pthread_mutex_unlock(&updateStatsMutex);
				}
			}
		}else if(r.flags != O_CREAT && risFind > 0){ //cerca file in cache ed aggiorna flag isOpen(se è gia == 1, ho un errore)
			pthread_mutex_lock(&fileCacheMutex);
			GenericNode *corr = fileCache.queue.head;
			MyFile f1;
			memset(&f1, 0, sizeof(f1));
			strncpy(f1.filePath, r.request_content, strlen(r.request_content));

			while(corr != NULL){
				if(fileCache.comparison(corr->info, (void*)&f1) == 1){
					MyFile *toOpen = (MyFile*)(corr->info);
					if(toOpen->isOpen == 0){
						toOpen->isOpen = 1;
						toOpen->lastSucceedOp.opType = r.type;
						toOpen->lastSucceedOp.optFlags = r.flags;
						toOpen->lastSucceedOp.clientDescriptor = r.comm_socket;
					}else
						result = -3;
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

/*
	Richiesta read file ritorna:
	0 -> successo
	-1 -> errori di read/write

	Cerca file e ne ritorna il contenuto al client
	facendo precedere la lunghezza del messaggio
*/
int executeReadFile(MyRequest r){
	MyFile f1;
	memset(&f1, 0, sizeof(f1));
	strncpy(f1.filePath, r.request_content, strlen(r.request_content));
	pthread_mutex_lock(&fileCacheMutex);
	int risFind = findElement(fileCache, (void*)&f1);
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
	MyFile *toRead;
	

	while(corr != NULL){
		if(fileCache.comparison(corr->info, (void*)&f1) == 1){
			toRead = (MyFile*)(corr->info);
			if(toRead->isOpen == 0){
				res = -1;
				break;
			}
			b = malloc(toRead->dim + 1);
			memset(b, 0, toRead->dim + 1);
			strncpy(b, toRead->content, toRead->dim);
			b[toRead->dim] = '\0';
			res = 0;
			toRead->lastSucceedOp.opType = r.type;
			toRead->lastSucceedOp.optFlags = r.flags;
			toRead->lastSucceedOp.clientDescriptor = r.comm_socket;
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

		free(b);
		//eventuale free(b) per liberare puntatore allocato sullo heap
	}
	return 0;
}

/*
	Richiesta close file ritorna:
	0 -> successo
	-1 -> errori di read/write

	Chiude il file, se questo c'è nella cache 
	e non è già chiuso, e resetta il flag modified del file
*/
int executeCloseFile(MyRequest r){
	MyFile f1, *toRead;
	int res = -1, l;
	memset(&f1, 0, sizeof(f1));
	strncpy(f1.filePath, r.request_content, strlen(r.request_content));
	pthread_mutex_lock(&fileCacheMutex);
	GenericNode *corr = fileCache.queue.head;

	while(corr != NULL){
		//pthread_mutex_lock(&fileCacheMutex);
		if(fileCache.comparison(corr->info, (void*)&f1) == 1){
			//pthread_mutex_unlock(&fileCacheMutex);
			toRead = (MyFile*)(corr->info);
			if(toRead->isOpen == 0){
				res = -2;
			}else{
				toRead->isOpen = 0;
				res  = 0;
				toRead->lastSucceedOp.opType = r.type;
				toRead->lastSucceedOp.optFlags = r.flags;
				toRead->lastSucceedOp.clientDescriptor = r.comm_socket;
			}
			break;
		}
		corr = corr->next;
	}
	pthread_mutex_unlock(&fileCacheMutex);
	

	if((l = writen(r.comm_socket, &res, sizeof(int))) == -1)
		return -1;
	return 0;
}

/*
	Richiesta remove file ritorna:
	0 -> successo
	-1 -> errori di read/write

	Rimuove il file cercato se presente ed in stato di locked(non implementato, quindi fallisce sempre)
*/
int executeRemoveFile(MyRequest r){
	MyFile f1, *toDel;
	int res = -1, l;
	memset(&f1, 0, sizeof(f1));
	strncpy(f1.filePath, r.request_content, strlen(r.request_content));
	pthread_mutex_lock(&fileCacheMutex);
	GenericNode *corr = fileCache.queue.head;

	while(corr != NULL){
		//pthread_mutex_lock(&fileCacheMutex);
		if(fileCache.comparison(corr->info, (void*)&f1) == 1){
			//pthread_mutex_unlock(&fileCacheMutex);
			toDel = (MyFile*)(corr->info);
			if(toDel->isLocked == 0){
				res = -2;
			}else{
				res  = 0;
				pthread_mutex_lock(&updateStatsMutex);
				serverStatistics.fileCacheActStorageSize -= toDel->dim; //aggiornamento storageSize
				pthread_mutex_unlock(&updateStatsMutex);
				//pthread_mutex_lock(&fileCacheMutex);
				deleteElement(&fileCache, (void*) &f1); //implicito il decremento della cardinalità della coda
				//pthread_mutex_unlock(&fileCacheMutex);
			}
			break;
		}
		corr = corr->next;
	}
	pthread_mutex_unlock(&fileCacheMutex);

	if((l = writen(r.comm_socket, &res, sizeof(int))) == -1)
		return -1;
	return 0;
}

/*
	Richiesta append file ritorna:
	0 -> successo
	-1 -> errori di read/write

	Estende contenuto del file se presente ed aperto
	e se ciò non causa il fallimento dell'algoritmo di rimpiazzamento
*/
int executeAppendFile(MyRequest r){
	MyFile f1, toRead;
	int res = 0, l, replaceResult;
	size_t buf_size;
	char *buf;

	memset(&f1, 0, sizeof(f1));
	strncpy(f1.filePath, r.request_content, strlen(r.request_content));
	pthread_mutex_lock(&fileCacheMutex);
	int isPresent = findElement(fileCache, (void*)&f1);
	pthread_mutex_unlock(&fileCacheMutex);

	if(isPresent < 0){
		res = -1;
		if((l = writen(r.comm_socket, &res, sizeof(int))) == -1){
			//printf("Non buono1\n");
			return -1;
		}
		return 0;
	}

	if((l = writen(r.comm_socket, &res, sizeof(int))) == -1){
		//printf("Non buono2\n");
		return -1;
	}

	if((l = readn(r.comm_socket, &buf_size, sizeof(size_t))) == -1){
		//printf("Non buono3\n");
		return -1;
	}

	buf = malloc((buf_size + 1) * sizeof(char));
	if(buf == NULL)
		return -1;
	memset(buf, 0, buf_size + 1);

	if((l = readn(r.comm_socket, buf, buf_size * sizeof(char))) == -1){
		//printf("Non buono4\n");
		return -1;
	}
	buf[l] = '\0';

	//verifica replaceCondition e possibility, se fallisce allora invio result = -4
	int replaceCondition = verifyReplaceCondition(r.type, buf_size);
	int replacePossibility = verifyReplacePossibility(r.type, r.request_content, buf_size);

	if(replaceCondition == 1 && replacePossibility == 0){
		res = -4; //algoritmo di rimpiazzamento fallirebbe quindi non permetto l'esecuzione dell'operazione
		if((l = writen(r.comm_socket, &res, sizeof(int))) == -1)
			return -1;
		return 0;
	}

	pthread_mutex_lock(&fileCacheMutex);
	GenericNode* corr = fileCache.queue.head;

	while(corr != NULL){
		//pthread_mutex_lock(&fileCacheMutex);
		if(fileCache.comparison(corr->info, (void*)&f1) == 1){
			MyFile *toAppend = (MyFile*) corr->info;
			//pthread_mutex_unlock(&fileCacheMutex);
			if(toAppend->isOpen == 0){
				res = -2;
			}else{
				res = 0;
				int act_dim = toAppend->dim + 1;
				pthread_mutex_lock(&updateStatsMutex);
				int checkCond = (toAppend->dim + l > s.maxStorageSpace);
				pthread_mutex_unlock(&updateStatsMutex);
				if(checkCond){ //evito che in append un file superi il massimo storageSpace della file cache
					res = -3;
					break;
				}
				while(toAppend->dim + l >= act_dim){
					toAppend->content = realloc(toAppend->content, 2 * act_dim * sizeof(char));
					act_dim *= 2;
					if(toAppend->content == NULL){
						perror("Errore realloc");
						exit(EXIT_FAILURE);
					}
				}
				//strncat(toAppend->content, buf, strlen(buf) + 1);
				//printf("Prima di estendere il contenuto: %s\n", toAppend->content);
				memcpy(toAppend->content + toAppend->dim, buf, l);
				toAppend->dim += l;
				toAppend->content[toAppend->dim] = '\0';
				toAppend->modified = 1;
				toAppend->timestamp = time(NULL);
				toAppend->lastSucceedOp.opType = r.type;
				toAppend->lastSucceedOp.optFlags = r.flags;
				toAppend->lastSucceedOp.clientDescriptor = r.comm_socket;
				pthread_mutex_lock(&updateStatsMutex);
				serverStatistics.fileCacheActStorageSize += l;
				pthread_mutex_unlock(&updateStatsMutex);
				pthread_mutex_unlock(&fileCacheMutex);
				replaceResult = replacingAlgorithm();
				pthread_mutex_lock(&fileCacheMutex);
				if(replaceResult == -1){
					perror("Rilevata inconsistenza nella cache\n"); //analogo alla openFile
					exit(EXIT_FAILURE);
				}
				printf("Ho scritto in APPEND il file: %s\n", toAppend->filePath);
				printf("Esito algoritmo di rimpiazzamento: %d\n\n", replaceResult);
				if(replaceResult == 1){
					//non ho rimosso files, procedo con aggiornamento maxStorageSize
					pthread_mutex_lock(&updateStatsMutex);
					if(serverStatistics.fileCacheActStorageSize > serverStatistics.fileCacheMaxStorageSize)
						serverStatistics.fileCacheMaxStorageSize = serverStatistics.fileCacheActStorageSize; 
					pthread_mutex_unlock(&updateStatsMutex);
				}
			}
			break;
		}
		corr = corr->next;
	}
	pthread_mutex_unlock(&fileCacheMutex);	

	if((l = writen(r.comm_socket, &res, sizeof(int))) == -1){
		//printf("Fine richiesta APPEND_FILE con INsuccesso\n");
		return -1;
	}
	free(buf);
	//printf("Fine richiesta APPEND_FILE con successo\n");
	return 0;
}

/*
	Richiesta readN file ritorna:
	0 -> successo
	-1 -> errori di read/write

	Invia N file della fileCache al client
	(ogni invio è ua coppia di messaggi 
	<metadati_file, contenuto_file>); 
	se N <= 0 || N > fileCache.size allora invia tutti i
	file nella cache
*/
int executeReadNFile(MyRequest r){
	int N = atoi(r.request_content);
	int i = 0, l;
	int finished = 0;

	pthread_mutex_lock(&fileCacheMutex);
	//se N <= 0 o più grande della cache size io invio tutti i file attualemente in cache
	int numFileExpected = N <= 0 || N > fileCache.size? fileCache.size : N;
	//pthread_mutex_unlock(&fileCacheMutex);

	//pthread_mutex_lock(&fileCacheMutex);
	GenericNode *corr = fileCache.queue.head;

	finished = !(i < numFileExpected);
	if((l = writen(r.comm_socket, &finished, sizeof(int))) == -1){
		pthread_mutex_unlock(&fileCacheMutex);
		return -1;
	}

	while(i < numFileExpected){
		MyFile *toSend = (MyFile*)corr->info;
		corr = corr->next;
		if((l = writen(r.comm_socket, toSend, sizeof(*toSend))) == -1){
			pthread_mutex_unlock(&fileCacheMutex);
			return -1;
		}
		
		if((l = writen(r.comm_socket, toSend->content, toSend->dim)) == -1){
			pthread_mutex_unlock(&fileCacheMutex);
			return -1;
		}
		//aggiorno ultima operazione con successo eseguita sul file
		toSend->lastSucceedOp.opType = r.type;
		toSend->lastSucceedOp.optFlags = r.flags;
		toSend->lastSucceedOp.clientDescriptor = r.comm_socket;
		i++;
		finished = !(i < numFileExpected);
		if((l = writen(r.comm_socket, &finished, sizeof(int))) == -1){
			pthread_mutex_unlock(&fileCacheMutex);
			return -1;
		}
	}
	pthread_mutex_unlock(&fileCacheMutex);
	
	return i;
}

/*
	Richiesta write file ritorna:
	0 -> successo
	-1 -> errori di read/write

	Scrive il primo contenuto del file se questo
	è presente, aperto e l'ultima operazione effettuata
	con successo sul file è una OPEN_FILE con flag
	O_CREAT inviata dallo stesso client che vuole
	scrivere il file
*/
int executeWriteFile(MyRequest r){
	int buf_size, l, found = 0, result = 0, replaceResult;
	char *buf; //buffer dove salvare i dati da scrivere
	MyFile toFind;
	memset(&toFind, 0, sizeof(MyFile));

	strncpy(toFind.filePath, r.request_content, strlen(r.request_content));

	if((l = readn(r.comm_socket, &buf_size, sizeof(int))) == -1)
		return -1;

	buf = malloc(buf_size + 1);
	if(buf == NULL)
		return -1;
	memset(buf, 0, buf_size + 1);

	if((l = readn(r.comm_socket, buf, buf_size)) == -1)
		return -1;

	buf[l] = '\0';

	//verifica replaceCondition e possibility, se fallisce allora invio result = -5
	int replaceCondition = verifyReplaceCondition(r.type, l);
	int replacePossibility = verifyReplacePossibility(r.type, r.request_content, l);

	//ho errore se l'implicazione verifyReplaceCondition -> verifyReplacePossibility è falsa
	if(replaceCondition == 1 && replacePossibility == 0){
		result = -5;
		if((l = writen(r.comm_socket, &result, sizeof(int))) == -1)
			return -1;
		return 0;
	}

	pthread_mutex_lock(&fileCacheMutex);
	GenericNode *corr = fileCache.queue.head;
	//pthread_mutex_unlock(&fileCacheMutex);

	while(corr != NULL){
		//pthread_mutex_lock(&fileCacheMutex);
		if(fileCache.comparison(corr->info, (void*) &toFind) == 1){
			MyFile *toWrite = (MyFile*) corr->info;
			//pthread_mutex_unlock(&fileCacheMutex);
			found = 1;
			if(toWrite->isOpen == 0){
				result = -2;
			}else if(toWrite->lastSucceedOp.opType == OPEN_FILE && \
						toWrite->lastSucceedOp.optFlags == O_CREAT && \
							toWrite->lastSucceedOp.clientDescriptor == r.comm_socket){
							result = 0;
							pthread_mutex_lock(&updateStatsMutex);
							int checkCond = (buf_size > s.maxStorageSpace);
							pthread_mutex_unlock(&updateStatsMutex);
							if(checkCond){ //evito che in scrittura un file superi il massimo storageSpace della file cache
								result = -4;
								break;
							}
							//MyFile backup = *toWrite; //copia stato attuale del file in caso di rollback
							toWrite->content = malloc(buf_size + 1);
							memset(toWrite->content, 0, sizeof(char) * (buf_size + 1));
							//strncpy(toWrite->content, buf, buf_size + 1);
							memcpy(toWrite->content, buf, buf_size);
							//printf("File content: %s\n", toWrite->content);
							toWrite->content[buf_size] = '\0';
							toWrite->dim = buf_size;
							//MyFile foo = *toWrite;
							//saveFile(foo, "./TestFileBinari");
							toWrite->modified = 1;
							toWrite->timestamp = time(NULL);
							toWrite->lastSucceedOp.opType = r.type;
							toWrite->lastSucceedOp.optFlags = r.flags;
							toWrite->lastSucceedOp.clientDescriptor = r.comm_socket;
							pthread_mutex_lock(&updateStatsMutex);
							serverStatistics.fileCacheActStorageSize += buf_size;
							pthread_mutex_unlock(&updateStatsMutex);
							pthread_mutex_unlock(&fileCacheMutex);
							replaceResult = replacingAlgorithm();
							pthread_mutex_lock(&fileCacheMutex);
							if(replaceResult == -1){
								perror("Rilevata inconsistenza in file cache\n");//analogo ad openFile
								exit(EXIT_FAILURE);
							}
							printf("Ho scritto il file: %s\n", toWrite->filePath);
							printf("Esito algoritmo di rimpiazzamento: %d\n\n", replaceResult);
							if(replaceResult == 1){
								//non ho rimosso files, procedo con aggiornamento maxStorageSize
								pthread_mutex_lock(&updateStatsMutex);
								if(serverStatistics.fileCacheActStorageSize > serverStatistics.fileCacheMaxStorageSize)
									serverStatistics.fileCacheMaxStorageSize = serverStatistics.fileCacheActStorageSize; 
								pthread_mutex_unlock(&updateStatsMutex);
							}
			}else{
				result = -3;
			}
			break;
		}
		corr = corr->next;
	}
	pthread_mutex_unlock(&fileCacheMutex);
	if(found == 0 && result == 0) //non ho trovato il file e non ho incontrato errori precedenti
		result = -1;
	if((l = writen(r.comm_socket, &result, sizeof(int))) == -1)
		return -1;
	free(buf);
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
			break;
		case READ_N_FILE: 
			if(executeReadNFile(r) == -1)
				perror("Errore esecuzione richiesta READ_N_FILE");
			break;
		case WRITE_FILE:
			if(executeWriteFile(r) == -1)
				perror("Errore esecuzione richiesta WRITE_FILE");
			break;
		default: return;	
	}
}

/*
	Funzione eseguita dai worker threads
	per scrivere nella pipe il descrittore sul quale
	il manager deve rimettersi in ascolto
*/
void commSocketGiveBack(int comm_socket){
	int l;
	pthread_mutex_lock(&pipeAccessMutex);

	while(celleLibere == 0)
		pthread_cond_wait(&pList, &pipeAccessMutex);
	celleLibere = 0;
	if((l = writen(p[1], &comm_socket, sizeof(int))) == -1){
		perror("Errore write!");
		exit(EXIT_FAILURE);
	}
	pthread_mutex_unlock(&pipeAccessMutex);
}

/*
	Funzione che rappresenta l'attività di un singolo
	worker thread: estrae richiesta dalla coda, la esegue e
	"restituisce" al manager il descrittore sul quale ha operato
*/
static void* workerThreadActivity(void* args){
	MyRequest *p; //struttura dove memorizzare la richiesta estratta dalla coda

	while(1){ //da implementare 
		pthread_mutex_lock(&requestsQueueMutex); //accesso in ME
		while(isEmpty(requests) == 1){ //coda vuota
			pthread_cond_wait(&waitingForRequestsList, &requestsQueueMutex);
			if(sigIntOrQuitFlag == 1){ //se quando mi sveglio devo interrmopermi esco e termino
				//printf("Esco dalla wait\n");
				break;
			}
		}
		//controllo se mi è arrivato sigInt o se le connessioni sono finite(se è arrivato sigHup in precedenza) se si break;
		if(sigIntOrQuitFlag == 1){
			pthread_mutex_unlock(&requestsQueueMutex);
			break;
		}
		//printf("Stampo coda richieste attuale:\n");
		//printQueue(requests);
		p = (MyRequest*) pop(&requests); //sono sicuro che p != NULL
		//printf("Stampo coda richieste dopo POP:\n");
		//printQueue(requests);
		pthread_mutex_unlock(&requestsQueueMutex);
		executeRequest(*p);
		commSocketGiveBack(p->comm_socket);
		freeRequest(p);
	}
	printf("Sono worker ho finito la mia attività\n");
	return (void*)NULL;
}

int main(int argc, char const *argv[]){
	pthread_t manage_tid;
	pthread_t *workers;
	pthread_t s_handler;
	int i;
	sigset_t mask; //maschera dei segnali

	if(argc < 2){
		perror("Path del file di configurazione deve essere passato da linea di comando!");
		exit(EXIT_FAILURE);
	}

	s = startConfig(argv[1]); //parsing del file di config
	printConfig(s);
	serverStatistics.fileCacheMaxSize = 0; //s'intende la quantità massima di file memorizzati
	serverStatistics.fileCacheMaxStorageSize = 0;
	serverStatistics.replaceAlgInvokeTimes = 0;
	serverStatistics.fileCacheActStorageSize = 0; //s'intende la somma delle dimensioni dei file nel file storage attuale
	workers = malloc(s.nWorkers * sizeof(pthread_t)); //alloco i TID per i worker threads
	memset(workers, 0, sizeof(pthread_t) * s.nWorkers);
	if(workers == NULL){
		perror("Errore malloc!");
		exit(EXIT_FAILURE);
	}
	//creo coda di descrittori che rappresentano le connessioni attuali
	connections = createQueue(&descriptorComparison, &descriptorPrint, &freeDescriptor); 
	fileCache = createQueue(&fileComparison, &filePrint, &freeFile);  //creo cache di file
	requests = createQueue(&requestComparison, &requestPrint, &freeRequest); //creo coda di richieste

	if(pipe(p) < 0 || pipe(signalPipe) < 0){ //creo pipe worker(s)-manager e signal_handler-manager
		perror("Errore creazione pipe(s)\n");
		exit(EXIT_FAILURE);
	}
	
	sigfillset(&mask); //blocco tutti i segnali
	pthread_sigmask(SIG_SETMASK, &mask, NULL); //tale maschera verrà ereditata dai thread "figli"

	//creo thread gestore dei segnali
	if(pthread_create(&s_handler, NULL, &signalHandlerActivity, NULL) == -1){ 
		perror("Errore creazione thread signal handler!");
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
	
	pthread_join(s_handler, NULL);
	//join di tutti i threads
	for(i = 0; i < s.nWorkers; i++)
			pthread_join(workers[i], NULL);
	pthread_join(manage_tid, NULL);
	/*
		Con questa condizione includo anche il caso in cui termino
		in seguito all'arrivo di SIGHUP, in quanto si riconduce
		ad un caso di SIGINT posticipato al momento in cui
		non si hanno più client connessi	
	*/
	if(sigIntOrQuitFlag == 1){
		printServerStats(serverStatistics);
		printf("Stampo file rimasti nella cache:\n");
		printQueue(fileCache);
		//printf("BOB\n");
	}
	freeStructures(); //libero le strutture usate
	free(workers); //libero puntatore di workers
	printf("Terminata esecuzione server\n");
	return 0;
}