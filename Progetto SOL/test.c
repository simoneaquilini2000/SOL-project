#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>

int p[2];

int celleLibere = 1;

pthread_mutex_t mux = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t pList = PTHREAD_COND_INITIALIZER;
pthread_cond_t cList = PTHREAD_COND_INITIALIZER;

static void* scriviInPipe(void* args){
    int arg = *(int*) args;

    pthread_mutex_lock(&mux);
    while(celleLibere == 0)
        pthread_cond_wait(&pList, &mux);
    celleLibere = 0;
    write(p[1], &arg, sizeof(int));
    printf("Ho scritto %d nella pipe\n", arg);
    pthread_cond_signal(&cList);
    pthread_mutex_unlock(&mux);
}

static void* leggiDaPipe(void* args){
    int rec;

    pthread_mutex_lock(&mux);
    while(celleLibere == 1)
        pthread_cond_wait(&cList, &mux);
    celleLibere = 1;
    read(p[0], &rec, sizeof(int));
    printf("Ho letto %d dalla pipe\n", rec);
    pthread_cond_signal(&pList);
    pthread_mutex_unlock(&mux);
}

int main(){
    pthread_t t1, t2;
    int N = 10;
    char a[2];
    int len = 2;

    while(N > 0){
        a = rialloca(a, len);
        N--;
    }



    /*printf("%d\n", (N>0));
    printf("%d\n", (N<=0));

    if(pipe(p) == -1)
        exit(EXIT_FAILURE);

    pthread_create(&t1, NULL, &scriviInPipe, (void*)&N);
    pthread_create(&t2, NULL, &leggiDaPipe, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);*/
}