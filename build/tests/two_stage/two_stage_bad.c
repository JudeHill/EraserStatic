#include <pthread.h>
#define NUM_THREADS 4
#define NUM_ITER 10

int a = 0;
int b = 0;
pthread_mutex_t *lock_1, *lock_2;

void* worker(){
    for (int j=0;j<NUM_ITER;j++){
        pthread_mutex_lock(lock_1);
        a = 1;
        pthread_mutex_unlock(lock_1);
    
        pthread_mutex_lock(lock_2);
        b = a + 1;
        pthread_mutex_unlock(lock_2);
    }
}

int main(void){
    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(lock_1, NULL);
    pthread_mutex_init(lock_2, NULL);
    for (int i=0;i<NUM_THREADS;i++){
        pthread_create(&threads[i], NULL, worker, NULL);
    }

    for (int i=0;i<NUM_THREADS;i++){
        pthread_join(threads[i], NULL);
    }
    return 0;
}