#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
//#include<limits.h>
#include<fcntl.h>
#include<errno.h>
#include<time.h>
#include<math.h>
#include "serverAPI.h"
#include "request.h"
#include "file.h"
#include "clientConfig.h"
#include "utility.h"

static int comm_socket_descriptor; //descrittore del socket lato client

extern ClientConfigInfo c; //struttura di configurazione del client

static int imConnected = 0; //flag che mi dice se sono connesso con il server o meno

int openConnection(const char* sockName, int msec, const struct timespec abstime){
	int connected = 0;
	struct sockaddr_un sa;
	double max_nsec = (abstime.tv_sec * pow(10, 9)) + abstime.tv_nsec;

	if(imConnected == 1){
		errno = EPERM; //non posso riconettermi se sono già connesso
		return -1;
	}

	//l'argomento deve essere quello specificato tramite l'opzione -f
	if(strcmp(sockName, c.socketName) != 0){
		errno = EINVAL; //argomento non valido
		return -1;
	}

   	sa.sun_family = AF_UNIX;
   	strncpy(sa.sun_path, sockName, sizeof(sa.sun_path) - 1);
   	comm_socket_descriptor = socket(AF_UNIX, SOCK_STREAM, 0);//ottengo socket descriptor
   	while(!connected && max_nsec > 0){ //finchè non sono connesso oppure non è scaduto il tempo
   		if(connect(comm_socket_descriptor, (struct sockaddr*)&sa, sizeof(sa)) != -1)//provo a connetermi
   			connected = 1;
   		else{
			//socket non ancora creata o rifiuta la connessione:attendo msec millisecondi
   			if(errno == ENOENT || errno == ECONNREFUSED){
   				msleep(msec);
   				max_nsec -= (msec * pow(10, 6));
   			}else{
				imConnected = 0;
   				perror("Errore fatale\n");
   				exit(EXIT_FAILURE);
   			}
   		}
   	}
   	if(connected){
		errno = 0;
		imConnected = 1;
   		printf("Sono connesso al server\n");
   		return 0;
   	}
   	printf("Tempo scaduto\n"); //tempo scaduto per la connessione tutte le r/w falliranno
   	errno = ETIMEDOUT;
	imConnected = 0;
   	return -1;
}

int closeConnection(const char* sockName){
	if(imConnected == 0){
		errno = EPERM; //non posso chiudere una connessione se non sono attualmente connesso con il server
		return -1;
	}

	//stesso discorso della openConnection
	if(strcmp(sockName, c.socketName) != 0){
		errno = EINVAL; //argomento non valido
		return -1;
	}

	do{
		int closeRes = close(comm_socket_descriptor); //provo a chiudere il descrittore
		if(closeRes == -1 && errno != EINTR){
			errno = EBADR; //descrittore richiesto non valido
			return -1;
		}
	}while(errno == EINTR);
	errno = 0;
	imConnected = 0;
	return 0;
}

int openFile(const char* pathname, int flags){
	if(pathname == NULL || (flags != O_CREAT && flags != 0)){
		perror("Invalid flags: it can be just O_CREAT \
			(if the file doesn't exist in the cache it will \
			 be created) or 0(no flags).In addition, pathname must be != NULL\n");
		errno = EINVAL;
		return -1;
	}
	MyRequest r;
	int ris, l;
	char absPath[PATH_MAX], pathBackup[PATH_MAX];
	memset(absPath, 0, sizeof(absPath));
	memset(pathBackup, 0, sizeof(pathBackup));

	strncpy(pathBackup, pathname, strlen(pathname));

	//costruisco richiesta da spedire
	memset(&r, 0, sizeof(r));
	r.flags = flags;
	r.type = OPEN_FILE;
	getAbsPathFromRelPath(pathBackup, absPath, PATH_MAX); //ottengo path assoluto da quello relativo
	strncpy(r.request_content, absPath, strlen(absPath));
	r.request_content[strlen(absPath)] = '\0';
	r.timestamp = time(NULL); //ottengo data/ora attuale
	r.request_dim = strlen(r.request_content);

	if((l = writen(comm_socket_descriptor, &r, sizeof(r))) == -1){
		errno = EAGAIN;
		return -1;
	}

	if((l = readn(comm_socket_descriptor, &ris, sizeof(int))) == -1){
		errno = EAGAIN;
		return -1;
	}

	switch(ris){
		case -1:
			errno = EEXIST; //file già esistente(flag O_CREAT specificato)
			return -1;
		case -2:
			errno = ENOENT; //file inesistente (flag O_CREAT non specificato)
			return -1;
		case -3:
			errno = EPERM; //non posso aprire un file già aperto(flag O_CREAT non specificato)
			return -1;
		case -4:
			errno = ENOSPC; //l'operazione ha causato il fallimento dell'algoritmo di rimpiazzamento non posso liberare memoria in filecache
			return -1;
		default: break;
	}
	errno = 0;
	return 0;
}

