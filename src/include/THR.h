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