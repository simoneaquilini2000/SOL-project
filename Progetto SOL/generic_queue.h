//header che rappresenta strutture e operazioni su una coda generica

typedef struct n{
	void *info; //contenuto informativo del nodo
	struct n *next; //puntatore al successivo
}GenericNode; //nodo generico

typedef struct q{ 
	GenericNode *head; //testa della coda
	GenericNode *tail; //fine della coda
}Queue; //per avere inserimento in coda e rimozione in testa in O(1) mi mantengo i puntatori alla testa ed alla fine della coda

typedef struct a{
	Queue queue; //struttura che rappresenta la coda
	int (*comparison) (void*, void*); //funzione di comparazione tra 2 elementi
	void (*printFunct) (void*); //funzione di stampa delle info di un singolo nodo
}GenericQueue; //in quanto generica nella coda va specificata la funzione di comparazione di due elementi


//funzione di comparazione di default(interpreta gli elementi come interi)
int defaultComparison(void*, void*);

//funzione di default per la stampa di un elmento
void defaultPrinter(void*);

/*
   crea coda generica e se f == NULL usa come funzione di comparazione "defaultComparison",
   mentre se s == NULL usa come funzione di stampa "defaultPrinter"
*/
GenericQueue createQueue(int (*) (void*, void*), void (*) (void*));

//crea un nodo generico
GenericNode* createNode(void *);

/*
inserimento in coda di un nuovo elemento:
 restituisce -1 se la malloc fallisce, 1 altrimenti
*/
int push(GenericQueue*, void *);

/*estrazione in testa:
	ritorna NULL se la coda è vuota, l'elemento di testa altrimenti
*/
void* pop(GenericQueue*);

//cancella un elemento ricercato dalla coda
void deleteElement(GenericQueue*, void*);

//cerca un elemento nella lista(1 se lo trova, -1 altrimenti)
int findElement(GenericQueue, void*);

//verifica se la coda è vuota o meno(1 se lo è, 0 altrimenti)
int isEmpty(GenericQueue);

//stampa della coda
void printQueue(GenericQueue);

//pulizia della lista
void freeQueue(GenericQueue*);