int readFile(const char* pathname, void** buf, size_t* size){
	MyRequest r;
	int l;
	int ris;
	int buf_size; //riceve la buf_size del contenuto da leggere
	char absPath[PATH_MAX], pathBackup[PATH_MAX];
	memset(absPath, 0, sizeof(absPath));
	memset(pathBackup, 0, sizeof(pathBackup));

	strncpy(pathBackup, pathname, strlen(pathname));

	//costruzione richiesta
	memset(&r, 0, sizeof(r));
	r.type = READ_FILE;
	r.flags = 0;
	r.timestamp = time(NULL);
	getAbsPathFromRelPath(pathBackup, absPath, PATH_MAX);
	strncpy(r.request_content, absPath, strlen(absPath));
	r.request_dim = strlen(r.request_content);

	//scrivo la richiesta
	if((l = writen(comm_socket_descriptor, &r, sizeof(r))) == -1){
		errno = EAGAIN;
		return -1;
	}

	//attendo risultato operazione
	if((l = readn(comm_socket_descriptor, &ris, sizeof(int))) == -1){
		errno = EAGAIN;
		return -1;
	}

	switch(ris){
		//in questi casi buf e size non sono validi
		case -1:
			*buf = NULL;
			*size = 0;
			errno = EACCES; //non posso leggere un file non aperto in precedenza 
			return -1;
		case -2:
			*buf = NULL;
			*size = 0;
			errno = ENOENT; //file da leggere non presente nel file
			return -1; 
		default: break; 
	}

	//ricevo buf_size
	if((l = readn(comm_socket_descriptor, &buf_size, sizeof(int))) == -1){
		errno = EAGAIN;
		return -1;
	}

	*size = buf_size;
	*buf = malloc(sizeof(char) * (buf_size + 1)); //alloco buffer per accogliere il contenuto del file
	if(*buf == NULL){//errore fatale della malloc
		perror("Malloc error!\n");
		exit(EXIT_FAILURE);
	}
	memset(*buf, 0, (buf_size + 1));
	
	//leggo il contenuto
	if((l = readn(comm_socket_descriptor, *buf, *size)) == -1){
		errno = EAGAIN;
		return -1;
	}
	errno = 0;
	return 0;
}

int readNFiles(int N, const char* dirname){
	MyRequest r;
	MyFile toSave;//struttura di ricezione del file
	int l, finished, counter = 0, readBytes = 0;

	//costruisco richiesta
	memset(&r, 0, sizeof(MyRequest));
	r.flags = 0;
	r.timestamp = time(NULL);
	r.type = READ_N_FILE;
	sprintf(r.request_content, "%d", N);//request_content conterrà il numero di file da leggere
	r.request_dim = strlen(r.request_content);

	//spedisco richiesta
	if((l = writen(comm_socket_descriptor, &r, sizeof(MyRequest))) == -1){
		errno = EAGAIN;
		return -1;
	}

	//ricevo flag che mi dice se il server ha ancora file da mandarmi
	if((l = readn(comm_socket_descriptor, &finished, sizeof(int))) == -1){
		errno = EAGAIN;
		return -1;
	}

	while(finished != 1){//finchè il server ha file da spedirmi
		memset(&toSave, 0, sizeof(toSave));
		//ricevo metadati del file
		if((l = readn(comm_socket_descriptor, &toSave, sizeof(toSave))) == -1){
			errno = EAGAIN;
			return -1;
		}
		toSave.content = malloc(toSave.dim * sizeof(char));

		//ricevo contenuto del file
		if((l = readn(comm_socket_descriptor, toSave.content, toSave.dim)) == -1){
			errno = EAGAIN;
			return -1;
		}

		//se la cartella è specificata salvo i file, altrimenti no
		if(dirname != NULL && strcmp(dirname, "") != 0){
			if((l = saveFile(toSave, dirname)) == -1){
				perror("Errore salvataggio file!\n");
				errno = EIO; //errore I/O nel salvataggio su disco del file
				return -1;
			}
		}

		free(toSave.content);
		counter++; //incremento numero di file letti(da ritornare)
		readBytes += toSave.dim; //incremento numero di byte letti in totale

		if((l = readn(comm_socket_descriptor, &finished, sizeof(int))) == -1){
			errno = EAGAIN;
			return -1;
		}
	}
	errno = 0;
	if(c.printEnable == 1)
		printf("In totale ho letto %d bytes\n", readBytes);
	return counter;
}

