#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

pthread_cond_t critic_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t critic_mutex = PTHREAD_MUTEX_INITIALIZER;
int readers_counter = 0;
pthread_mutex_t counters_mutex;
sem_t writers_mutex;
int value = 10;
int writers;
int readers;
int iterations;

volatile int is_critic_running = 1;

void* reader(void *arg)
{

    for(int i = 0 ; i < iterations ; ++i){
        sleep(rand() % 10);

        pthread_mutex_lock(&counters_mutex);
        if(readers_counter == 0)
            sem_wait(&writers_mutex);
        
        readers_counter++;
        pthread_mutex_unlock(&counters_mutex);

        printf("READER: %d\n", value);

        sleep(rand() % 4);

        printf("READER: gh\n");
        pthread_mutex_lock(&counters_mutex);
        readers_counter--;
        if(readers_counter == 0)
            sem_post(&writers_mutex);
        pthread_mutex_unlock(&counters_mutex);
    }
    return NULL;
}

void* critic(void *arg)
{

    while(is_critic_running){
        pthread_mutex_lock(&critic_mutex);
        pthread_cond_wait(&critic_cond, &critic_mutex);
        printf("CRITIC: %d\n", value);
        pthread_mutex_unlock(&critic_mutex);
        sem_post(&writers_mutex);
    }
    return NULL;
}


void* writer(void *arg)
{
    for(int i = 0 ; i < iterations ; ++i){
        sleep(rand() % 10);
        sem_wait(&writers_mutex);

        value = rand() % 15;

        printf("WRITER: %d\n", value);

        printf("WRITER: going home\n");

        pthread_mutex_lock(&critic_mutex);
        pthread_cond_signal(&critic_cond);
        pthread_mutex_unlock(&critic_mutex);
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
    
    pthread_t* threads = (pthread_t*)malloc((readers + writers + 1)*sizeof(pthread_t));

    for(int i = 0; i < readers; ++i){

        pthread_create(&threads[i], NULL, reader, NULL); 
    }
    
    for(int i = 0; i < writers; ++i){
        pthread_create(&threads[readers + i], NULL, writer, NULL); 
    }

    pthread_create(&threads[readers + writers], NULL, critic, NULL); 

    for(int i = 0; i < writers + readers; ++i)
        pthread_join(threads[i], NULL); 

    is_critic_running = 0;
    sem_destroy(&writers_mutex); 
    pthread_mutex_destroy(&counters_mutex);
    pthread_mutex_destroy(&critic_mutex);
    pthread_cond_destroy(&critic_cond);
    free(threads);
    return 0;
}