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

int celleLibere = 10;

int prodActive = 1;

pthread_mutex_t flagMux = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mux = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t varcondProd = PTHREAD_COND_INITIALIZER;

pthread_cond_t varcondCons = PTHREAD_COND_INITIALIZER;

static void* scrivi(){
    while(1){
        pthread_mutex_lock(&mux);
        while(celleLibere == 0)
            pthread_cond_wait(&varcondProd, &mux);
        //pthread_mutex_lock(&flagMux);
        if(prodActive == 0){
            //pthread_mutex_unlock(&flagMux);
            break;
        }
        //pthread_mutex_unlock(&flagMux);
        celleLibere--;
        //printf("CelleLibere = %d\n", celleLibere);
        pthread_cond_signal(&varcondCons);
        pthread_mutex_unlock(&mux);
    }
    printf("Sono worker esco!\n");
    return NULL;
}

static void* leggi(){
    int letti = 0;

    while(1){
        pthread_mutex_lock(&mux);
        while(celleLibere == 10)
            pthread_cond_wait(&varcondCons, &mux);
        celleLibere++;
        //printf("CelleLibere = %d\n", celleLibere);
        letti++;
        pthread_cond_signal(&varcondProd);
        pthread_mutex_unlock(&mux);
        if(letti > 2)
            break;
    }
    pthread_mutex_lock(&mux);
    prodActive = 0;
    pthread_mutex_unlock(&mux);
    pthread_cond_broadcast(&varcondProd);

    printf("Sono manager esco!\n");
    return NULL;
}

int main(){
    pthread_t cons;
    pthread_t prods[10];

    pthread_create(&cons, NULL, &leggi, NULL);

    for(int i = 0; i < 10; i++)
        pthread_create(&prods[i], NULL, &scrivi, NULL);

    pthread_join(cons, NULL);
    /*for(int i = 0; i < 10; i++){
         pthread_cond_signal(&varcondProd);
    }*/
    for(int i = 0; i < 10; i++){
        //pthread_cond_signal(&varcondProd);
        pthread_join(prods[i], NULL);
    }
    return 0;
}