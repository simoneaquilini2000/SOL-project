/*
	Header che definisce una richiesta, il suo formato
	ed i prototipi delle funzione utili per la sua gestione	
*/
#include<limits.h>

//enum rappresentante ogni possibile tipo di richiesta
typedef enum e{
	OPEN_FILE, //corrisponde alla richiesta di aprire un file(sia con O_CREAT che senza)
	READ_FILE, //richiesta di lettura di un file
	CLOSE_FILE, //richiesta di chiusura di un file
	REMOVE_FILE, //richiesta di rimozione di un file
	APPEND_FILE, //richiesta di scrittura in append di un file
	WRITE_FILE, //richiesta di scrittura di un file
	READ_N_FILE //richiesta di lettura di N file
}RequestType; 

typedef struct r{
	int comm_socket; //descrittore che indica il client della richiesta(settato dal server)
	int request_dim; //dimensione della richiesta
	time_t timestamp; //orario di invio della richiesta
	char request_content[PATH_MAX]; //contenuto della richiesta(argomento su cui opera)
	RequestType type; //tipo di richiesta
	int flags; //flag opzionali(O_CREAT o 0 per la OPEN_FILE)
}MyRequest;

/*
	Verifica se due richieste sono uguali,
	 confrontandone client,
	 tipo e timestamp.
	 Ritorna 1 se sono uguali, 0 altrimenti
*/
int requestComparison(void*, void *);

//stampa di una richiesta 
void requestPrint(void*);

//funzione di liberazione di memoria di una singola richiesta
void freeRequest(void*);