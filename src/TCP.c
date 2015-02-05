#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

#include "include/TCP.h"

#define DEFAULT_PKT_SIZE_B 512

uint64_t pack754(long double f, unsigned bits, unsigned expbits) {
  long double fnorm;
  int shift;
  long long sign, exp, significand;
  unsigned significandbits = bits - expbits - 1; // -1 for sign bit
  if (f == 0.0) return 0; // get this special case out of the way
  // check sign and begin normalization
  if (f < 0) { sign = 1; fnorm = -f; }
  else { sign = 0; fnorm = f; }
  // get the normalized form of f and track the exponent
  shift = 0;
  while(fnorm >= 2.0) { fnorm /= 2.0; shift++; }
  while(fnorm < 1.0) { fnorm *= 2.0; shift--; }
  fnorm = fnorm - 1.0;
  // calculate the binary form (non-float) of the significand data
  significand = fnorm * ((1LL<<significandbits) + 0.5f);
  // get the biased exponent
  exp = shift + ((1<<(expbits-1)) - 1); // shift + bias
  // return the final answer
  return (sign<<(bits-1)) | (exp<<(bits-expbits-1)) | significand;
}

long double unpack754(uint64_t i, unsigned bits, unsigned expbits) {
  long double result;
  long long shift;
  unsigned bias;
  unsigned significandbits = bits - expbits - 1; // -1 for sign bit
  if (i == 0) return 0.0;
  // pull the significand
  result = (i&((1LL<<significandbits)-1)); // mask
  result /= (1LL<<significandbits); // convert back to float
  result += 1.0f; // add the one back on
  // deal with the exponent
  bias = (1<<(expbits-1)) - 1;
  shift = ((i>>significandbits)&((1LL<<expbits)-1)) - bias;
  while(shift > 0) { result *= 2.0; shift--; }
  while(shift < 0) { result /= 2.0; shift++; }
  // sign it
  result *= (i>>(bits-1))&1? -1.0: 1.0;
  return result;
}

static void tcp_info(TCP_Connection *c, const int err, const char *msg) {
  int portnum;
  char hostaddr[INET_ADDRSTRLEN];
  
  if(c != NULL) {
    portnum = ntohs(c->host_addr.sin_port);
    inet_ntop(AF_INET, &(c->host_addr.sin_addr), hostaddr, INET_ADDRSTRLEN);
  } else {
    portnum = -1;
    strncpy(hostaddr, "empty", INET_ADDRSTRLEN);
  }
  fprintf(stderr, "%s\tTCP %s:%d\t%s:%d\n", err ? "[ERROR]" : "[OK]", msg, err, hostaddr, portnum);
  fprintf(stderr, "\t%s\n", err ? strerror(errno) : "");
}

int tcp_init(TCP_Connection **c, const char *hostaddr, const int portnum) {
  
  *c = (TCP_Connection *) malloc(sizeof(TCP_Connection));
  
  // Initialize socket
  (*c)->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if((*c)->socket_fd < 0) {
    perror("ERROR on socket initialization");
    return -1;
  }
  // Initialize host address
  bzero(&((*c)->host_addr), sizeof((*c)->host_addr));
  (*c)->host_addr.sin_family = AF_INET;
  (*c)->host_addr.sin_port = htons(portnum);
  if(hostaddr == NULL) {
    (*c)->host_addr.sin_addr.s_addr = INADDR_ANY;
    //Bind socket and host address
    if(bind((*c)->socket_fd, (struct sockaddr *) &((*c)->host_addr), sizeof((*c)->host_addr)) < 0) {
      perror("ERROR on binding socket and host address");
      exit(1);
    }
  } else {
    if(inet_pton(AF_INET, hostaddr, &((*c)->host_addr.sin_addr)) < 1) {
      perror("ERROR on host address initialization");
      return -1;
    }
  }
  
  return 0;
}

int tcp_init_p(TCP_Connection **c, const char *hostaddr, const int portnum) {
  int err = tcp_init(c, hostaddr, portnum);
  tcp_info(*c, err, "init");
  return err;
}

