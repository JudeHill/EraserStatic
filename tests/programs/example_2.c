
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <unistd.h>

#define NTHREADS 4
#define NODES    20000

struct node {
    struct node *next;
    char payload[16]; 
};

static struct node *head = NULL; 
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* popper(void* arg) {
    (void)arg;
    for (;;) {
        pthread_mutex_lock(&lock);
        struct node *n = head;           
        if (!n) {
            pthread_mutex_unlock(&lock);
            break;
        }
        // Should be 3 lines lower
        pthread_mutex_unlock(&lock);
        head = n->next;              
        free(n);
        // pthread_mutex_unlock(&lock);
        
    }
    return NULL;
}

int main(void) {
    for (int i = 0; i < NODES; i++) {
        struct node *n = malloc(sizeof *n);
        if (!n) { perror("malloc"); exit(1); }
        n->next = head;
        head = n;
    }

    pthread_t th[NTHREADS];
    for (int i = 0; i < NTHREADS; i++) {
        if (pthread_create(&th[i], NULL, popper, NULL) != 0) {
            perror("pthread_create"); exit(1);
        }
    }

    for (int i = 0; i < NTHREADS; i++) pthread_join(th[i], NULL);

    pthread_mutex_destroy(&lock);
    puts("Done (if you got here, you were lucky)!");
    return 0;
}
