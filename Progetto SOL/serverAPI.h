/*
	Header contenente i prototipi delle funzioni che il
	client usa per comunicare con il server
*/

/*
	Apre il socket per comunicare con il server e se riesce a connettersi
	prima che sia passato un certo limite temporale(specificato nel 3° parametro)
	ritorna 0.
	Incaso di errore ritorna -1 e setta errno:
		errno = ETIMEDOUT -> tempo scaduto per la connesione
		errno = EINVAL -> nome del socket da aprire diverso da quello specificato in configurazione
*/
int openConnection(const char*, int, const struct timespec);

/*
	Chiude il socket aperto per comunicare con il server;
	ritorna 0 in caso di successo, -1 in caso di errore e setta errno:
		errno = EBADR -> la close ha fallito non a causa di una interruzione
		errno = EINVAL -> nome del socket da chiudere diverso da quello specificato in configurazione
*/
int closeConnection(const char*);

/*
	Costruisce richiesta di apertura di un file e la spedisce al server.
	Primo parametro = file su cui operare; Secondo parametro = 0 oppure O_CREATE

	Ritorna 0 in caso di successo, -1 in caso di errore e setta errno:
		errno = EAGAIN -> errore nella read o write della richiesta o risposta
		errno = EEXIST -> O_CREATE è specificato, ma il file su cui operare è già presente in cache
		errno = ENOENT -> O_CREATE non specificato ed il file su cui operare non è presente in cache
		errno = EINVAL -> flag diverso sia da 0 che da O_CREAT
		errno = EPERM -> file da aprire è già aperto
*/
int openFile(const char*, int);

/*
	Legge il file(primo parametro) dal server e ne
	salva il contenuto nel puntatore(allocato sullo heap) passato
	come secondo parametro e memorizza nel terzo la dimensione.

	Ritorna 0 in caso di successo, -1 in caso di errore e setta errno:
		errno = EAGAIN -> errori in read/write
		errno = ENOENT -> file da leggere non presente nel server
		errno = EACCES -> ho tentato di leggere un file non aperto in precedenza
*/
int readFile(const char*, void**, size_t*);

/*
	Legge N file dal server(se N <= 0 o > del numero di file nel server
	li legge tutti) e li memorizza nella cartella passata come secondo 
	parametro(se != NULL).Ritorna il numero di file letti con successo,
	-1 altrimenti e setta errno:
		errno = EAGAIN -> errore di read/write
		errno = EIO -> cartella dove salvare i file inesistente
*/
int readNFiles(int, const char*);

/*
	Scrive il file(il cui path è passato come primo parametro) nel server.
	Ritorna 0 in caso di succeso, -1 altrimenti e setta errno:
		errno = ENOENT -> file da scrivere non trovato lato client o lato server
		errno = EAGAIN -> errore di read/write
		errno = EACCES -> file non aperto
		errno = EPERM -> prima devi eseguire con successo openFile(pathName, O_CREAT)
*/
int writeFile(const char*, const char*);

/*
	Estende il contenuto del file(passato come 1° parametro) con
	i dati nel buffer.Ritorna 0 in caso di successo, -1 altrimenti e setta
	errno:
		errno = EAGAIN -> errore read/write
		errno = EACCES -> file da appendere non aperto
		errno = ENOENT -> file specificato non presente in cache
*/
int appendToFile(const char*, void*, size_t, const char*);

/*
	Chiude il file passato come parametro.
	Ritorna 0 in caso di successo, -1 e setta errno altrimenti:
		errno = EAGAIN -> errori in read/write
		errno = ENOENT -> file da leggere non presente nel server
		errno = EPERM -> file da chiudere già chiuso 
*/
int closeFile(const char*);

/*
	Rimuove il file passato come parametro.
	Ritorna 0 in caso di successo, -1 e setta errno altrimenti:
		errno = EAGAIN -> errori in read/write
		errno = ENOENT -> file da rimuovere non presente nel server
		errno = EPERM -> file da rimuovere non in stato di locked  	
*/
int removeFile(const char*);