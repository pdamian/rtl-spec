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

#ifndef UTI_H /* Utilities */
#define UTI_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
  int max_bits_per_second;		// Bandwidth throttling in bits/sec, or 0 for  unlimited
  int max_bytes_per_window;		// Number of bytes that can be sent per window
  int max_payload_size_in_bytes;	// Maximal payload size in bytes
  int bytes_sent_in_window;		// Number of bytes already sent in current window
  struct timespec window_start;		// Starting time stamp of current window
} UTI_BandwidthController;

/*!
 * Read file to string
 * 
 * \param file_name Name of the file to be read
 * \return String containing the file content
 */
char* UTI_read_file(const char *file_name);

/*!
 * Initialize bandwidth controller
 * 
 * \param bandwidth_limit Bandwidth limitation in Kb/s, unlimited bandwidth for non-positive values
 * \return Bandwidth controller
 */
UTI_BandwidthController* UTI_initialize_bandwidth_controller(const int bandwidth_limit);

/*!
 * Enforce bandwidth throttling
 * 
 * \param bc Bandwidth controller
 * \param bytes_sent Number of bytes sent
 */
void UTI_enforce_bandwidth_throttling(UTI_BandwidthController *bc, const int bytes_sent);

/*!
 * Release bandwidth controller
 * 
 * \param bc Bandwidth controller
 */
void UTI_release_bandwidth_controller(UTI_BandwidthController *bc);

#endif /* UTI_H */