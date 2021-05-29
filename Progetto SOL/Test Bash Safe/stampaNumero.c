#include<stdio.h>
#include<stdlib.h>

int main(int argc, char *argv[]){
    if(argc < 2)
        exit(EXIT_FAILURE);
    int N = atoi(argv[1]);
    printf("Mi hai dato il numero %d\n", N);
    return 0;
}