#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include "generic_queue.h"

void defaultPrinter(void *n){
	printf("%d -> ", *(int*)n);
}

int defaultComparison(void* a, void* b){
	int valA = *(int*) a;
	int valB = *(int*) b;

	return (valA == valB); //interpreto i dati come degli interi
}

void defaultFreeFunct(void* c){
	int *x = (int*)c;

	free(x);
}

GenericQueue createQueue(int (*comp) (void*, void*), void (*print) (void*), void (*fr) (void*)){
	GenericQueue q;

	if(comp == NULL)
		q.comparison = defaultComparison;
	else
		q.comparison = comp;//setto la funzione di comparazione passata come parametro

	if(print == NULL)
		q.printFunct = defaultPrinter;
	else
		q.printFunct = print;//setto la funzione di stampa passata come parametro

	if(fr == NULL)
		q.freeFunct = defaultFreeFunct;
	else
		q.freeFunct = fr;//setto la funzione di liberazione passata come parametro

	q.queue.head = NULL;
	q.queue.tail = NULL;
	q.size = 0; //inizialmente la coda è vuota

	return q;
}

static GenericNode* createNode(void *ptr){
	if(ptr == NULL) //non posso creare un nodo con contenuto informativo NULL
		return NULL;
	GenericNode *x = (GenericNode*) malloc(sizeof(GenericNode));
	memset(x, 0, sizeof(GenericNode));
	if(x == NULL) //errore se la malloc fallisce
		return NULL;
	x->info = ptr; //assegno il puntatore alla struttura al campo info del nodo
	x->next = NULL;

	return x;
}

int push(GenericQueue *q, void *ptr){
	GenericNode *toAdd = createNode(ptr);
	//se la malloc fallisce la memoria potrebbe non essere più consistente -> ERRORE FATALE
	if(toAdd == NULL) 
		return -1;

	if(q->queue.head == NULL){//inserimento in coda quando questa è vuota
		q->queue.head = toAdd;
		q->queue.tail = q->queue.head;
	}else{ //inserimento in coda standard
		q->queue.tail->next = toAdd;
		q->queue.tail = toAdd;
	}
	q->size++;
	return 1;
}

void* pop(GenericQueue *q){
	if(isEmpty(*q) == 1){ //se la coda è vuota la pop restituisce NULL
		printf("Lista vuota: operazione di POP impossibile\n");
		return NULL;
	}

	GenericNode *result = q->queue.head;
	void* ris = result->info;

	if(q->queue.head == q->queue.tail){//rimozione in testa quando la coda ha un solo elemento
		q->queue.tail = NULL;
		q->queue.head = NULL;
	}else{
		q->queue.head = q->queue.head->next;//rimozione in testa
	}

	free(result);//libero il nodo ma non i puntatori che questo contiene
	q->size--;

	return ris;
}

void deleteElement(GenericQueue *q, void *ptr){
	if(q->queue.head == NULL){
		printf("Coda vuota\n");
		return;
	}

	GenericNode *corr = q->queue.head;
	GenericNode *prec = NULL;
	GenericNode *toDel;

	while(corr != NULL){
		if(q->comparison(corr->info, ptr) == 1){
			if(prec == NULL){//eliminazione in testa
				toDel = q->queue.head;
				q->queue.head = q->queue.head->next;
				q->freeFunct(toDel->info); //libero la struttura
				free(toDel); //libero il nodo
				corr = q->queue.head;
			}else{
				if(corr == q->queue.tail){//eliminazione in coda
					toDel = corr;
					q->queue.tail = prec;
					q->queue.tail->next = corr->next;
					corr = corr->next;
					q->freeFunct(toDel->info);
					free(toDel);
				}else{ //eliminazione "nel mezzo"
					toDel = corr;
					corr = corr->next;
					prec->next = corr;
					q->freeFunct(toDel->info);
					free(toDel);
				}
			}
			q->size--;
		}else{
			prec = corr;
			corr = corr->next;
		}
	}
}

int findElement(GenericQueue q, void *toFind){
	GenericNode *corr = q.queue.head;

	while(corr != NULL){
		if(q.comparison(corr->info, toFind) == 1)
			return 1;
		corr = corr->next;
	}
	return -1;
}

int isEmpty(GenericQueue q){
	if(q.queue.head == NULL)
		return 1;
	return 0;
}

void printQueue(GenericQueue q){
	GenericNode *corr = q.queue.head;

	while(corr != NULL){
		q.printFunct(corr->info);
		corr = corr->next;
	}
	printf("\n");
}

void freeQueue(GenericQueue *q){
	GenericNode* toDel;

	while(q->queue.head != NULL){
		toDel = q->queue.head;//mi salvo in toDel il nodo da ripulire
		q->queue.head = q->queue.head->next;
		q->freeFunct(toDel->info);
		free(toDel);
		q->size--;
	} 
}
