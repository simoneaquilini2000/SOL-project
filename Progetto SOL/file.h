/*
	Header che definisce la struttura in memoria principale di un file
	e le operazioni che possono essere effettuate su di essi
*/

typedef struct lso{
	int opType; // tipo operazione(prenderà i valori di RequestType, vedi request.h)
	int optFlags; // flag opzionali
	int clientDescriptor; // descrittore(lato server) sul quale il client ha mandato la richiesta
}LastSuccessfulOperation;

typedef struct fi{
	char filePath[2048]; //path assoluto del file
	int dim; //dimensione attuale del file
	char *content; //contenuto
	time_t timestamp; //timestamp dell'ultima modifica
	int isLocked; //è lockato?
	int isOpen; // è aperto?
	int modified; // è stato modificato?
	LastSuccessfulOperation lastSucceedOp; //ultima operazione eseguita con successo su tale file
}MyFile;

//verifica se due file sono uguali, confrontandone il filepath(assoluto)
int fileComparison(void*, void *);

//funzione per la stampa di un file
void filePrint(void*);

//funzione di liberazione di memoria di un singolo file
void freeFile (void*);

//funzione che esegue il salvataggio di un file in una directory
int saveFile(MyFile, const char[]);

char* readFileContent(const char *, char **);

