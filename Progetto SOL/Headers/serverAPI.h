/*
	Header contenente i prototipi delle funzioni che il
	client usa per comunicare con il server
*/

/*
	Apre il socket per comunicare con il server e se riesce a connettersi
	prima che sia passato un certo limite temporale(specificato nel 3° parametro)
	ritorna 0.
	In caso di errore ritorna -1 e setta errno:
		errno = ETIMEDOUT -> tempo scaduto per la connesione
		errno = EINVAL -> nome del socket da aprire diverso da quello specificato in configurazione
		errno = EPERM -> sono già connesso al server, non posso connettermi una seconda volta
*/
int openConnection(const char*, int, const struct timespec);

/*
	Chiude il socket aperto per comunicare con il server;
	ritorna 0 in caso di successo, -1 in caso di errore e setta errno:
		errno = EBADR -> la close ha fallito non a causa di una interruzione
		errno = EINVAL -> nome del socket da chiudere diverso da quello specificato in configurazione
		errno = EPERM -> non posso disconnettermi dal server se non ero precendentemente connesso
*/
int closeConnection(const char*);

/*
	Costruisce richiesta di apertura di un file e la spedisce al server.
	Primo parametro = file su cui operare; Secondo parametro = 0 oppure O_CREAT(vedi fcntl.h)

	Ritorna 0 in caso di successo, -1 in caso di errore e setta errno:
		errno = EAGAIN -> errore nella read o write della richiesta o risposta
		errno = EEXIST -> O_CREAT è specificato, ma il file su cui operare è già presente in cache
		errno = ENOENT -> O_CREAT non specificato ed il file su cui operare non è presente in cache
		errno = EINVAL -> flag diverso sia da 0 che da O_CREAT oppure file su cui operare == NULL
		errno = EPERM -> file da aprire è già aperto
		errno = ENOSPC -> l'operazione non è stata effettuata con successo perchè la sua esecuzione causerebbe
						  il fallimento dell'algoritmo di rimpiazzamento, in quanto c'è il bisogno
						  (in seguito all'operazione richiesta) di rimpiazzare file, ma non ce n'è
						  l'effettiva possibilità
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
	parametro(se != NULL e != "").Ritorna il numero di file letti con successo,
	-1 altrimenti e setta errno:
		errno = EAGAIN -> errore di read/write
		errno = EIO -> cartella dove salvare i file inesistente od errore durante il salvataggio del file
*/
int readNFiles(int, const char*);

/*
	Scrive il file(il cui path è passato come primo parametro) nel server.
	Ritorna 0 in caso di succeso, -1 altrimenti e setta errno:
		errno = ENOENT -> file da scrivere non trovato lato server
		errno = EIO -> errore nella lettura del file(file non trovato ecc...)
		errno = EAGAIN -> errore di read/write
		errno = EACCES -> file non aperto
		errno = EPERM -> prima devi eseguire con successo openFile(pathName, O_CREAT)
		errno = ENOSPC -> scrittura non effettuata in quanto causerebbe
						  il fallimento dell'algoritmo di rimpiazzamento
*/
int writeFile(const char*, const char*);

/*
	Estende il contenuto del file(passato come 1° parametro) con
	i dati nel buffer.Ritorna 0 in caso di successo, -1 altrimenti e setta
	errno:
		errno = EAGAIN -> errore read/write
		errno = EACCES -> file da appendere non aperto
		errno = ENOENT -> file specificato non presente in cache
		errno = ENOSPC -> operazione non eseguita in quanto la sua esecuzione
						  comporterebbe il fallimento dell'algoritmo di rimpiazzamento 
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
	Rimuove il file passato come parametro, se questo è in stato di locked
	Ritorna 0 in caso di successo, -1 e setta errno altrimenti:
		errno = EAGAIN -> errori in read/write
		errno = ENOENT -> file da rimuovere non presente nel server
		errno = EPERM -> file da rimuovere non in stato di locked  	
*/
int removeFile(const char*);