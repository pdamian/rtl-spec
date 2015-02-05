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