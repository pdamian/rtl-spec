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

#include "include/ITE.h"

Item* ITE_init() {
  Item *it = NULL;

  it = (Item *) malloc(sizeof(Item));
  if(it == NULL) return NULL;

  it->data_size = 0;
  it->data = NULL;
  it->samples_size = 0;
  it->samples = NULL;

  return it;
}

Item* ITE_copy(const Item *it) {
  Item *iout = NULL;
  
  size_t data_size = it->data_size;
  size_t samples_size = it->samples_size;
  
  iout = ITE_init();
  if(iout == NULL) return NULL;
  memcpy(iout, it, sizeof(Item));
  
  if(data_size > 0) {
    iout->data = (void *) malloc(data_size);
    memcpy(iout->data, it->data, data_size);
  }
  
  if(samples_size > 0) {
    iout->samples = (float *) malloc(samples_size);
    memcpy(iout->samples, it->samples, samples_size);
  }
  
  return iout;
}

void  ITE_free(Item *it) {
  if(it->data_size > 0 && it->data != NULL) free(it->data);
  if(it->samples_size > 0 && it->samples != NULL) free(it->samples);
  free(it);
}