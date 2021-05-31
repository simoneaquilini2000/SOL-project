/*
	Header contenente strutture e prototipi di 
	funzioni utili alla configurazione del client,
	cioè parsing delle opzioni(di configurazione e non)
	da cmd e costruzione coda delle richieste
	da spedire al server
*/

/*
	Oltre alle opzioni indicate nella specifica,
	per arrivare ad avere corrispondenza uno ad uno
	tra opzioni da riga di comando e tipi di richieste
	da mandare al server ho deciso di aggiungerne alcune mancanti:
		-o file1[,file2] -> apro i file specificati(non ho il flag O_CREAT
		quindi se non sono presenti nella cache l'operazione fallirà)

		-i dirname[,n] -> semantica analoga all'opzione -w, ma 
		riadattata con il tipo di richiesta OPEN_FILE(stavolta il flag O_CREAT
		è specificato, quindi se si tenta di creare un file già esistente oppure
		non si ha la possibilità di inserirlo l'operazione fallirà)

		-C file1[,file2] -> chiudo i file specificati, se esistono nella cache
*/
#define ALL_OPTIONS "h::p::f:w:W:r:R::d:c:t:C:i:o:"

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
int buildInsertRequest(int, char*, int);

/*
	Inserisce nella coda di richieste fino a n richieste WRITE_FILE
	per i file contenuti ricorsivamente nel rootpath(se n è specificato);
	se n=0 o non specificato leggo ed inserisco richieste WRITE_FILE
	per tutti i file nella directory rootpath(per distinguere i 2 casi
	ho l'ultimo parametro che funge da flag)
*/
int navigateFileSystem(char *, int, int, int, int);

/*
	Inserisce nella coda una richiesta di tipo READ_N_FILE 
	con eventuale parametro n nel campo request_content della struttura
*/
int buildReadNRequest(int, char*, int);

//parsa la cmd e costruisce la coda delle richieste da spedire al server
void getToSendRequestsFromCmd(int, const char* []);