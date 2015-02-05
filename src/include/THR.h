#ifndef THR_H /* Threading */
#define THR_H

#include <pthread.h>
#include <stdlib.h>

typedef struct {
  unsigned int     id;
  unsigned int     is_running;
  unsigned int     flags;
  pthread_t       *fd;
  pthread_mutex_t *lock;
  pthread_cond_t  *awake;
} Thread;

void THR_initialize(Thread **t, unsigned int id);
void THR_release(Thread *t);

#endif /* THR_H */