#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>
#include<sys/types.h>
#include<dirent.h>
#include<pthread.h>
#include "generic_queue.h"
#include "file.h"
#include<unistd.h>

int main(){
    MyFile f1;
    memset(&f1, 0, sizeof(f1));
    GenericQueue g = createQueue(&fileComparison, &filePrint, &freeFile);

    push(&g, (void*) &f1);

    printQueue(g);

    return 0;
}