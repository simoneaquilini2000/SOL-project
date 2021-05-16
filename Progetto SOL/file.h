/*
	Header che definisce la struttura in memoria principale di un file
	e le operazioni che possono essere effettuate su di essi
*/

typedef struct fi{
	char filePath[2048]; //path assoluto del file
	int dim; //dimensione attuale del file
	char *content; //contenuto
	time_t timestamp; //timestamp dell'ultima modifica
	int isLocked; //è lockato?
	int isOpen; // è aperto?
	int modified; // è stato modificato?
}MyFile;

//verifica se due file sono uguali, confrontandone il filepath(assoluto)
int fileComparison(void*, void *);

//funzione per la stampa di un file
void filePrint(void*);

