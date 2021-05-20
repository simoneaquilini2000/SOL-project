/*
	Header di utilità, quali funzioni per la formattazione del testo
	e/o di lettura/scrittura
*/

//funzione per attendere X msec
void msleep(int);

//utility per eliminare caratteri '\r' e '\n' da una stringa
void myStrNCpy(char *, char *, int);

//ripulisce il buffer
void clearBuffer(char[], int);

/* Read "n" bytes from a descriptor */
int readn(long fd, void *buf, size_t size);

/* Write "n" bytes to a descriptor */
int writen(long fd, void *buf, size_t size); 
