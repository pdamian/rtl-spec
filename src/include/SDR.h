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
