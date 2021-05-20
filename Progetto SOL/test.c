#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>
#include<sys/types.h>
#include<dirent.h>
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

void getAbsPathFromRelPath(char *pathname, char absPath[], int len){
    if(pathname == NULL || strcmp(pathname, "") == 0)
        return;
    char currWorkDir[1024], pathBackup[1024];
    getcwd(currWorkDir, 1024);
    strncpy(pathBackup, pathname, strlen(pathname));

    //salvo preventivamente la cwd in modo da poter tornare qui in modo sicuro

    if(pathname[0] == '/'){
        printf("Pathname è già un path assoluto\n");
        return;
    }
    //guardo se pathname inizia con /, se sì ritorno perchè pathname è già assoluto

    char *token = strtok(pathname, "/");
    char *fileName;
    //divido stringa in base a carattere '/'

    printf("Token iniziale = %s\n", token);
    //printf("Pathname attuale = %s\n", pathname);

    if(strcmp(token, pathBackup) == 0){ //non ho '/' quindi ho un solo token
        printf("Path senza / intermedi\n");
        getcwd(absPath, len);
        if(strlen(absPath) + strlen(pathBackup) + 2 < len){
            strncat(absPath, "/", 1);
            strncat(absPath, pathBackup, strlen(pathBackup));
        }
        return;
    }

    while(token != NULL){
        printf("%s\n", token);
        fileName = malloc(strlen(token) + 1);
        strncpy(fileName, token, strlen(token));
        //per ogni token cambio cwd nella cartella indicata dal token e mi salvo il precedente
        if(chdir(token) == -1){
            printf("Schianto\n");
            break;
        }
        token = strtok(NULL, "/");
        if(token != NULL)
            free(fileName);
    }
    //una volta finito il ciclo metto in un buffer la cwd e ci appendo il nome del file(quello sarà il mio path assoluto)
    getcwd(absPath, len);
    if(strlen(absPath) + strlen(fileName) + 2 < len){
        strncat(absPath, "/", 1);
        strncat(absPath, fileName, strlen(fileName));
    }

    chdir(currWorkDir); //torno a cwd iniziale
}

int main(){
    pthread_t t1, t2;
    int N = 10;
    char a[2], buf[PATH_MAX];
    char input[1000] = "../../../Calcolo Numerico/../Basi di Dati/Dispensa_BD.pdf";
    int len = 2;

    getAbsPathFromRelPath(input, buf, PATH_MAX);
    printf("%s\n", buf);
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