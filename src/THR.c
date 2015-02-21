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