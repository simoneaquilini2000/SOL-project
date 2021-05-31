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

	if(strcmp(sockName, c.socketName) != 0){
		errno = EINVAL; //argomento non valido
		return -1;
	}

   	sa.sun_family = AF_UNIX;
   	strncpy(sa.sun_path, sockName, sizeof(sa.sun_path) - 1);
   	comm_socket_descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
   	while(!connected && max_nsec > 0){ //finchè non sono connesso oppure non è scaduto il tempo
   		if(connect(comm_socket_descriptor, (struct sockaddr*)&sa, sizeof(sa)) != -1)
   			connected = 1;
   		else{
   			//printf("Errore in connesione di codice: %d\n", errno);
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
   	printf("Tempo scaduto\n");
   	errno = ETIMEDOUT;
	imConnected = 0;
   	return -1;
}

int closeConnection(const char* sockName){
	if(imConnected == 0){
		errno = EPERM; //non posso chiudere una connessione se non sono attualmente connesso con il server
		return -1;
	}

	if(strcmp(sockName, c.socketName) != 0){
		errno = EINVAL; //argomento non valido
		return -1;
	}

	do{
		//printf("Sto chiudendo connessione\n");
		int closeRes = close(comm_socket_descriptor);
		if(closeRes == -1 && errno != EINTR){
			//printf("MUOIO\n");
			errno = EBADR; //descrittore richiesto non valido
			return -1;
		}
	}while(errno == EINTR);
	errno = 0;
	imConnected = 0;
	//printf("Disconnesso\n");
	//unlink(c.socketName);
	return 0;
}

int openFile(const char* pathname, int flags){
	if(flags != O_CREAT && flags != 0){
		perror("Invalid flags: it can be just O_CREAT(if the file doesn't exist in the cache it will be created) or 0(no flags)\n");
		errno = EINVAL;
		return -1;
	}
	MyRequest r;
	int ris, l;
	char absPath[PATH_MAX], pathBackup[PATH_MAX];
	memset(absPath, 0, sizeof(absPath));
	memset(pathBackup, 0, sizeof(pathBackup));

	strncpy(pathBackup, pathname, strlen(pathname));

	memset(&r, 0, sizeof(r));
	//r.comm_socket = comm_socket_descriptor;
	r.flags = flags;
	r.type = OPEN_FILE;
	getAbsPathFromRelPath(pathBackup, absPath, PATH_MAX);
	/*char *res = realpath(pathname, buf);
	if(res == NULL){
		perror("File to open not found!\n");
		errno = EINVAL;
		return -1;
	}*/
	//r.request_content = malloc(sizeof(char) * (strlen(pathname) + 1));
	strncpy(r.request_content, absPath, strlen(absPath));
	r.request_content[strlen(absPath)] = '\0';
	r.timestamp = time(NULL);
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
			//break;
		case -2:
			//printf("Setto errno = ENOENT\n");
			errno = ENOENT; //file inesistente (flag O_CREAT non specificato)
			return -1;
		case -3:
			errno = EPERM; //non posso aprire un file già aperto
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
	int buf_size;
	char absPath[PATH_MAX], pathBackup[PATH_MAX];
	memset(absPath, 0, sizeof(absPath));
	memset(pathBackup, 0, sizeof(pathBackup));

	strncpy(pathBackup, pathname, strlen(pathname));

	memset(&r, 0, sizeof(r));
	r.type = READ_FILE;
	r.flags = 0;
	r.timestamp = time(NULL);
	getAbsPathFromRelPath(pathBackup, absPath, PATH_MAX);
	strncpy(r.request_content, absPath, strlen(absPath));
	r.request_dim = strlen(r.request_content);

	if((l = writen(comm_socket_descriptor, &r, sizeof(r))) == -1){
		//printf("1\n");
		errno = EAGAIN;
		return -1;
	}

	if((l = readn(comm_socket_descriptor, &ris, sizeof(int))) == -1){
		//printf("2\n");
		errno = EAGAIN;
		return -1;
	}

	switch(ris){
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

	if((l = readn(comm_socket_descriptor, &buf_size, sizeof(int))) == -1){
		//printf("3\n");
		errno = EAGAIN;
		return -1;
	}

	*size = buf_size;
	*buf = malloc(sizeof(char) * (buf_size + 1));
	memset(buf, 0, (buf_size + 1));
	if(*buf == NULL){
		return -1;
	}

	if((l = readn(comm_socket_descriptor, *buf, *size)) == -1){
		//printf("4\n");
		errno = EAGAIN;
		return -1;
	}
	errno = 0;
	return 0;
}

int readNFiles(int N, const char* dirname){
	MyRequest r;
	MyFile toSave;
	int l, finished, counter = 0, readBytes = 0;

	memset(&r, 0, sizeof(MyRequest));
	r.flags = 0;
	r.timestamp = time(NULL);
	r.type = READ_N_FILE;
	sprintf(r.request_content, "%d", N);
	r.request_dim = strlen(r.request_content);

	if((l = writen(comm_socket_descriptor, &r, sizeof(MyRequest))) == -1){
		errno = EAGAIN;
		return -1;
	}

	if((l = readn(comm_socket_descriptor, &finished, sizeof(int))) == -1){
		errno = EAGAIN;
		return -1;
	}

	while(finished != 1){
		memset(&toSave, 0, sizeof(toSave));
		if((l = readn(comm_socket_descriptor, &toSave, sizeof(toSave))) == -1){
			errno = EAGAIN;
			return -1;
		}
		//printf("Sto ricevendo file %s\n", toSave.filePath);
		toSave.content = malloc(toSave.dim * sizeof(char));

		if((l = readn(comm_socket_descriptor, toSave.content, toSave.dim)) == -1){
			errno = EAGAIN;
			return -1;
		}

		if(dirname != NULL && strcmp(dirname, "") != 0){
			if((l = saveFile(toSave, dirname)) == -1){
				perror("Errore salvataggio file!\n");
				errno = EIO; //errore I/O nel salvataggio su disco del file
				return -1;
			}
		}

		free(toSave.content);
		counter++;
		readBytes += toSave.dim;

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
	//printf("Scrivo file: %s\n", pathname);

	char *fileContent;
	int fileContentDim, l, result;
	char absPath[PATH_MAX];
	memset(absPath, 0, sizeof(absPath));
	//funzione per cercare file che ritorna *char nel quale ho il contenuto del file
	int ris = readFileContent(pathname, &fileContent); //ris contiene il numero di byte letti
	MyRequest r;
	memset(&r, 0, sizeof(MyRequest));

	if(ris == -1){
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
			errno = EPERM; //operazione non permessa
			return -1;
		case -4: 
			errno = EIO; //non posso scrivere un file la cui dimensione è > del maxStorageSpace della fileCache
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

	memset(&r, 0, sizeof(r));
	r.type = APPEND_FILE;
	r.flags = 0;
	r.timestamp = time(NULL);
	getAbsPathFromRelPath(pathBackup, absPath, PATH_MAX);
	strncpy(r.request_content, absPath, strlen(absPath));
	r.request_dim = strlen(r.request_content);

	//printf("Voglio appendere %s\n", (char*)buf);

	if((l = writen(comm_socket_descriptor, &r, sizeof(r))) == -1){
		errno = EAGAIN;
		return -1;
	}

	if((l = readn(comm_socket_descriptor, &ris, sizeof(int))) == -1){
		errno = EAGAIN;
		return -1;
	}

	//printf("Ho ricevuto dal server = %d\n", ris);

	if(ris == -1){
		errno = ENOENT; //file da scrivere in append non presente nel server
		return -1;
	}

	if((l = writen(comm_socket_descriptor, &size, sizeof(size_t))) == -1){
		errno = EAGAIN;
		return -1;
	}

	//printf("Buf da mandare = %s\n", (char*) buf);

	if((l = writen(comm_socket_descriptor, buf, size)) == -1){
		errno = EAGAIN;
		return -1;
	}

	//printf("Esito scrittura = %d\n", l);

	//printf("Leggo risultato dal server\n");

	if((l = readn(comm_socket_descriptor, &ris, sizeof(int))) == -1){
		errno = EAGAIN;
		return -1;
	}

	//printf("Adesso ris = %d\n", ris);

	switch(ris){
		case -2:
			errno = EACCES; //file da scrivere in append non è stato ancora aperto
			return -1;
			//break;
		case -3:
			errno = EPERM; //non posso appendere un file se la sua dimensione supererebbe il maxStorageSpace della fileCache
			return -1; 
			//break;
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

	memset(&r, 0, sizeof(r));
	r.type = CLOSE_FILE;
	r.flags = 0;
	r.timestamp = time(NULL);
	getAbsPathFromRelPath(pathBackup, absPath, PATH_MAX);
	strncpy(r.request_content, absPath, strlen(absPath));
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
			errno = ENOENT;  //file da leggere non presente nel file
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

	memset(&r, 0, sizeof(r));
	r.type = REMOVE_FILE;
	r.flags = 0;
	r.timestamp = time(NULL);
	getAbsPathFromRelPath(pathBackup, absPath, PATH_MAX);
	strncpy(r.request_content, absPath, strlen(absPath));
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