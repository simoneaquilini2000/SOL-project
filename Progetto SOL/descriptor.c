#include<stdio.h>
#include<stdlib.h>
#include "descriptor.h"

int descriptorComparison(void* f1, void *f2){
	MyDescriptor a = *(MyDescriptor*) f1;
	MyDescriptor b = *(MyDescriptor*) f2;

	return (a.cfd == b.cfd); //paragone solo su cfd
}

void descriptorPrint(void* c){
	MyDescriptor d = *(MyDescriptor*)c;

	printf("%d -> ", d.cfd);
}

void freeDescriptor(void* c){
	MyDescriptor *f = (MyDescriptor*)c;

	//free(f);
}