#include "include/THR.h"

void THR_initialize(Thread **t, unsigned int id) {
  *t = (Thread *) malloc(sizeof(Thread));
  
  (*t)->id = id;
  (*t)->is_running = 1;
  (*t)->flags = 0;
  (*t)->fd = (pthread_t *) malloc(sizeof(pthread_t));
  (*t)->lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init((*t)->lock, NULL);
  (*t)->awake = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
  pthread_cond_init((*t)->awake, NULL);
}

void THR_release(Thread *t) {
  pthread_cond_destroy(t->awake);
  free(t->awake);
  pthread_mutex_destroy(t->lock);
  free(t->lock);
  free(t->fd);
  free(t);
}