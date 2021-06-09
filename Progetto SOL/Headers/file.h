/*
	Header che definisce la struttura in memoria principale di un file
	e le operazioni che possono essere effettuate su di essi
*/
#include<limits.h>

typedef struct lso{
	int opType; // tipo operazione(prenderà i valori di RequestType, vedi request.h)
	int optFlags; // flag opzionali
	int clientDescriptor; // descrittore(lato server) sul quale il client ha mandato la richiesta
}LastSuccessfulOperation;

typedef struct fi{
	char filePath[PATH_MAX]; //path assoluto del file
	int dim; //dimensione attuale del file
	char *content; //contenuto
	time_t timestamp; //timestamp dell'ultima modifica
	int isLocked; //è lockato?
	int isOpen; // è aperto?
	int modified; // è stato modificato?
	LastSuccessfulOperation lastSucceedOp; //ultima operazione eseguita con successo su tale file
}MyFile;

/*
	Verifica se due file sono uguali, confrontandone il filepath(assoluto).
	Ritorna 1 se sono lo stesso file, 0 altrimenti
*/
int fileComparison(void*, void *);

//funzione per la stampa di un file
void filePrint(void*);

//funzione di liberazione di memoria di un singolo file
void freeFile (void*);

/*
	Funzione che estrapola il nome del file da un path
	tokenizzandolo in base al carattere '/' e prendendo
	come nome del file l'ultimo token != NULL.
	Se il path è NULL, la funzione restituirà NULL.
*/
char* getFileNameFromPath(char[]);

//funzione che esegue il salvataggio di un file in una directory
//Ritorna -1 in caso di errore(file inesistente, errori nel cambio di directory ecc...)
//1 altrimenti
int saveFile(MyFile, const char[]);

/*
	funzione che restituisce, nel secondo parametro,
	l'intero contenuto del file specificato come primo parametro.
	Ritorna -1 in caso di errore, il numero di byte letti altrimenti
*/
int readFileContent(const char *, char **);

/*
	Funzione di traduzione da path relativo ad assoluto
	(il risultato sarà salvato nel secondo parametro, la cui lunghezza è
	rappresentata dal terzo).
	In caso di errore il risultato sarà una stringa vuota.
*/
void getAbsPathFromRelPath(char *, char[], int);

