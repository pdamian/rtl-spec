#ifndef FFT_H /* Fast Fourier Transform */
#define FFT_H

#include <stdint.h>

/*!
 * Initialize FFT resources
 * 
 * \param log2_N Binary logarithm of the FFT size N
 * \param batchsize Number of FFT jobs to process as batch
 */
void FFT_initialize(int log2_N, int batchsize);

/*!
 * Perform forward FFT. The outputs are scaled with factor 1/N and given as magnitudes squared on a
 * logarithmic scale (dB).
 * 
 * \param in An array of references pointing to BATCHSIZE input buffers. Each input buffer is
 * 	     assumed to contain a stream of 2N interleaved I/Q samples.
 * 
 * \param out An array of references pointing to BATCHSIZE output buffers. Each output buffer must
 * 	      provide space for storing N output values. The frequency-domain values are shifted
 * 	      and converted to a dB scale.
 */
void FFT_forward(float **in, float **out);

/*!
 * Release FFT resources
 */
void FFT_release();

#endif /* FFT_H */
