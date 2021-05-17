#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<errno.h>
#include<time.h>
#include<math.h>
#include "serverAPI.h"
#include "request.h"
#include "file.h"
#include "clientConfig.h"
#include "utility.h"

int comm_socket_descriptor; //descrittore del socket lato client

extern ClientConfigInfo c;

/*
	funzione di attesa in millisecondi(rientrante)
*/
void msleep(int msec){
	struct timespec ts, tm;

	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * pow(10, 6);

	do{
		nanosleep(&ts, &tm);
	}while(errno == EINTR);
}

int openConnection(const char* sockName, int msec, const struct timespec abstime){
	int connected = 0;
	struct sockaddr_un sa;
	double max_nsec = (abstime.tv_sec * pow(10, 9)) + abstime.tv_nsec;

	if(strcmp(sockName, c.socketName) != 0){
		errno = EINVAL; //argomento non valido
		return -1;
	}

   	sa.sun_family = AF_UNIX;
   	strncpy(sa.sun_path, sockName, sizeof(sa.sun_path) - 1);
   	comm_socket_descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
   	while(!connected && max_nsec > 0){
   		if(connect(comm_socket_descriptor, (struct sockaddr*)&sa, sizeof(sa)) != -1)
   			connected = 1;
   		else{
   			printf("%d\n", errno);
   			if(errno == ENOENT || errno == ECONNREFUSED){
   				msleep(msec);
   				max_nsec -= (msec * pow(10, 6));
   			}else{
   				printf("Errore fatale\n");
   				return -1;
   			}
   		}
   	}
   	if(connected){
   		printf("Sono connesso al server\n");
   		return 0;
   	}
   	printf("Tempo scaduto\n");
   	errno = ETIMEDOUT;
   	return -1;
}

int closeConnection(const char* sockName){
	if(strcmp(sockName, c.socketName) != 0){
		errno = EINVAL; //argomento non valido
		return -1;
	}

	MyRequest r;
	int l, ris;

	memset(&r, 0, sizeof(MyRequest));
	r.type = CLOSE_CONN;
	r.timestamp = time(NULL);

	/*if((l = writen(comm_socket_descriptor, &r, sizeof(r))) == -1){
		errno = EAGAIN;
		return -1;
	}

	if((l = readn(comm_socket_descriptor, &ris, sizeof(int))) == -1){
		errno = EAGAIN;
		return -1;
	}*/

	do{
		if(close(comm_socket_descriptor) == -1 && errno != EINTR){
			errno = EBADR; //descrittore richiesto non valido
			return -1;
		}
	}while(errno == EINTR);
	//unlink(c.socketName);
	return 0;
}

int openFile(const char* pathname, int flags){
	MyRequest r;
	int ris, l;

	memset(&r, 0, sizeof(r));
	//r.comm_socket = comm_socket_descriptor;
	r.flags = flags;
	r.type = OPEN_FILE;
	//r.request_content = malloc(sizeof(char) * (strlen(pathname) + 1));
	strncpy(r.request_content, pathname, strlen(pathname));
	r.request_content[strlen(pathname)] = '\0';
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
			errno = EEXIST; //file già esistente(flag O_CREATE specificato)
			return -1;
			break;
		case -2:
			errno = ENOENT; //file inesistente (flag O_CREATE non specificato)
			return -1;
			break;
		default: break;
	}
	return 0;
}

int readFile(const char* pathname, void** buf, size_t* size){
	MyRequest r;
	int l;
	int ris;
	int buf_size;
	char b[4096];

	memset(&r, 0, sizeof(r));
	r.type = READ_FILE;
	r.flags = 0;
	r.timestamp = time(NULL);
	strncpy(r.request_content, pathname, strlen(pathname));
	r.request_dim = strlen(r.request_content);

	if((l = writen(comm_socket_descriptor, &r, sizeof(r))) == -1){
		printf("1\n");
		errno = EAGAIN;
		return -1;
	}

	if((l = readn(comm_socket_descriptor, &ris, sizeof(int))) == -1){
		printf("2\n");
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
		printf("3\n");
		errno = EAGAIN;
		return -1;
	}

	*size = buf_size;
	*buf = malloc(sizeof(char) * (buf_size + 1));
	if(*buf == NULL){
		return -1;
	}

	if((l = readn(comm_socket_descriptor, *buf, *size)) == -1){
		printf("4\n");
		errno = EAGAIN;
		return -1;
	}
	return 0;
}

int readNFiles(int N, const char* dirname){
	MyRequest r;
	MyFile toSave;
	int l, finished, counter = 0;

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

		toSave.content = malloc(toSave.dim * sizeof(char));

		if((l = readn(comm_socket_descriptor, toSave.content, toSave.dim)) == -1){
			errno = EAGAIN;
			return -1;
		}

		if(strcmp(dirname, "") != 0){
			if((l = saveFile(toSave, dirname)) == -1){
				perror("Errore salvataggio file!\n");
				errno = EIO; //errore I/O nel salvataggio su disco del file
				return -1;
			}
		}
		counter++;

		if((l = readn(comm_socket_descriptor, &finished, sizeof(int))) == -1){
			errno = EAGAIN;
			return -1;
		}
	}
	return counter;
}

int writeFile(const char* pathname, const char* dirname){
	char buf[1024];

	//findFile(pathname, buf);
	return 1;
}

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname){
	MyRequest r;
	int l;
	int ris;

	memset(&r, 0, sizeof(r));
	r.type = APPEND_FILE;
	r.flags = 0;
	r.timestamp = time(NULL);
	strncpy(r.request_content, pathname, strlen(pathname));
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

	printf("Buf da mandare = %s\n", (char*) buf);

	if((l = writen(comm_socket_descriptor, buf, size)) == -1){
		errno = EAGAIN;
		return -1;
	}

	printf("Esito scrittura = %d\n", l);

	printf("Leggo risultato dal server\n");

	if((l = readn(comm_socket_descriptor, &ris, sizeof(int))) == -1){
		errno = EAGAIN;
		return -1;
	}

	printf("Adesso ris = %d\n", ris);

	if(ris == -2){
		errno = EACCES; //file da scrivere in append non è stato ancora aperto
		return -1;
	}
	printf("SUCCESS\n");
	return 0;
}

int closeFile(const char* pathname){
	MyRequest r;
	int l;
	int ris;

	memset(&r, 0, sizeof(r));
	r.type = CLOSE_FILE;
	r.flags = 0;
	r.timestamp = time(NULL);
	strncpy(r.request_content, pathname, strlen(pathname));
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
	return 0;
}

int removeFile(const char* pathname){
	MyRequest r;
	int l;
	int ris;

	memset(&r, 0, sizeof(r));
	r.type = REMOVE_FILE;
	r.flags = 0;
	r.timestamp = time(NULL);
	strncpy(r.request_content, pathname, strlen(pathname));
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
	return 0;
}