/*
 * Copyright (C) 2015 by Damian Pfammatter <pfammatterdamian@gmail.com>
 *
 * This file is part of RTL-spec.
 *
 * RTL-Spec is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * RTL-Spec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RTL-Spec.  If not, see <http://www.gnu.org/licenses/>.
 */

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