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
	int (*comparison) (void*, void*); //funzione di comparazione tra 2 elementi(ritorna 1 se uguali, 0 altrimenti)
	void (*printFunct) (void*); //funzione di stampa delle info di un singolo nodo
	void (*freeFunct) (void*); //funzione di liberazione della memoria di un singolo nodo
	int size; //cardinalità attuale della coda
}GenericQueue; //in quanto generica nella coda va specificata la funzione di comparazione di due elementi


//funzione di comparazione di default(interpreta gli elementi come interi)
int defaultComparison(void*, void*);

//funzione di default per la stampa di un elmento
void defaultPrinter(void*);

//funzione di liberazione di memoria di default
void defaultFreeFunct(void*);

/*
   crea coda generica e se comp == NULL usa come funzione di comparazione "defaultComparison",
	se print == NULL usa come funzione di stampa "defaultPrinter",
	se fr == NULL usa come funzione di liberazione "defaultFreeFunct"
*/
GenericQueue createQueue(int (*) (void*, void*), void (*) (void*), void (*) (void*));

//crea un nodo generico
static GenericNode* createNode(void *);

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

//cerca un elemento nella coda(1 se lo trova, -1 altrimenti)
int findElement(GenericQueue, void*);

//verifica se la coda è vuota o meno(1 se lo è, 0 altrimenti)
int isEmpty(GenericQueue);

//stampa della coda
void printQueue(GenericQueue);

//pulizia della coda
void freeQueue(GenericQueue*);