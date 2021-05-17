/*
	Header di utilità, quali parametri di configurazione
	e/o funzioni per la formattazione del testo
*/

#define N_INFO 5 //numero di righe da parsare dal file di testo

typedef struct s{
	size_t maxStorageSpace; //massimo spazio di configurazione
	int nMaxFile; //massimo numero di file memorizzabili
	int nWorkers; //numero di thread workers
	char socketPath[1024]; //path del socket
	char logFilePath[1024]; //path del file di log
}serverInfo;

/*
	Parsa il file di configurazione e restituisce
	una struttura contenente tutte le info parsate
*/
serverInfo startConfig(const char*);

//utility per eliminare caratteri '\r' e '\n' da una stringa
void myStrNCpy(char *, char *, int);

//ripulisce il buffer
void clearBuffer(char[], int);

/* Read "n" bytes from a descriptor */
int readn(long fd, void *buf, size_t size);

/* Write "n" bytes to a descriptor */
int writen(long fd, void *buf, size_t size); 
