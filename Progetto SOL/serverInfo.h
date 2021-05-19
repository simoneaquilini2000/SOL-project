/*
    Header contenente le strutture che contengono informazioni
    riguardo il server(sia statistiche che di configurazione) e
    dichiarazione delle funzioni su esse operanti
*/

#define N_INFO 5 //numero di righe da parsare dal file di testo

typedef struct s{
	size_t maxStorageSpace; //massimo spazio di configurazione
	int nMaxFile; //massimo numero di file memorizzabili
	int nWorkers; //numero di thread workers
	char socketPath[1024]; //path del socket
	char logFilePath[1024]; //path del file di log
}serverInfo;

typedef struct stats{
    int fileCacheMaxSize; //numero di file massimo memorizzato nel server
    size_t fileCacheMaxStorageSize; //dimensione massima raggiunta sommando il totale delle dimensioni dei singoli file nello storage
    int replaceAlgInvokeTimes; //numero di volte che l'algoritmo di rimpiazzamento è stato invocato per selezionare file da rimuovere
    int fileCacheActStorageSize; //dimensione attuale dello storage(come somma delle dimensioni dei file presenti)
    //per stampare lista dei file rimanenti basta stampare in ME la fileCache
}serverStats;

/*
	Parsa il file di configurazione e restituisce
	una struttura contenente tutte le info parsate
*/
serverInfo startConfig(const char*);

//stampa parametri di configurazione
void printConfig(serverInfo);

//stampa statistiche su operato del server
void printServerStats(serverStats);