#ifndef QUE_H /* Queues */
#define QUE_H

#include <stdint.h>
#include <pthread.h>

typedef struct {
  void **buf;
  int head, tail;
  int full, empty, exit;
  int size;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} Queue;

Queue* QUE_initialize(int size);
void QUE_insert(Queue *q, void *in);
void* QUE_remove(Queue *q);
void QUE_release(Queue *q);

#endif /* QUE_H */