/*
	Header contenente strutture e prototipi di 
	funzioni utili alla configurazione del client,
	cioè parsing delle opzioni(di configurazione e non)
	da cmd e costruzione coda delle richieste
	da spedire al server
*/

#define ALL_OPTIONS "h::p::f:w:W:r:R::d:c:t:"

typedef struct i{
	int printEnable; //flag per abilitare le stampe su stdout per ogni operazione(opzione -p)
	char socketName[1024]; // path del file socket a cui connettersi (opzione -f)
	char saveReadFileDir[1024]; //path della cartella dove salvare i file letti dal server(opzione -d)
	int requestInterval; // ritardo tra due richieste successive, in millisecondi(opzione -t)
}ClientConfigInfo;

//stampa tutte le opzioni disponibili da riga di comando
void printAvailableOptions();

//restituisce struttura con opzioni di configurazioni passate
ClientConfigInfo getConfigInfoFromCmd(int, const char* []);

//stampa i parametri di configurazione
void printConfigInfo(ClientConfigInfo);

/*
	Tokenizza la stringa usando come delimitatore il carattere ','
	e crea per ognuna di queste una richiesta del tipo passato come primo parametro,
	inserendola nella coda di richieste
*/
int buildInsertRequest(int, char*);

/*
	Inserisce nella coda di richieste fino a n richieste WRITE_FILE
	per i file contenuti ricorsivamente nel rootpath(se n è specificato);
	se n=0 o non specificato leggo ed inserisco richieste WRITE_FILE
	per tutti i file nella directory rootpath(per distinguere i 2 casi
	ho l'ultimo parametro che funge da flag)
*/
int navigateFileSystem(char *, int, int);

/*
	Inserisce nella coda una richiesta di tipo READ_N_FILE 
	con eventuale parametro n nel campo request_content della struttura
*/
int buildReadNRequest(int, char*);

//parsa la cmd e costruisce la coda delle richieste da spedire al server
void getToSendRequestsFromCmd(int, const char* []);