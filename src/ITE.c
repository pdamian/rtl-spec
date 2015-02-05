
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