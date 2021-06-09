/*
	Header contenente strutture e prototipi di 
	funzioni utili alla configurazione del client,
	cioè parsing delle opzioni(di configurazione e non)
	da cmd e costruzione coda delle richieste
	da spedire al server
*/

/*
	Oltre alle opzioni indicate nella specifica,
	per arrivare ad avere all'incirca corrispondenza uno ad uno
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
	int requestInterval; // intervallo temporale tra la spedizione di due richieste successive,
						// in millisecondi(opzione -t)
}ClientConfigInfo;

//stampa tutte le opzioni disponibili da riga di comando
void printAvailableOptions();

//restituisce struttura con opzioni di configurazioni passate
ClientConfigInfo getConfigInfoFromCmd(int, char* const*);

//stampa i parametri di configurazione
void printConfigInfo(ClientConfigInfo);

/*
	Tokenizza la stringa usando come delimitatore il carattere ','
	e crea per ognuna di queste una richiesta del tipo passato come primo parametro,
	inserendola nella coda di richieste, con i flag specificati(3° parametro)
*/
int buildInsertRequest(int, char*, int);

/*
	Inserisce nella coda di richieste fino a n richieste del tipo
	e con i flag specificati(ultimi due parametri)
	per i file contenuti ricorsivamente nel rootpath(se n è specificato);
	se n=0 o non specificato leggo ed inserisco richieste del tipo e con i
	flag specificati per tutti i file nella
	directory rootpath(per distinguere i 2 casi ho il terzo parametro che funge da flag).
*/
int navigateFileSystem(char *, int, int, int, int);

/*
	Usa la navigateFileSystem per inserire richieste
	di un certo tipo ed i rispettivi flags.
	Serve a gestire le opzioni -w e -i(aggiuntiva)
*/
int buildMultipleWriteRequest(int , char* , int);

/*
	Inserisce nella coda una richiesta di tipo READ_N_FILE 
	con eventuale parametro n nel campo request_content della struttura
	ed i flags
*/
int buildReadNRequest(int, char*, int);

/*
	Parsa la cmd e costruisce la coda delle richieste da spedire al server.
	Per tutte le opzioni dove il parametro è una stringa in cui gli argomenti sono
	separate da virgole(tipo -W) si userà la funzione buildInsertRequest con il tipo
	ed i flag della richiesta appositi;
	per quelle dove, invece, si hanno solo 2 parametri(cioè una cartella ed un parametro
	opzionale aggiuntivo come accade per -w) verrà utilizzata la buildMultipleWriteRequest;
	infine per l'opzione -R ho l'apposita funzione buildReadNRequest con flag specificati
*/
void getToSendRequestsFromCmd(int, char* const*);