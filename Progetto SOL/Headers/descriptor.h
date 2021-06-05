/*
	Header che definisce la struttura che rappresenta una
	connessione al server
*/

typedef struct i{
	//int client_pid; //PID del client che ha effettuato la connessione
	int cfd; //descrittore dove avviene lo scambio di dati(lato server, cioè il
			// descrittore restituito dalla accept)
}MyDescriptor;



//verifica se due descrittori sono uguali, confrontandone il communication descriptor
int descriptorComparison(void*, void *);

//esegue la stampa di un descrittore della coda
void descriptorPrint(void*);

//funzione per la liberazione in memoria di nodo rappresentante il descrittore
void freeDescriptor(void*);