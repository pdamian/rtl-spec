#ifndef SDR_H /* Software-Defined Radios */
#define SDR_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <rtl-sdr.h>

void SDR_initialize(rtlsdr_dev_t **dev, int dev_index);
void SDR_set_sample_rate(rtlsdr_dev_t *dev, uint32_t samp_rate);
void SDR_set_gain(rtlsdr_dev_t *dev, float gain);
void SDR_set_freq_correction(rtlsdr_dev_t *dev, int ppm_error);
void SDR_retune(rtlsdr_dev_t *dev, uint32_t freq);

/*!
 * Read N-point interleaved I/Q stream from device
 * 
 * \param iq_buf buffer to store the N-point interleaved I/Q stream
 * \param N length of interleaved I/Q stream
 */
void SDR_read(rtlsdr_dev_t *dev, uint8_t *iq_buf, int N);
void SDR_release(rtlsdr_dev_t *dev);

#endif /* SDR_H */
