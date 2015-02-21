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

#include "include/UTI.h"

#define MAX_FILE_SIZE 8192

#define DEFAULT_PKT_SIZE_B 128
#define DEFAULT_HEADER_SIZE_B 66
#define DEFAULT_WIN_LEN_MS 200

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

char* UTI_read_file(const char *file_name) {
  // Open file
  FILE *file = fopen(file_name, "r");
  if(file == NULL) {
    fprintf(stderr, "ERROR: Failed to open file %s.\n", file_name);
    exit(1);
  }
  
  // Get file size
  fseek(file, 0, SEEK_END);
  int file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  if(file_size <= 0) {
    fprintf(stderr, "ERROR: File %s is empty.\n", file_name);
    exit(1);
  }
  
  if(file_size > MAX_FILE_SIZE) {
    fprintf(stderr, "ERROR: File %s exceeds maximum file size.\n", file_name);
    exit(1);
  }
  
  // Read file
  char *buf = (char *) malloc(file_size+1);
  size_t read = fread(buf, 1, file_size, file);
  if(read != file_size) {
    fprintf(stderr, "ERROR: File %s incompletely read.\n", file_name);
    exit(1);
  }
  buf[file_size] = '\0';
  fclose(file);
  
  return buf;
}

UTI_BandwidthController* UTI_initialize_bandwidth_controller(const int bandwidth_limit) {
  UTI_BandwidthController *bc = NULL;
  
  bc = (UTI_BandwidthController *) malloc(sizeof(UTI_BandwidthController));
  if(bc == NULL) return NULL;
  
  // Bandwidth limitation
  if(bandwidth_limit > 0) {
    bc->max_bits_per_second = bandwidth_limit * 1024;
    bc->max_bytes_per_window = (bc->max_bits_per_second * DEFAULT_WIN_LEN_MS) / 8000;
    bc->max_payload_size_in_bytes = MAX(DEFAULT_PKT_SIZE_B,
					bc->max_bytes_per_window) - DEFAULT_HEADER_SIZE_B;
  } 
  // No bandwidth limitation
  else {
    bc->max_bits_per_second = 0;
    bc->max_bytes_per_window = 0;
    bc->max_payload_size_in_bytes = DEFAULT_PKT_SIZE_B - DEFAULT_HEADER_SIZE_B;
  }
  bc->bytes_sent_in_window = 0;
  clock_gettime(CLOCK_MONOTONIC, &(bc->window_start));
  
  return bc;
}

void UTI_enforce_bandwidth_throttling(UTI_BandwidthController *bc, const int bytes_sent) {
  
  // Return immediately for no bandwidth throttling
  if(bc->max_bits_per_second <= 0) return;
  
  // Increase the number of bytes sent in the current window
  bc->bytes_sent_in_window += bytes_sent;
  
  // Window data limit exceeded
  if(bc->bytes_sent_in_window >= bc->max_bytes_per_window) {
    struct timespec window_end, now;
    
    // Calculate window ending time
    window_end.tv_sec = bc->window_start.tv_sec;
    window_end.tv_nsec = bc->window_start.tv_nsec;
    window_end.tv_nsec += DEFAULT_WIN_LEN_MS * 1000000;
    if(window_end.tv_nsec > 1000000000L) {
      window_end.tv_sec += 1;
      window_end.tv_nsec -= 1000000000L;
    }
    
    // Get current time
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    // Current time is before window ending
    if(now.tv_sec < window_end.tv_sec ||
      (now.tv_sec == window_end.tv_sec && now.tv_nsec < window_end.tv_nsec)) {
      struct timespec remaining;
      
      // Calculate remaining time till window ending
      remaining.tv_sec = window_end.tv_sec - now.tv_sec;
      if(window_end.tv_nsec >= now.tv_nsec) {
	  remaining.tv_nsec = window_end.tv_nsec - now.tv_nsec;
      } else {
	remaining.tv_nsec = 1000000000L + window_end.tv_nsec - now.tv_nsec;
	remaining.tv_sec -= 1;
      }
      
      // Sleep till window ending
      nanosleep(&remaining, NULL);
    }
    
    // Reset window
    bc->bytes_sent_in_window = 0;
    clock_gettime(CLOCK_MONOTONIC, &bc->window_start);
  }
  
}

void UTI_release_bandwidth_controller(UTI_BandwidthController *bc) {
  free(bc);
}