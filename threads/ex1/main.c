#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

int readers_counter = 0;
pthread_mutex_t counters_mutex;
sem_t writers_mutex;
int value = 10;
int writers;
int readers;
int iterations;

void* reader(void *arg)
{
    int index = (int *) arg;

    for(int i = 0 ; i < iterations ; ++i){
        sleep(rand() % 10);

        pthread_mutex_lock(&counters_mutex);
        if(readers_counter == 0)
            sem_wait(&writers_mutex);
        
        readers_counter++;
        pthread_mutex_unlock(&counters_mutex);

        printf("READER %d: %d\n", index, value);
        sleep(rand() % 3);

        printf("READER %d: going home\n", index);
        pthread_mutex_lock(&counters_mutex);
        readers_counter--;
        if(readers_counter == 0)
            sem_post(&writers_mutex);
        pthread_mutex_unlock(&counters_mutex);
        sleep(rand() % 4);
    }
    return NULL;
}

void* writer(void *arg)
{
    int index = (int *) arg;

    for(int i = 0 ; i < iterations ; ++i){
        printf("WRITER %d wants to write\n",index);
        sem_wait(&writers_mutex);
        value = rand() % 15;
        printf("WRITER %d: %d\n",index, value);
        sem_post(&writers_mutex);
        sleep(rand() % 10);
    }
    return NULL;
}


int main(int argc, char *argv[]) {

    srand(time(0));

    
    if( argc != 4 || sscanf (argv[1], "%i", &writers) != 1 || sscanf (argv[2], "%i", &readers) != 1 || sscanf (argv[3], "%i", &iterations) != 1 ){
        printf("Wrong arguments: main (int)writers_count (int)readers_count (int)iterations\n");
        exit(1);
    }


    if (pthread_mutex_init(&counters_mutex, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        exit(1);
    }

    sem_init(&writers_mutex, 0, 1); 
    
    pthread_t* threads = (pthread_t*)malloc((readers + writers)*sizeof(pthread_t));

    for(int i = 0; i < readers; ++i){

        pthread_create(&threads[i], NULL, reader, (void*) i); 
    }
    
    for(int i = 0; i < writers; ++i){
        pthread_create(&threads[readers + i], NULL, writer, (void*) i); 
    }

    for(int i = 0; i < writers + readers; ++i)
        pthread_join(threads[i], NULL); 

    sem_destroy(&writers_mutex); 
    pthread_mutex_destroy(&counters_mutex);
    free(threads);
    return 0;
}