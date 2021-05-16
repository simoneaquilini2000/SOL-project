#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<unistd.h>
#include<errno.h>
#include "generic_queue.h"
#include "request.h"
#include "file.h"

int fileComparison(void* f1, void *f2){
	MyFile a = *(MyFile*) f1;
	MyFile b = *(MyFile*) f2;

	if(strcmp(a.filePath, b.filePath) == 0)
		return 1;
	return 0;
}

void filePrint(void* info){
	if(info == NULL)
		printf("BAB\n");
	MyFile f = *(MyFile*) info;
	char buff[20];

	//ottengo in buff una stringa nel formato Y-m-d H:M:S del timestamp
	strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&f.timestamp));

	printf("FilePath: %s\n", f.filePath);
	printf("Dimension: %d\n", f.dim);
	printf("Content: %d\n", (int)f.content[1]);
	printf("Timestamp: %s\n", buff);
	printf("Locked: %d\n", f.isLocked);
	printf("Open: %d\n", f.isOpen);
	printf("Modified: %d\n\n", f.modified);
}

