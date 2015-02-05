#ifndef TCP_H /* Transmission Control Protocol */
#define TCP_H

#include <netinet/in.h>



#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

typedef struct {
  int socket_fd;
  struct sockaddr_in host_addr;
} TCP_Connection;

/*!
 * Encode floats and doubles into IEEE-754 format.
 * 
 * NOTE: pack754 does not support +-inf.
 */
uint64_t pack754(long double f, unsigned bits, unsigned expbits);

/*!
 * Decode floats and doubles from IEEE-754 format.
 * 
 * NOTE: unpack754 does not support +-inf.
 */
long double unpack754(uint64_t i, unsigned bits, unsigned expbits);

/*!
 * Initialization of TCP connection capable to enforce bandwidth throttling.
 * 
 * \param c
 * \param hostaddr
 * \param portnum
 * \param bandwidth Bandwidth limitation in Kb/s, unlimited bandwidth for non-positive values
 * 
 * \return
 */
int tcp_init(TCP_Connection **c, const char *hostaddr, const int portnum);
int tcp_init_p(TCP_Connection **c, const char *hostaddr, const int portnum);
int tcp_connect(TCP_Connection *c);
int tcp_connect_p(TCP_Connection *c);
int tcp_listen(TCP_Connection *c, const int backlog);
int tcp_listen_p(TCP_Connection *c, const int backlog);
int tcp_accept(TCP_Connection *wel, TCP_Connection **cli);
int tcp_accept_p(TCP_Connection *wel, TCP_Connection **cli);
int tcp_read(TCP_Connection *c, void *msg, int len);
int tcp_read_p(TCP_Connection *c, void *msg, int len);
int tcp_write(TCP_Connection *c, const void *msg, int len);
int tcp_write_p(TCP_Connection *c, const void *msg, int len);
int tcp_disconnect(TCP_Connection *c);
int tcp_disconnect_p(TCP_Connection *c);
int tcp_release(TCP_Connection *c);
int tcp_release_p(TCP_Connection *c);

#endif /* TCP_H */