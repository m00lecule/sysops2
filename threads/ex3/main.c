#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#define RESOURCES 10


int readers_counter[RESOURCES] = {};
pthread_mutex_t counters_mutex[RESOURCES];
sem_t writers_mutex[RESOURCES];
int value[RESOURCES] = {};
int writers;
int readers;
int iterations;

void* reader(void *arg)
{

    for(int i = 0 ; i < iterations ; ++i){
        sleep(rand() % 10);

        int pick = rand() % RESOURCES;

        pthread_mutex_lock(&counters_mutex[pick]);
        if(readers_counter[pick] == 0)
            sem_wait(&writers_mutex[pick]);
        
        readers_counter[pick]++;
        pthread_mutex_unlock(&counters_mutex[pick]);

        printf("READER:from %d value %d\n",pick,  value[pick]);

        sleep(rand() % 4);

        printf("READER: %d gh\n", pick);
        pthread_mutex_lock(&counters_mutex[pick]);
        readers_counter[pick]--;
        if(readers_counter[pick] == 0)
            sem_post(&writers_mutex[pick]);
        pthread_mutex_unlock(&counters_mutex[pick]);
    }
    return NULL;
}

void* writer(void *arg)
{
    for(int i = 0 ; i < iterations ; ++i){
            
        for(int j = 0 ; j < RESOURCES ; ++j){
            printf("WRITER tries %d\n", j);

            if(sem_trywait(&writers_mutex[j]) != 0){
                continue;
            }

            value[j] = rand() % 15;

            printf("WRITER to %d : %d\n", j, value[j]);

            sleep(rand() % 10);
            sem_post(&writers_mutex[j]);
            break;
        }
    }
    return NULL;
}


int main(int argc, char *argv[]) {

    srand(time(0));

    
    if( argc != 4 || sscanf (argv[1], "%i", &writers) != 1 || sscanf (argv[2], "%i", &readers) != 1 || sscanf (argv[3], "%i", &iterations) != 1 ){
        printf("Wrong arguments: main (int)writers_count (int)readers_count (int)iterations\n");
        exit(1);
    }


    for(int i = 0 ; i < RESOURCES ; ++i){

        if (pthread_mutex_init(&counters_mutex[i], NULL) != 0)
        {
            printf("\n mutex init failed\n");
            exit(1);
        }

        if (sem_init(&writers_mutex[i], 0, 1) != 0)
        {
            printf("\n semaphore init failed\n");
            exit(1);
        }
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



    for(int i = 0 ; i < RESOURCES ; ++i){
        sem_destroy(&writers_mutex[i]); 
        pthread_mutex_destroy(&counters_mutex[i]);
    }

    free(threads);
    return 0;
}