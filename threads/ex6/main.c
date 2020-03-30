#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#define BUFFOR_SIZE 10


int index = 0;
int readers_counter = 0;

pthread_mutex_t writers_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t readers_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t writers_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t readers_cond = PTHREAD_COND_INITIALIZER;

sem_t writers_mutex;

int value[BUFFOR_SIZE];
int writers;
int readers;
int iterations;

void* reader(void *arg)
{

    for(int i = 0 ; i < iterations ; ++i){
    
        pthread_mutex_lock(&readers_mut);
        pthread_cond_wait(&readers_cond, &readers_mut);

        index--;

        printf("READER: %d at index %d\n", value[index], index);

        if(index == 0){
            printf("BUFOR IS EMPTY\n");
            pthread_cond_signal(&writers_cond);
        }else{
            pthread_cond_signal(&readers_cond);
        }
        pthread_mutex_unlock(&readers_mut);
    }
    return NULL;
}

void* writer(void *arg)
{
    for(int i = 0 ; i < iterations ; ++i){

        pthread_mutex_lock(&writers_mut);

        value[index] = rand() % 15;

        printf("WRITER: %d at i: %d\n", value[index], index);

        if((index++) == BUFFOR_SIZE){
            printf("BUFOR IS FULL\n");
            pthread_mutex_lock(&readers_mut);
            pthread_cond_signal(&readers_cond);
            pthread_cond_wait(&writers_cond, &readers_mut);
            pthread_mutex_unlock(&readers_mut);
        }
        pthread_mutex_unlock(&writers_mut);
    }
    return NULL;
}


int main(int argc, char *argv[]) {

    srand(time(0));

    
    if( argc != 4 || sscanf (argv[1], "%i", &writers) != 1 || sscanf (argv[2], "%i", &readers) != 1 || sscanf (argv[3], "%i", &iterations) != 1 ){
        printf("Wrong arguments: main (int)writers_count (int)readers_count (int)iterations\n");
        exit(1);
    }
    
    pthread_t* threads = (pthread_t*)malloc((readers + writers)*sizeof(pthread_t));

    for(int i = 0; i < readers; ++i){

        pthread_create(&threads[i], NULL, reader, NULL); 
    }
    
    for(int i = 0; i < writers; ++i){
        pthread_create(&threads[readers + i], NULL, writer, NULL); 
    }

    for(int i = 0; i < writers + readers; ++i)
        pthread_join(threads[i], NULL); 


    pthread_mutex_destroy(&writers_mut);
    pthread_mutex_destroy(&readers_mut);
    pthread_cond_destroy(&writers_cond);
    pthread_cond_destroy(&readers_cond);
    free(threads);
    return 0;
}