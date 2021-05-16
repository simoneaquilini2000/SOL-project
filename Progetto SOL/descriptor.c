#include<stdio.h>
#include "descriptor.h"

int descriptorComparison(void* f1, void *f2){
	MyDescriptor a = *(MyDescriptor*) f1;
	MyDescriptor b = *(MyDescriptor*) f2;

	return (a.cfd == b.cfd);
}

void descriptorPrint(void* c){
	MyDescriptor d = *(MyDescriptor*)c;

	printf("%d -> ", d.cfd);
}