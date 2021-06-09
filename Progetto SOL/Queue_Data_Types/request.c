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

	printf("Communication descriptor: %d\n", d.comm_socket);
	printf("Request Dimension: %d\n", d.request_dim);
	printf("Request content: %s\n", d.request_content);
	printf("Request send timestamp: %s\n", buff);
	printf("Request flags: %d\n", d.flags);
	printf("Type = %d\n\n", d.type);
}

void freeRequest(void* c){
	MyRequest *f = (MyRequest*)c;

	free(f);
}