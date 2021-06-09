/*
	Header di utilità, quali funzioni per la formattazione del testo
	e/o di lettura/scrittura
*/

//funzione per attendere X msec
void msleep(int);

//utility per eliminare caratteri '\r' e '\n' da una stringa
void myStrNCpy(char *, char *, int);

/*
	trasformazione da stringa a numero, se possibile:
	ritorna < 0 in caso di errore e setta errno, il valore altrimenti
*/
long isNumber(const char*);

//ripulisce il buffer
void clearBuffer(char[], int);

/* Read "n" bytes from a descriptor */
int readn(long fd, void *buf, size_t size);

/* Write "n" bytes to a descriptor */
int writen(long fd, void *buf, size_t size); 