int writeFile(const char* pathname, const char* dirname){
	char *fileContent; //
	int fileContentDim, l, result;
	char absPath[PATH_MAX];
	memset(absPath, 0, sizeof(absPath));
	//funzione per leggere file che ritorna char* nel quale ho il contenuto del file
	int ris = readFileContent(pathname, &fileContent); //ris contiene il numero di byte letti
	MyRequest r;
	memset(&r, 0, sizeof(MyRequest));

	if(ris == -1){ //errore lettura nel file, impossibile eseguire l'operazione
		perror("Errore lettura file\n");
		errno = EIO;
		return -1;
	}
	//setup richiesta
	fileContentDim = ris;
	realpath(pathname, absPath); //sicuro che absPath non è vuoto in quanto ris != -1
	r.type = WRITE_FILE;
	r.timestamp = time(NULL);
	r.flags = 0;
	strncpy(r.request_content, absPath, strlen(absPath));
	r.request_dim = strlen(absPath);

	//scrittura richiesta(ricevuta dal manager)
	if((l = writen(comm_socket_descriptor, &r, sizeof(r))) == -1){
		errno = EAGAIN;
		return -1;
	}

	//scrittura buf_size
	if((l = writen(comm_socket_descriptor, &fileContentDim, sizeof(int))) == -1){
		errno = EAGAIN;
		return -1;
	}

	//scrittura buf
	if((l = writen(comm_socket_descriptor, fileContent, fileContentDim)) == -1){
		errno = EAGAIN;
		return -1;
	}

	//attesa risultato
	if((l = readn(comm_socket_descriptor, &result, sizeof(int))) == -1){
		errno = EAGAIN;
		return -1;
	}

	switch(result){
		case -1:
			errno = ENOENT; //file da scrivere non presente nel server 
			return -1;
		case -2: 
			errno = EACCES; //file da scrivere non aperto
			return -1;
		case -3: 
			errno = EPERM; //operazione non permessa(la precedente non è stata una openFile con O_CREAT)
			return -1;
		case -4: 
			errno = ENOSPC; //non posso scrivere un file la cui dimensione è > del maxStorageSpace della fileCache
			return -1;
		case -5:
			errno = ENOSPC; //scrittura non effettuata in quanto causerebbe il fallimento dell'algoritmo di rimpiazzamento
			return -1;
		default: break;
	}
	errno = 0;
	if(c.printEnable == 1)
		printf("Ho scritto nel file storage %d bytes\n", fileContentDim);
	return 0;
}

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname){
	MyRequest r;
	int l;
	int ris;
	char pathBackup[PATH_MAX], absPath[PATH_MAX];
	memset(absPath, 0, sizeof(absPath));
	memset(pathBackup, 0, sizeof(pathBackup));

	strncpy(pathBackup, pathname, strlen(pathname)); 

	//costruisco richiesta
	memset(&r, 0, sizeof(r));
	r.type = APPEND_FILE;
	r.flags = 0;
	r.timestamp = time(NULL);
	getAbsPathFromRelPath(pathBackup, absPath, PATH_MAX);//traduzione da path relativo ad assoluto
	strncpy(r.request_content, absPath, strlen(absPath));
	r.request_dim = strlen(r.request_content);

	//scrivo richiesta
	if((l = writen(comm_socket_descriptor, &r, sizeof(r))) == -1){
		errno = EAGAIN;
		return -1;
	}

	//lettura risultato controllo di presenza del file
	if((l = readn(comm_socket_descriptor, &ris, sizeof(int))) == -1){
		errno = EAGAIN;
		return -1;
	}

	//se non c'è
	if(ris == -1){
		errno = ENOENT; //file da scrivere in append non presente nel server
		return -1;
	}

	//scrivo size
	if((l = writen(comm_socket_descriptor, &size, sizeof(size_t))) == -1){
		errno = EAGAIN;
		return -1;
	}

	//scrivo buf
	if((l = writen(comm_socket_descriptor, buf, size)) == -1){
		errno = EAGAIN;
		return -1;
	}

	//leggo risultato operazione
	if((l = readn(comm_socket_descriptor, &ris, sizeof(int))) == -1){
		errno = EAGAIN;
		return -1;
	}

	switch(ris){
		case -2:
			errno = EACCES; //file da scrivere in append non è stato ancora aperto
			return -1;
		case -3:
			errno = ENOSPC; //non posso appendere un file se la sua dimensione supererebbe il maxStorageSpace della fileCache
			return -1; 
		case -4: 
			errno = ENOSPC; //non permetto l'operazione in quanto causerebbe il fallimento dell'algoritmo di rimpiazzamento
			return -1;
		default: break;
	}
	errno = 0;
	if(c.printEnable == 1)
		printf("Ho scritto nel file storage server %d bytes\n", size);
	return 0;
}

