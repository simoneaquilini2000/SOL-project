#include<stdio.h>
#include<stdlib.h>
#include "generic_queue.h"

void defaultPrinter(void *n){
	printf("%d -> ", *(int*)n);
}

int defaultComparison(void* a, void* b){
	int valA = *(int*) a;
	int valB = *(int*) b;

	return (valA == valB);
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
		q.comparison = comp;

	if(print == NULL)
		q.printFunct = defaultPrinter;
	else
		q.printFunct = print;

	if(fr == NULL)
		q.freeFunct = defaultFreeFunct;
	else
		q.freeFunct = fr;

	q.queue.head = NULL;
	q.queue.tail = NULL;
	q.size = 0;

	return q;
}

GenericNode* createNode(void *ptr){
	GenericNode *x = malloc(sizeof(GenericNode));
	if(x == NULL)
		return NULL;
	x->info = ptr;
	x->next = NULL;

	return x;
}

int push(GenericQueue *q, void *ptr){
	GenericNode *toAdd = createNode(ptr);
	if(toAdd == NULL)
		return -1;

	if(q->queue.head == NULL){
		q->queue.head = toAdd;
		q->queue.tail = q->queue.head;
	}else{
		q->queue.tail->next = toAdd;
		q->queue.tail = toAdd;
	}
	q->size++;
	return 1;
}

void* pop(GenericQueue *q){
	//printf("Esito isEmpty = %d\n", isEmpty(*q));
	if(isEmpty(*q) == 1){
		printf("Ciao\n");
		return NULL;
	}

	GenericNode *result = q->queue.head;
	void* ris = result->info;

	if(q->queue.head == q->queue.tail){
		q->queue.tail = NULL;
		q->queue.head = NULL;
	}else{
		q->queue.head = q->queue.head->next;
	}

	free(result);
	q->size--;
	/*if(ris == NULL)
		printf("BAB\n");
	else
		printf("BOB\n");*/

	//printf("Stampo roba estratta\n");
	//q->printFunct(ris);

	return ris;
}

void deleteElement(GenericQueue *q, void *ptr){
	if(q->queue.head == NULL)
		return;

	GenericNode *corr = q->queue.head;
	GenericNode *prec = NULL;
	GenericNode *toDel;

	while(corr != NULL){
		if(q->comparison(corr->info, ptr) == 1){
			if(prec == NULL){
				toDel = corr;
				corr = corr->next;
				q->freeFunct(toDel->info);
				free(toDel);
			}else{
				toDel = corr;
				corr = corr->next;
				prec->next = corr;
				q->freeFunct(toDel->info);
				free(toDel);
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
		//printf("sto stampando\n");
		q.printFunct(corr->info);
		corr = corr->next;
	}
	printf("\n");
}

void freeQueue(GenericQueue *q){
	if(q->comparison != NULL)
		free(q->comparison);
	if(q->printFunct != NULL)
		free(q->printFunct);

	GenericNode* corr = q->queue.head;
	GenericNode* toDel;

	while(corr != NULL){
		toDel = corr;
		corr = corr->next;
		q->freeFunct(toDel->info);
		free(toDel);
		q->size--;
	}

	if(q->freeFunct != NULL)
		free(q->freeFunct); 
}
