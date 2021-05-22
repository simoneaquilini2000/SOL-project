#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>
#include<sys/types.h>
#include<dirent.h>
#include "generic_queue.h"
#include "file.h"
#include<unistd.h>

GenericQueue coda;

void insert(){
    MyFile f1, f2, f3;
    memset(&f1, 0, sizeof(f1));
    memset(&f2, 0, sizeof(f2));
    memset(&f3, 0, sizeof(f3));
    //memset(&f4, 0, sizeof(f4));

    strcpy(f1.filePath, "Babbo.txt");
    strcpy(f2.filePath, "Mamma.txt");
    strcpy(f3.filePath, "Davide.txt");

    MyFile *pf1 = malloc(sizeof(MyFile));
    memset(pf1, 0, sizeof(MyFile));
    memcpy(pf1, &f1, sizeof(MyFile));

    MyFile *pf2 = malloc(sizeof(MyFile));
    memset(pf2, 0, sizeof(MyFile));
    memcpy(pf2, &f2, sizeof(MyFile));

    MyFile *pf3 = malloc(sizeof(MyFile));
    memset(pf3, 0, sizeof(MyFile));
    memcpy(pf3, &f3, sizeof(MyFile));

    //strcpy(f4.filePath, "Simone.txt");

    push(&coda, (void*)pf1);
    push(&coda, (void*)pf2);
    push(&coda, (void*)pf3);

    //free(pf1);
    //free(pf2);
    //free(pf3);
    //push(&coda, (void*)&f4);
}

int main(){
    MyFile f2;
    memset(&f2, 0, sizeof(MyFile));
    strcpy(f2.filePath, "Mamma.txt");

    coda = createQueue(&fileComparison, &filePrint, &freeFile);
    insert();

    printf("Stampo coda attuale:\n");
    printQueue(coda);

    deleteElement(&coda, (void*)&f2);

    printf("Stampo coda modificata:\n");
    printQueue(coda);

    freeQueue(&coda);

    //printf("Stampo la testa della coda: \n");
    //coda.printFunct(coda.queue.head);
    //printf("Stampo coda originale:\n");
    //printQueue(coda2);
    return 0;



    /*printf("%d\n", (N>0));
    printf("%d\n", (N<=0));

    if(pipe(p) == -1)
        exit(EXIT_FAILURE);

    pthread_create(&t1, NULL, &scriviInPipe, (void*)&N);
    pthread_create(&t2, NULL, &leggiDaPipe, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);*/
}