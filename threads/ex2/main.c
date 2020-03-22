#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#define BUFFOR_SIZE 10

struct buffor_t {
    int buffor[BUFFOR_SIZE];
    int head_iterator;
    pthread_mutex_t mutex_inside;
    pthread_mutex_t mutex_outside;
};

void init_buffor(struct buffor_t* buffor){
    for(int i = 0 ; i < BUFFOR_SIZE; ++i)
        buffor->buffor[i]=0;

    buffor->head_iterator=-1;

     if (pthread_mutex_init(&(buffor->mutex_outside), NULL) != 0)
    {
        printf("\n mutex init failed\n");
        exit(1);
    }

    if (pthread_mutex_init(&(buffor->mutex_inside), NULL) != 0)
    {
        printf("\n mutex init failed\n");
        exit(1);
    }
}


volatile int buffor_running = 1;
struct buffor_t buff;
int readers_counter = 0;
pthread_mutex_t counters_mutex;
sem_t writers_mutex;
int value = 10;
int writers;
int readers;
int iterations;

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

        printf("READER: going home\n");
        pthread_mutex_lock(&counters_mutex);
        readers_counter--;
        if(readers_counter == 0)
            sem_post(&writers_mutex);
        pthread_mutex_unlock(&counters_mutex);
    }
    return NULL;
}

void* buffor(void *arg)
{
    int should_sleep = 0;
    while(buffor_running){
        sem_wait(&writers_mutex);
        pthread_mutex_lock(&buff.mutex_inside);

        if(buff.head_iterator != -1){
            should_sleep = 0;
            value = buff.buffor[0];
            printf("BUFFOR provisiones: %d\n", value);

            for(int i = 0 ; i < buff.head_iterator && i < BUFFOR_SIZE - 1 ; ++i)
                buff.buffor[i] = buff.buffor[i + 1];
        }else{
            should_sleep = 1;
        }

        printf("BUFFOR: going home\n");
        pthread_mutex_unlock(&buff.mutex_inside);
        sem_post(&writers_mutex);
        sleep(2);
    }
}


void* writer(void *arg)
{
    for(int i = 0 ; i < iterations ; ++i){
        pthread_mutex_lock(&buff.mutex_outside);

        while(buff.head_iterator == BUFFOR_SIZE){
            sleep(rand()%10);
        }

        pthread_mutex_lock(&buff.mutex_inside);

        buff.buffor[buff.head_iterator] = rand() % 15;

        printf("WRITER: %d head index: %d\n", buff.buffor[buff.head_iterator], buff.head_iterator);

        buff.head_iterator++;

        printf("WRITER: going home\n");
        pthread_mutex_unlock(&buff.mutex_inside);
        sleep(1);
        pthread_mutex_unlock(&buff.mutex_outside);

    }
    return NULL;
}


int main(int argc, char *argv[]) {

    srand(time(0));
    
    if( argc != 4 || sscanf (argv[1], "%i", &writers) != 1 || sscanf (argv[2], "%i", &readers) != 1 || sscanf (argv[3], "%i", &iterations) != 1 ){
        printf("Wrong arguments: main (int)writers_count (int)readers_count (int)iterations\n");
        exit(1);
    }

    init_buffor(&buff);

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

    pthread_create(&threads[readers + writers], NULL, buffor, NULL); 


    for(int i = 0; i < writers + readers; ++i)
        pthread_join(threads[i], NULL); 

    buffor_running=0;
    sem_destroy(&writers_mutex); 
    pthread_mutex_destroy(&counters_mutex);
    free(threads);
    return 0;
}