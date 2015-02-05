#include <stdlib.h>

#include "include/QUE.h"

Queue* QUE_initialize(int size) {
  Queue *q = NULL;
  
  q = (Queue *) malloc(sizeof(Queue));
  if(q == NULL) return NULL;

  q->buf = (void **) malloc(size * sizeof(void *));
  q->head = 0;
  q->tail = 0;
  q->full = 0;
  q->empty = 1;
  q->exit = 0;
  q->size = size;
  q->mut = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
  pthread_cond_init(q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
  pthread_cond_init(q->notEmpty, NULL);
  
  return q;
}

void QUE_insert(Queue *q, void *in) {
  q->buf[q->tail] = in;
  ++q->tail;
  if(q->tail == q->size) q->tail = 0;
  if(q->tail == q->head) q->full = 1;
  q->empty = 0;
}

void* QUE_remove(Queue *q) {
  void *i;
  
  i = q->buf[q->head];
  ++q->head;
  if(q->head == q->size) q->head = 0;
  if(q->head == q->tail) q->empty = 1;
  q->full = 0;
  
  return i;
}

void QUE_release(Queue *q) {
  free(q->buf);
  pthread_mutex_destroy(q->mut);
  free(q->mut);	
  pthread_cond_destroy(q->notFull);
  free(q->notFull);
  pthread_cond_destroy(q->notEmpty);
  free(q->notEmpty);
  free(q);
}