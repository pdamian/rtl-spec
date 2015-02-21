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

#include "include/SDR.h"

/*
 * SDR_initialize - allocate memory and initialize device
 */
 void SDR_initialize(rtlsdr_dev_t **dev, int dev_index) {
  int r, dev_count;
  
  // Count RTL-SDR devices
  dev_count = rtlsdr_get_device_count();
  if(dev_count <= 0) {
    fprintf(stderr, "No supported devices found\n");
    exit(1);
  }
  
  // Open RTL-SDR device
  r = rtlsdr_open(dev, dev_index);
  if(r < 0) {
    fprintf(stderr, "Failed to open rtlsdr device #%d\n", dev_index);
    exit(1);
  }
}

/*
 * SDR_set_sample_rate - set device's sampling rate
 */
void SDR_set_sample_rate(rtlsdr_dev_t *dev, uint32_t samp_rate) {
  int r;
  
  // Set sample rate
  r = rtlsdr_set_sample_rate(dev, samp_rate);
  if(r < 0) {
    fprintf(stderr, "WARNING: Failed to set sample rate to %d MHz\n", samp_rate / 1000000);
  }
}

/*
 * SDR_set_gain - set device's gain
 */
void SDR_set_gain(rtlsdr_dev_t *dev, float gain) {
  int r;
  
  // Enable automatic gain
  if(gain < 0) {
    r = rtlsdr_set_tuner_gain_mode(dev, 0);
    if(r < 0) {
      fprintf(stderr, "WARNING: Failed to enable automatic gain mode\n");
    }
  }
  // Enable manual gain
  else {
    r = rtlsdr_set_tuner_gain_mode(dev, 1);
    if(r < 0) {
      fprintf(stderr, "WARNING: Failed to enable manual gain mode\n");
    }
    r = rtlsdr_set_tuner_gain(dev, 10*gain);
    if(r < 0) {
      fprintf(stderr, "WARNING: Failed to set manual tuner gain\n");
    }
  }
}

void SDR_set_freq_correction(rtlsdr_dev_t *dev, int ppm_error) {
  int r;

  r = rtlsdr_set_freq_correction(dev, ppm_error);
  if(r < 0 && r != -2) fprintf(stderr, "WARNING: Failed to set ppm error %d\n", ppm_error);
}

/*
 * SDR_retune - set device's center frequency
 */
void SDR_retune(rtlsdr_dev_t *dev, uint32_t freq) {
  int r;
  uint32_t actual_freq;
  
  // Set center frequency of device 'dev' to 'freq'
  r = rtlsdr_set_center_freq(dev, freq);
  if(r < 0) fprintf(stderr, "WARNING: Failed to set center frequency to %d Hz\n", freq);
  
  // Get actual center frequency
  actual_freq = rtlsdr_get_center_freq(dev);
  if(freq != actual_freq)
    fprintf(stderr, "WARNING: Theoretical / actual center frequency:\t%d / %dHz\n",
	    freq, actual_freq);
  
  // Flush buffer
  r = rtlsdr_reset_buffer(dev);
  if(r != 0) fprintf(stderr, "WARNING: Flushing buffer failed.\n");
}

void SDR_read(rtlsdr_dev_t *dev, uint8_t *iq_buf, int N) {
  int r, n_read;
  
  // Read N-point interleaved I/Q stream from device
  r = rtlsdr_read_sync(dev, iq_buf, N, &n_read);
  if(r != 0 || n_read != N) fprintf(stderr, "WARNING: Synchronous read failed.\n");
}

/*
 * SDR_release
 */
void SDR_release(rtlsdr_dev_t *dev) {
  // Close device
  rtlsdr_close(dev);
}