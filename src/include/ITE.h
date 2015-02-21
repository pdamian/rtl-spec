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

#ifndef ITE_H /* Items */
#define ITE_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  uint32_t     Fc;
  uint32_t     Ts_sec;
  uint32_t     Ts_usec;
  uint32_t     samp_rate;
  uint32_t     log2_fft_size;
  uint32_t     reduced_fft_size;
  uint32_t     avg_index;
  uint32_t     cmpr_level;
  int          hopping_strategy_id;
  int          window_fun_id;
  int          avg_factor;
  int          soverlap;
  float        gain;
  size_t       data_size;
  void         *data;
  float        freq_overlap;
  size_t       samples_size;
  float        freq_res;
  float        *samples;
} Item;

Item* ITE_init();
Item* ITE_copy(const Item *it);
void  ITE_free(Item *it);

#endif /* ITE_H */