#define _XOPEN_SOURCE 600
#include <pthread.h>
#include <stdlib.h>

#define N 4
#define LEN 64
#define ITERS 200

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_barrier_t b;

static int buf1[LEN];
static int buf2[LEN];
static int *cur = buf1;

static void *worker(void *arg) {
  (void)arg;

  for (int it = 0; it < ITERS; it++) {
    // "Commit" (update global pointer) under lock to avoid template W/W issues
    pthread_mutex_lock(&m);
    cur = (it & 1) ? buf1 : buf2;
    pthread_mutex_unlock(&m);

    pthread_barrier_wait(&b);

    // Unlocked reads from cur; safe because pointer was committed before barrier
    int s = 0;
    for (int i = 0; i < LEN; i++) s += cur[i];
    if (s == 123456789) abort();

    pthread_barrier_wait(&b);
  }
  return NULL;
}

int main(void) {
  pthread_t t[N];
  pthread_barrier_init(&b, NULL, N);

  for (int i = 0; i < N; i++) pthread_create(&t[i], NULL, worker, NULL);
  for (int i = 0; i < N; i++) pthread_join(t[i], NULL);

  pthread_barrier_destroy(&b);
  return 0;
}
