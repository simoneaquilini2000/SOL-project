#include<stdio.h>
#include<string.h>
#include<time.h>
#include<stdlib.h>
#include "request.h"

int requestComparison(void* r1, void *r2){
	MyRequest req1 = *(MyRequest*)r1;
	MyRequest req2 = *(MyRequest*)r2;

	/*
	 	confronto descrittore su cui avviene la comunicazione,
		il tipo di richiesta ed il timestamp
	*/
	if(req1.comm_socket == req2.comm_socket && req1.type == req2.type && \
		difftime(req1.timestamp, req2.timestamp) == 0)
		return 1;
	return 0;
}

void requestPrint(void* c){
	if(c == NULL)
		return;
	MyRequest d = *(MyRequest*)c;
	char buff[20];

	//ottengo in buff una stringa nel formato Y-m-d H:M:S del timestamp
	strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&d.timestamp));

	printf("Descrittore sul quale avviene la comunicazione: %d\n", d.comm_socket);
	printf("Dimensione richiesta: %d\n", d.request_dim);
	printf("Contenuto della richiesta: %s\n", d.request_content);
	printf("Timestamp dell'invio della richiesta: %s\n", buff);
	printf("Flags della richiesta: %d\n", d.flags);
	printf("Tipo richiesta: %d\n\n", d.type);
}

void freeRequest(void* c){
	MyRequest *f = (MyRequest*)c;

	free(f);
}