int tcp_connect(TCP_Connection *c) {
  if(connect(c->socket_fd, (struct sockaddr *) &(c->host_addr), sizeof(c->host_addr)) < 0) {
    perror("ERROR on connecting");
    return -1;
  }
  return 0;
}

int tcp_connect_p(TCP_Connection *c) {
  int err = tcp_connect(c);
  tcp_info(c, err, "connect");
  return err;
}

int tcp_listen(TCP_Connection *c, const int backlog) {
  if(listen(c->socket_fd, backlog) < 0) {
    perror("ERROR on listening");
    return -1;
  }
  return 0;
}

int tcp_listen_p(TCP_Connection *c, const int backlog) {
  int err = tcp_listen(c, backlog);
  tcp_info(c, err, "listen");
  return err;
}

int tcp_accept(TCP_Connection *wel, TCP_Connection **cli) {
  *cli = (TCP_Connection *) malloc(sizeof(TCP_Connection));
  
  socklen_t len = sizeof((*cli)->host_addr);
  (*cli)->socket_fd = accept(wel->socket_fd, (struct sockaddr *) &((*cli)->host_addr), &len);
  if((*cli)->socket_fd < 0) {
    perror("ERROR on accept");
    return -1;
  }
  return 0;
}

int tcp_accept_p(TCP_Connection *wel, TCP_Connection **cli) {
  int err = tcp_accept(wel, cli);
  tcp_info((*cli), err, "accept");
  return err;
}

int tcp_read(TCP_Connection *c, void *msg, int len) {
  int b, cnt = 0, bytes_read = 0;
  
  while(cnt < 5) {
    b = recvfrom(c->socket_fd, msg+bytes_read, len-bytes_read, 0, (struct sockaddr *) 0, (socklen_t *) 0);
    // Error occurred while reading
    if(b < 0) return -1;
    // Received end of file or connection closure
    if(b == 0) {
      if(bytes_read == len) return 0;
      ++cnt;
    }
    // End of file not yet reached
    else {
      bytes_read += b;
    }
  }
  
  return -1;
}

int tcp_read_p(TCP_Connection *c, void *msg, int len) {
  int err = tcp_read(c, msg, len);
  tcp_info(c, err, "read");
  return err;
}

int tcp_write(TCP_Connection *c, const void *msg, int len) {
  int b, bytes_written = 0;
  
  while(1) {
    // Write data in chunks
    b = write(c->socket_fd, msg+bytes_written, MIN(DEFAULT_PKT_SIZE_B, len-bytes_written));
    
    // Error occurred while writing
    if(b < 0) return -1;
    
    // Written b bytes
    bytes_written += b;
    
    // All data sent
    if(bytes_written == len) return 0;
  }
  
}

int tcp_write_p(TCP_Connection *c, const void *msg, int len) {
  int err = tcp_write(c, msg, len);
  tcp_info(c, err, "write");
  return err;  
}

int tcp_disconnect(TCP_Connection *c) {
  if(c == NULL) return 0;
  if(close(c->socket_fd) < 0) return -1;
  return 0;
}

int tcp_disconnect_p(TCP_Connection *c) {
  int err = tcp_disconnect(c);
  tcp_info(c, err, "discon");
  return err;
}

int tcp_release(TCP_Connection *c) {
  free(c);
  c = NULL;
  return 0;
}

int tcp_release_p(TCP_Connection *c) {
  int portnum, err;
  char hostaddr[INET_ADDRSTRLEN];
  if(c == NULL) return 0;
  portnum = ntohs(c->host_addr.sin_port);
  inet_ntop(AF_INET, &(c->host_addr.sin_addr), hostaddr, INET_ADDRSTRLEN);
  err = tcp_release(c);
  fprintf(stderr, "%s\tTCP %s:%d\t%s:%d\n", err ? "[ERROR]" : "[OK]", "release", err, hostaddr, portnum);
  fprintf(stderr, "\t%s\n", err ? strerror(errno) : "");
  return err;
}