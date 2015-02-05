#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined(RPI_GPU)
#include "include/FFT.h"
#include "include/mailbox.h"
#include "include/gpu_fft.h"
#else
#include <fftw.h>
#endif

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

static int _LOG2_N = 0, _N = 0;
static int _BATCHSIZE;

#if defined(RPI_GPU)
static int _IOCTL_MB;
static struct GPU_FFT *gpu_fft = NULL;
#else
static fftw_complex *fftw_in = NULL, *fftw_out = NULL;
static fftw_plan plan = NULL;
#endif

void FFT_initialize(int log2_N, int batchsize) {
  // Set FFT length
  _LOG2_N = log2_N;
  _N = 1<<log2_N;
  // Set batch size
  _BATCHSIZE = batchsize;
#if defined(RPI_GPU)
  int r;
  // Open ioctl mailbox through which ARM CPU and Videocore GPU communicate
  _IOCTL_MB = mbox_open();
  // Allocate memory and initialize data structures for forward FFT
  r = gpu_fft_prepare(_IOCTL_MB, _LOG2_N, GPU_FFT_FWD, _BATCHSIZE, &gpu_fft);
  switch(r) {
      case -1: 
	fprintf(stderr, "Unable to enable V3D. Please check your firmware is up to date.\n");
	exit(1);
      case -2:
	fprintf(stderr, "log2_N=%d not supported. Try between 8 and 17.\n", _LOG2_N);
	exit(1);
      case -3:
	fprintf(stderr, "Out of memory. Try a smaller batch or increase GPU memory.\n");
	exit(1);
  }
#else
  fftw_in = (fftw_complex *) malloc(_N*sizeof(fftw_complex));
  fftw_out = (fftw_complex *) malloc(_N*sizeof(fftw_complex));
  plan = fftw_create_plan(_N, FFTW_FORWARD, FFTW_ESTIMATE);
#endif
}

void FFT_forward(float **in, float **out) {
  int i, j;
  float re, im;
  
#if defined(RPI_GPU)
  struct GPU_FFT_COMPLEX *base;
  
  // Prepare FFT input
  for(i=0; i<_BATCHSIZE; ++i) {
    base = gpu_fft->in + i*gpu_fft->step;
    for(j=0; j<_N; ++j) {
//       base[j].re = in[i][2*j]; // / 255.f;
//       base[j].im = in[i][2*j+1]; // / 255.f;
      base[j].re = in[i][2*j] / 128.0f;
      base[j].im = in[i][2*j+1] / 128.0f;
    }
  }
  
  // Perform forward FFT
  gpu_fft_execute(gpu_fft);
  
  // Scale, shift and convert to magnitude squared (dB)
  for(i=0; i<_BATCHSIZE; ++i) {
    base = gpu_fft->out + i*gpu_fft->step;
    for(j=0; j<_N; ++j) {
      // Scale and shift
      if(j < _N/2) {
	re = base[_N/2+j].re / _N;
	im = base[_N/2+j].im / _N;
      } else {
	re = base[j-_N/2].re / _N;
	im = base[j-_N/2].im / _N;
      }
//       // Magnitude squared (dB)
//       out[i][j] = 10.f * log10(re*re + im*im);
      // TODO: Magnitude [dB] and lower limit enforcement
      out[i][j] = MAX(10.0f * log10(re*re + im*im), -100);
    }
  }
  
#else
  
  for(i=0; i<_BATCHSIZE; ++i) {
    
    // Prepare FFT input
    for(j=0; j<_N; ++j) {
      fftw_in[j].re = in[i][2*j];
      fftw_in[j].im = in[i][2*j+1];
    }
    
    // Perform forward FFT
    fftw_one(plan, fftw_in, fftw_out);
    
    // Scale, shift and convert to magnitude squared (dB)
    for(j=0; j<_N; ++j) {
      // Scale and shift
      if(j < _N/2) {
	re = fftw_out[_N/2+j].re / _N;
	im = fftw_out[_N/2+j].im / _N;
      } else {
	re = fftw_out[j-_N/2].re / _N;
	im = fftw_out[j-_N/2].im / _N;
      }
//       out[i][j] = 10.0f * log10(re*re + im*im);
      // TODO: Magnitude [dB] and lower limit enforcement
      out[i][j] = MAX(10.0f * log10(re*re + im*im), -100);
    }
    
  }
  
#endif
  
}

void FFT_release() {
#if defined(RPI_GPU)
  if(gpu_fft == NULL) return;
  gpu_fft_release(gpu_fft);
  mbox_close(_IOCTL_MB);
#else
  if(fftw_in != NULL) free(fftw_in);
  if(fftw_out != NULL) free(fftw_out);
  if(plan != NULL) fftw_destroy_plan(plan);
#endif
}