int closeFile(const char* pathname){
	MyRequest r;
	int l;
	int ris;
	char pathBackup[PATH_MAX], absPath[PATH_MAX];
	memset(absPath, 0, sizeof(absPath));
	memset(pathBackup, 0, sizeof(pathBackup));

	strncpy(pathBackup, pathname, strlen(pathname)); 

	//costruzione richiesta
	memset(&r, 0, sizeof(r));
	r.type = CLOSE_FILE;
	r.flags = 0;
	r.timestamp = time(NULL);
	getAbsPathFromRelPath(pathBackup, absPath, PATH_MAX);
	strncpy(r.request_content, absPath, strlen(absPath));
	r.request_dim = strlen(r.request_content);

	//scrittura richiesta
	if((l = writen(comm_socket_descriptor, &r, sizeof(r))) == -1){
		errno = EAGAIN;
		return -1;
	}

	//lettura risultato operazione
	if((l = readn(comm_socket_descriptor, &ris, sizeof(int))) == -1){
		errno = EAGAIN;
		return -1;
	}

	switch(ris){
		case -1:
			errno = ENOENT;  //file da chiudere non presente nel file
			return -1;
		case -2:
			errno = EPERM; //operazione non permessa:impossibile chiudere un file già chiuso
			return -1; 
		default: break; 
	}
	errno = 0;
	return 0;
}

int removeFile(const char* pathname){
	MyRequest r;
	int l;
	int ris;
	char pathBackup[PATH_MAX], absPath[PATH_MAX];
	memset(absPath, 0, sizeof(absPath));
	memset(pathBackup, 0, sizeof(pathBackup));

	strncpy(pathBackup, pathname, strlen(pathname)); 

	//costruzione richiesta
	memset(&r, 0, sizeof(r));
	r.type = REMOVE_FILE;
	r.flags = 0;
	r.timestamp = time(NULL);
	getAbsPathFromRelPath(pathBackup, absPath, PATH_MAX);
	strncpy(r.request_content, absPath, strlen(absPath));
	r.request_dim = strlen(r.request_content);

	//scrittura richiesta
	if((l = writen(comm_socket_descriptor, &r, sizeof(r))) == -1){
		errno = EAGAIN;
		return -1;
	}

	//lettura risultato
	if((l = readn(comm_socket_descriptor, &ris, sizeof(int))) == -1){
		errno = EAGAIN;
		return -1;
	}

	switch(ris){
		case -1:
			errno = ENOENT;  //file da rimuovere non presente nel server
			return -1;
		case -2:
			errno = EPERM;  //Non posso rimuovere il file se questo non è in stato di locked
			return -1;  
		default: break; 
	}
	errno = 0;
	return 0;
}