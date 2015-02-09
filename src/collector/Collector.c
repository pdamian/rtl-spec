#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <pthread.h>
#include <zlib.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>

#include "../include/UTI.h"
#include "../include/THR.h"
#include "../include/ITE.h"
#include "../include/QUE.h"
#include "../include/TCP.h"

#define DEFAULT_THREAD_POOL_SIZE 25
#define DEFAULT_FILE_TIME 3600
#define DEFAULT_FILE_PATH_STR "dat/"

#define THR_COLLECTOR 0
#define THR_RECEPTION 1

#define STR_LEN 30
#define STR_LEN_LONG 512

typedef struct {
  Thread       *thread;
  Queue        *thread_pool;
  unsigned int portnumber;
  unsigned int thread_pool_size;
  unsigned int file_time;
  char         *file_path_str;
} CollectorCTX;

typedef struct {
  Thread         *thread;
  TCP_Connection *tcp_c;
  unsigned int   file_time;
  char           *file_path_str;
} ReceptionCTX;

typedef struct {
  CollectorCTX *coll_ctx;
  ReceptionCTX *recp_ctx;
} ReceptionARG;

typedef struct {
  Thread *thread;
} DecompressionCTX;

typedef struct {
  ReceptionCTX     *recp_ctx;
  DecompressionCTX *decmpr_ctx;
  Queue            *qin, **qsout;
  int              qsout_cnt;
} DecompressionARG;

typedef struct {
  Thread       *thread;
  unsigned int file_time;
  char         *file_path_str;
} StoringCTX;

typedef struct {
  ReceptionCTX *recp_ctx;
  StoringCTX   *stor_ctx;
  Queue        *qin, **qsout;
  int          qsout_cnt;
} StoringARG;

static void* reception(void *args);
static void* decompression(void *args);
static void* storing(void *args);

int main(int argc, char *argv[]) {
  int            i;
  
  CollectorCTX   *coll_ctx;
  ReceptionCTX   **recp_ctx = NULL, *r_ctx;
  ReceptionARG   *recp_arg;
  
  TCP_Connection *tcp_wel = NULL, *tcp_cli = NULL;
  
  // Ctrl-C signal catcher
  void terminate(int sig) {
    
    if(sig == SIGINT) fprintf(stderr, "\nCtrl-C caught. Waiting for termination...\n");
    
    // Release TCP and SSL/TLS connections
#if defined(VERBOSE)
    tcp_disconnect_p(tcp_wel);
    tcp_release_p(tcp_wel);
//     tcp_disconnect_p(tcp_cli);
//     tcp_release_p(tcp_cli);
    free(tcp_cli);
#else
    tcp_disconnect(tcp_wel);
    tcp_release(tcp_wel);
//     tcp_disconnect(tcp_cli);
//     tcp_release(tcp_cli);
    free(tcp_cli);
#endif
    
    // Ask reception threads to terminate
    for(i=0; i<coll_ctx->thread_pool_size; ++i) {
      pthread_mutex_lock(recp_ctx[i]->thread->lock);
      recp_ctx[i]->thread->is_running = 0;
      pthread_mutex_unlock(recp_ctx[i]->thread->lock);
      pthread_cond_signal(recp_ctx[i]->thread->awake);
    }
    
    // Terminate and release reception threads
    for(i=0; i<coll_ctx->thread_pool_size; ++i) {
      // Join reception threads
      pthread_join(*(recp_ctx[i]->thread->fd), NULL);
      // Release reception threads
      THR_release(recp_ctx[i]->thread);
      // Release reception context
      free(recp_ctx[i]);
    }
    free(recp_ctx);
    
    // Release collector context
    THR_release(coll_ctx->thread);
    QUE_release(coll_ctx->thread_pool);
    free(coll_ctx);
    
    fprintf(stderr, "Terminated.\n");
    pthread_exit(NULL);
  }

  // Parse arguments and options
  void parse_args(int argc, char *argv[]) {
    int opt;
    const char *options = "p:t:f:";
    
    // Option arguments
    while((opt = getopt(argc, argv, options)) != -1) {
      switch(opt) {
	case 'p':
	  coll_ctx->thread_pool_size = atol(optarg);
	  break;
	case 't':
	  coll_ctx->file_time = atol(optarg);
	  break;
	case 'f':
	  coll_ctx->file_path_str = optarg;
	  break;
	default:
	  goto usage;
      }
    }
    
    // Non-option arguments
    if(optind+1 != argc) {
      usage:
      fprintf(stderr,
	"Usage:\n"
	"  %s portnumber\n"
	"  [-p <thread_pool_size>]\n"
	"  [-t <file_time>] [-f <file_path>]\n"
#if defined(APACHE_KAFKA)
	"  [-m] <hostname1>:<portnumber1>,...,<hostnameN>:<portnumberN>[#bandwidth]\n"
	"  [-u] <topic>\n"
#endif
	"\n"
	"Arguments:\n"
	"  portnumber               Port number\n"
	"\n"
	"Options:\n"
	"  -p <thread_pool_size>    Maximal number of simultaneous connections [default=%u]\n"
	"  -t <file_time>           Time in seconds after which to split files [default=%u]\n"
	"  -f <file_path>           Path to folder where collected data can be stored [default=%s]\n"
	"",
	argv[0],
	coll_ctx->thread_pool_size,
	coll_ctx->file_time,
	coll_ctx->file_path_str);
      exit(1);
    } else {
      coll_ctx->portnumber = atol(argv[optind]);
    }
  }
  
  // Initialize collector context
  coll_ctx = (CollectorCTX *) malloc(sizeof(CollectorCTX));
  coll_ctx->thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
  coll_ctx->file_time = DEFAULT_FILE_TIME;
  coll_ctx->file_path_str = DEFAULT_FILE_PATH_STR;
  parse_args(argc, argv);
  coll_ctx->thread_pool = QUE_initialize(coll_ctx->thread_pool_size);
  THR_initialize(&(coll_ctx->thread), THR_COLLECTOR);
    
  // Initialize reception threads
  recp_ctx = (ReceptionCTX **) malloc(coll_ctx->thread_pool_size*sizeof(ReceptionCTX *));
  for(i=0; i<coll_ctx->thread_pool_size; ++i) {
    // Initialize reception context
    recp_ctx[i] = (ReceptionCTX *) malloc(sizeof(ReceptionCTX));
    recp_ctx[i]->tcp_c = NULL;
    recp_ctx[i]->file_time = coll_ctx->file_time;
    recp_ctx[i]->file_path_str = coll_ctx->file_path_str;
    THR_initialize(&(recp_ctx[i]->thread), THR_RECEPTION+i);
    // Initialize reception arguments
    recp_arg = (ReceptionARG *) malloc(sizeof(ReceptionARG));
    recp_arg->coll_ctx = coll_ctx;
    recp_arg->recp_ctx = recp_ctx[i];
    // Start reception threads
    pthread_mutex_lock(recp_ctx[i]->thread->lock);
    pthread_create(recp_ctx[i]->thread->fd, NULL, reception, recp_arg);
    pthread_cond_wait(coll_ctx->thread->awake, recp_ctx[i]->thread->lock);
    pthread_mutex_unlock(recp_ctx[i]->thread->lock);
    // Add reception thread to thread pool
    QUE_insert(coll_ctx->thread_pool, recp_ctx[i]);
  }
  
#if defined(VERBOSE)
  fprintf(stderr, "[COLL] Started.\n");
#endif

    // Initialize TCP connections
#if defined(VERBOSE)
  tcp_init_p(&tcp_wel, NULL, coll_ctx->portnumber);
  tcp_listen_p(tcp_wel, coll_ctx->thread_pool_size);
#else
  tcp_init(&tcp_wel, NULL, coll_ctx->portnumber);
  tcp_listen(tcp_wel, coll_ctx->thread_pool_size);
#endif
  
  // Ctrl-C signal catcher
  signal(SIGINT, terminate);
  
  while(1) {
    // Wait for thread pool not being empty
    pthread_mutex_lock(coll_ctx->thread_pool->mut);
    while(coll_ctx->thread_pool->empty) {
      pthread_cond_wait(coll_ctx->thread_pool->notEmpty, coll_ctx->thread_pool->mut);
    }
    // Pull thread out of the pool
    r_ctx = (ReceptionCTX *) QUE_remove(coll_ctx->thread_pool);
    pthread_mutex_unlock(coll_ctx->thread_pool->mut);
    pthread_cond_signal(coll_ctx->thread_pool->notFull);
    
    // Accept incoming TCP connections
#if defined(VERBOSE)
    tcp_accept_p(tcp_wel, &tcp_cli);
#else
    tcp_accept(tcp_wel, &tcp_cli);
#endif
    
    // Awake thread to serve TCP connection
    pthread_mutex_lock(r_ctx->thread->lock);
    r_ctx->tcp_c = tcp_cli;
    pthread_cond_signal(r_ctx->thread->awake);
    pthread_mutex_unlock(r_ctx->thread->lock);
  }
  
  return 0;
}

static void* reception(void *args) {
  int                  q_size;
  uint32_t             reduced_fft_size, data_size, payload_size, *payload_buf;
  
  ReceptionARG         *recp_arg;
  CollectorCTX         *coll_ctx;
  ReceptionCTX         *recp_ctx;
  
  TCP_Connection       *tcp_c = NULL;
  
  Item                 *iout;
  
  Queue                *q_decmpr = NULL;
  DecompressionCTX     *decmpr_ctx;
  DecompressionARG     *decmpr_arg;
  
  Queue                *q_stor = NULL;
  StoringCTX           *stor_ctx;
  StoringARG           *stor_arg;
  
  // Parse arguments
  recp_arg = (ReceptionARG *) args;
  coll_ctx = (CollectorCTX *) recp_arg->coll_ctx;
  recp_ctx = (ReceptionCTX *) recp_arg->recp_ctx;
  
#if defined(VERBOSE) || defined(VERBOSE_RECP)
  fprintf(stderr, "[RECP] ID:\t%u\t Started.\n", recp_ctx->thread->id);
#endif
  
  // Signal collector that we are ready
  pthread_mutex_lock(recp_ctx->thread->lock);
  pthread_cond_signal(coll_ctx->thread->awake);
  while(recp_ctx->thread->is_running) {
    // Await requests
    pthread_cond_wait(recp_ctx->thread->awake, recp_ctx->thread->lock);
    if(!recp_ctx->thread->is_running) break;

    // BEGIN SERVING REQUEST
#if defined(VERBOSE) || defined(VERBOSE_RECP)
    fprintf(stderr, "[RECP] ID:\t%u\t Request received.\n", recp_ctx->thread->id);
#endif

    tcp_c = recp_ctx->tcp_c;
    
    // Initialize data processing queues
    q_size = 1000;
    q_decmpr = QUE_initialize(q_size);
    q_stor = QUE_initialize(q_size);
    
    // Initialize data processing contexts
    decmpr_ctx = (DecompressionCTX *) malloc(sizeof(DecompressionCTX));
    THR_initialize(&(decmpr_ctx->thread), recp_ctx->thread->id);
    
    stor_ctx = (StoringCTX *) malloc(sizeof(StoringCTX));
    stor_ctx->file_time = recp_ctx->file_time;
    stor_ctx->file_path_str = recp_ctx->file_path_str;
    THR_initialize(&(stor_ctx->thread), recp_ctx->thread->id);

    // Initialize data processing arguments
    decmpr_arg = (DecompressionARG *) malloc(sizeof(DecompressionARG));
    decmpr_arg->recp_ctx = recp_ctx;
    decmpr_arg->decmpr_ctx = decmpr_ctx;
    decmpr_arg->qin = q_decmpr;
    decmpr_arg->qsout_cnt = 1;
    decmpr_arg->qsout = (Queue **) malloc(decmpr_arg->qsout_cnt*sizeof(Queue *));
    decmpr_arg->qsout[0] = q_stor;
    
    stor_arg = (StoringARG *) malloc(sizeof(StoringARG));
    stor_arg->recp_ctx = recp_ctx;
    stor_arg->stor_ctx = stor_ctx;
    stor_arg->qin = q_stor;
    stor_arg->qsout_cnt = 0;
    stor_arg->qsout = (Queue **) malloc(stor_arg->qsout_cnt*sizeof(Queue *));
    
    // Start data processing threads
    pthread_create(decmpr_ctx->thread->fd, NULL, decompression, decmpr_arg);
    pthread_create(stor_ctx->thread->fd, NULL, storing, stor_arg);
    
    
    // Serve request
    while(recp_ctx->thread->is_running) {
      pthread_mutex_unlock(recp_ctx->thread->lock);
      
      // Read header
#if defined(VERBOSE) || defined(VERBOSE_RECP)
      tcp_read_p(tcp_c, &data_size, sizeof(uint32_t));
#else
      tcp_read(tcp_c, &data_size, sizeof(uint32_t));
#endif
      data_size = ntohl(data_size);
      if(data_size <= 0) break;

#if defined(VERBOSE) || defined(VERBOSE_RECP)
      tcp_read_p(tcp_c, &reduced_fft_size, sizeof(uint32_t));
#else
      tcp_read(tcp_c, &reduced_fft_size, sizeof(uint32_t));
#endif
      reduced_fft_size = ntohl(reduced_fft_size);
      
      // Read payload
      payload_size = (data_size + 3) & ~0x03;
      payload_buf = (uint32_t *) malloc(payload_size);

#if defined(VERBOSE) || defined(VERBOSE_RECP)
      tcp_read_p(tcp_c, payload_buf, payload_size);
#else
      tcp_read(tcp_c, payload_buf, payload_size);
#endif
      
      // Initialize output item
      iout = ITE_init();
      iout->reduced_fft_size = reduced_fft_size;
      iout->data_size = data_size;
      iout->data = payload_buf;
      
      // Wait for output queue not being full
      pthread_mutex_lock(q_decmpr->mut);
      while(q_decmpr->full) {
	pthread_cond_wait(q_decmpr->notFull, q_decmpr->mut);
#if defined(VERBOSE) || defined(VERBOSE_RECP)
	fprintf(stderr, "[RECP] ID:\t%u\t QA full.\n", recp_ctx->thread->id);
#endif
      }
      // Write item to output queue
      QUE_insert(q_decmpr, iout);
      pthread_mutex_unlock(q_decmpr->mut);
      pthread_cond_signal(q_decmpr->notEmpty);
#if defined(VERBOSE) || defined(VERBOSE_RECP)
      fprintf(stderr, "[RECP] ID:\t%u\t Push item.\n", recp_ctx->thread->id);
#endif
      
      pthread_mutex_lock(recp_ctx->thread->lock);
    }
    
    // Signal that we are done and no further items will appear in the queue
    pthread_mutex_lock(q_decmpr->mut);
    q_decmpr->exit = 1;
    pthread_cond_signal(q_decmpr->notEmpty);
    pthread_cond_signal(q_decmpr->notFull);
    pthread_mutex_unlock(q_decmpr->mut);
    
    // Join data processing threads
    pthread_join(*(decmpr_ctx->thread->fd), NULL);
    pthread_join(*(stor_ctx->thread->fd), NULL);
    
    // Free output queues
    free(decmpr_arg->qsout);
    free(stor_arg->qsout);
    
    // Free data processing arguments
    free(decmpr_arg);
    free(stor_arg);
    
    // Free data processing contexts
    THR_release(decmpr_ctx->thread);
    free(decmpr_ctx);
    THR_release(stor_ctx->thread);
    free(stor_ctx);
    
    // Free data processing queues
    QUE_release(q_decmpr);
    QUE_release(q_stor);

    // Disconnect TCP connection
#if defined(VERBOSE) || defined(VERBOSE_RECP)
    tcp_disconnect_p(tcp_c);
    tcp_release_p(tcp_c);
#else
    tcp_disconnect(tcp_c);
    tcp_release(tcp_c);
#endif
    
#if defined(VERBOSE) || defined(VERBOSE_RECP)
    fprintf(stderr, "[RECP] ID:\t%u\t Request served.\n", recp_ctx->thread->id);
#endif
    recp_ctx->tcp_c = NULL;
    
    // END SERVING REQUEST

    // Wait for thread pool not being full
    pthread_mutex_lock(coll_ctx->thread_pool->mut);
    while(coll_ctx->thread_pool->full) {
#if defined(VERBOSE) || defined(VERBOSE_RECP)
      fprintf(stderr, "[RECP] ID:\t%u\t Thread poll full.\n", recp_ctx->thread->id);
#endif
      pthread_cond_wait(coll_ctx->thread_pool->notFull, coll_ctx->thread_pool->mut);
    }
    // Reinsert thread to pool
    QUE_insert(coll_ctx->thread_pool, recp_ctx);
    pthread_mutex_unlock(coll_ctx->thread_pool->mut);
    pthread_cond_signal(coll_ctx->thread_pool->notEmpty);    
  }

#if defined(VERBOSE) || defined(VERBOSE_RECP)
  tcp_disconnect_p(recp_ctx->tcp_c);
  tcp_release_p(recp_ctx->tcp_c);
#else
  tcp_disconnect(recp_ctx->tcp_c);
  tcp_release(recp_ctx->tcp_c);
#endif
  pthread_mutex_unlock(recp_ctx->thread->lock);
  
#if defined(VERBOSE) || defined(VERBOSE_RECP)
  fprintf(stderr, "[RECP] ID:\t%u\t Terminated.\n", recp_ctx->thread->id);
#endif
  
  free(recp_arg);
  
  pthread_exit(NULL);
}

static void* decompression(void *args) {
  int r, i;
  
  uint32_t reduced_fft_size, prev_reduced_fft_size = 0;
  
  size_t dst_len = 0, tgt_len = 0;
  uint32_t *dst_buf = NULL, *tgt_buf = NULL;
  
  DecompressionARG *decmpr_arg;
  ReceptionCTX     *recp_ctx;
  
  size_t k, qsout_cnt;
  Queue *qin, **qsout;
  Item *iin, *iout, *nout;
  
  // Parse arguments
  decmpr_arg = (DecompressionARG *) args;
  recp_ctx = (ReceptionCTX *) decmpr_arg->recp_ctx;
  qin = (Queue *) decmpr_arg->qin;
  qsout = (Queue **) decmpr_arg->qsout;
  qsout_cnt = decmpr_arg->qsout_cnt;
  
  while(1) {
    // Wait for input queue not being empty
    pthread_mutex_lock(qin->mut);
    while(qin->empty) {
      // No more input is coming to this queue
      if(qin->exit) {
	pthread_mutex_unlock(qin->mut);
	goto EXIT;
      }
      // Wait for more input coming to this queue
      pthread_cond_wait(qin->notEmpty, qin->mut);
    }
    
    // Read item from input queue
    iin = (Item *) QUE_remove(qin);
    pthread_mutex_unlock(qin->mut);
    pthread_cond_signal(qin->notFull);
    
    reduced_fft_size = iin->reduced_fft_size;
    
    // Uncompressed data buffer
    if(reduced_fft_size != prev_reduced_fft_size) {
      dst_len = (4+reduced_fft_size)*sizeof(uint32_t);
      dst_buf = realloc(dst_buf, dst_len);
      prev_reduced_fft_size = reduced_fft_size;
    }
    
    // Compressed data buffer
    tgt_len = iin->data_size;
    tgt_buf = iin->data;
    
    r = uncompress((Bytef *) dst_buf, (uLongf *) &dst_len, (Bytef *) tgt_buf, (uLong) tgt_len);
    
    // Error handling
    if(r != Z_OK) {
      // TODO: How should we handle iout in case of an error?
      switch(r) {
	case Z_MEM_ERROR:
	  fprintf(stderr, "[DCMP] ID:\t%u\t Error: Not enough memory for decompression.\n",
		  recp_ctx->thread->id);
	  break;
	case Z_BUF_ERROR:
	  fprintf(stderr, "[DCMP] ID:\t%u\t Error: Destination buffer too small.\n",
		  recp_ctx->thread->id);
	  break;
	case Z_DATA_ERROR:
	  fprintf(stderr, "[DCMP] ID:\t%u\t Error: Incomplete or corrupted input data.\n",
		  recp_ctx->thread->id);
	  break;
      }
    }
    
    // Free compressed data buffer
    free(tgt_buf);
    
    // Unmarshalling data
    iout = iin;
    iout->Fc = ntohl(dst_buf[0]);
    iout->Ts_sec = ntohl(dst_buf[1]);
    iout->Ts_usec = ntohl(dst_buf[2]);
    iout->freq_res = unpack754_32(ntohl(dst_buf[3]));
    
    iout->samples = (float *) malloc(reduced_fft_size*sizeof(float));
    for(i=0; i<reduced_fft_size; ++i) iout->samples[i] = unpack754_32(ntohl(dst_buf[4+i]));
    
    // Single output queue
    if(qsout_cnt == 1) {
      // Wait for output queue not being full
      pthread_mutex_lock(qsout[0]->mut);
      while(qsout[0]->full) pthread_cond_wait(qsout[0]->notFull, qsout[0]->mut);
      // Write item to output queue
      QUE_insert(qsout[0], iout);
      pthread_mutex_unlock(qsout[0]->mut);
      pthread_cond_signal(qsout[0]->notEmpty);
    }
    // Multiple output queues
    else {
      // Put item to output queues
      for(k=0; k<qsout_cnt; ++k) {
	// Wait for output queue not being full
	pthread_mutex_lock(qsout[k]->mut);
	while(qsout[k]->full) pthread_cond_wait(qsout[k]->notFull, qsout[k]->mut);
	nout = ITE_copy(iout);
	// Write item to output queue
	QUE_insert(qsout[k], nout);
	pthread_mutex_unlock(qsout[k]->mut);
	pthread_cond_signal(qsout[k]->notEmpty);
      }
      // Release item
      ITE_free(iout);
    }
  }
EXIT:
  
  // Signal that we are done and no further items will appear in the queues
  for(k=0; k<qsout_cnt; ++k) {
    pthread_mutex_lock(qsout[k]->mut);
    qsout[k]->exit = 1;
    pthread_cond_signal(qsout[k]->notEmpty);
    pthread_cond_signal(qsout[k]->notFull);
    pthread_mutex_unlock(qsout[k]->mut);
  }
  
  if(dst_buf != NULL) free(dst_buf);
  
#if defined(VERBOSE) || defined(VERBOSE_DECMPR)
  fprintf(stderr, "[DCMP] Terminated.\n");
#endif
  
  pthread_exit(NULL);
}

static void* storing(void *args) {
  int i, t;
  
  uint32_t freq;
  uint32_t center_freq, prev_center_freq = 0;
  uint32_t reduced_fft_size;
  float    freq_res;
  float    *samples;
  
  unsigned int   portnumber;
  char           hostaddr[INET_ADDRSTRLEN];
  TCP_Connection *tcp_c;
  
  unsigned int file_time;
  char         *file_path_str;
  time_t start_t = 0, current_t;
  char datetime[32], filename[128];
  FILE *file = NULL;

  StoringARG   *stor_arg;
  ReceptionCTX *recp_ctx;
  
  Item  *iin;
  Queue *qin;
  
  // Parse arguments
  stor_arg = (StoringARG *) args;
  recp_ctx = (ReceptionCTX *) stor_arg->recp_ctx;
  qin = (Queue *) stor_arg->qin;
  
  // TCP connection information
  tcp_c = recp_ctx->tcp_c;
  portnumber = ntohs(tcp_c->host_addr.sin_port);
  inet_ntop(AF_INET, &(tcp_c->host_addr.sin_addr), hostaddr, INET_ADDRSTRLEN);
  
  // TODO: Don't access recp_ctx here but stor_ctx
  // Time in seconds after which to split files
  file_time = recp_ctx->file_time;
  file_path_str = recp_ctx->file_path_str;
  snprintf(filename, sizeof(filename)-1, "%s%s/", file_path_str, hostaddr);
  mkdir(filename, 0777);
  
  // Store full plain-text data to file
  while(1) {
    // Wait for input queue not being empty
    pthread_mutex_lock(qin->mut);
    while(qin->empty) {
      // No more input is coming to this queue
      if(qin->exit) {
	pthread_mutex_unlock(qin->mut);
	goto EXIT;
      }
      // Wait for more input coming to this queue
      pthread_cond_wait(qin->notEmpty, qin->mut);
    }
    
    // Read item from input queue
    iin = (Item *) QUE_remove(qin);
    pthread_mutex_unlock(qin->mut);
    pthread_cond_signal(qin->notFull);
    
    center_freq = iin->Fc;
    reduced_fft_size = iin->reduced_fft_size;
    freq_res = iin->freq_res;
    samples = iin->samples;
    
    // Create new file every file_time seconds
    time(&current_t);
    if(file == NULL || 
      ((difftime(current_t, start_t) > file_time) && (center_freq < prev_center_freq))
    ) {
      if(file != NULL) fclose(file);
      strftime(datetime, sizeof(datetime)-1, "%Y-%m-%d_%H:%M:%S", localtime(&current_t));
      snprintf(filename, sizeof(filename)-1, "%s%s/%s_%s:%u.csv",
	      file_path_str, hostaddr, datetime, hostaddr, portnumber);
      file = fopen(filename, "w");
      if(file == NULL) {
	// TODO: Error handling
	fprintf(stderr, "[STOR] ID:\t%u\t Error: Failed to open file %s.\n",
		recp_ctx->thread->id, filename);
      }
      start_t = current_t;
    }
    
//     if(difftime(current_t, start_t) > file_time) {
//       if(file != NULL) fclose(file);
//       strftime(datetime, sizeof(datetime)-1, "%Y-%m-%d_%H:%M:%S", localtime(&current_t));
//       snprintf(filename, sizeof(filename)-1, "%s%s/%s_%s:%u.csv",
// 	       file_path_str, hostaddr, datetime, hostaddr, portnumber);
//       file = fopen(filename, "w");
//       if(file == NULL) {
// 	// TODO: Error handling
// 	fprintf(stderr, "[STOR] ID:\t%u\t Error: Failed to open file %s.\n",
// 		recp_ctx->thread->id, filename);
//       }
//       start_t = current_t;
//     }
    // NOTE: Ensuring new file is created after finishing current sweep.
    // NOTE: This currently has to be done due to the limitations of plotting.
    prev_center_freq = center_freq;
    
    // Write item to file
    t = reduced_fft_size / 2;
    for(i=0; i<reduced_fft_size; ++i) {
      freq = center_freq - (t-i)*freq_res;
      fprintf(file, "%u,%u,%u,%.1f\n", iin->Ts_sec, iin->Ts_usec, freq, samples[i]);
    }
    
    // Release item
    free(samples);
    free(iin);

  }
  
EXIT:
  
#if defined(VERBOSE) || defined(VERBOSE_STOR)
  fprintf(stderr, "[STOR] Terminated.\n");
#endif
  
  if(file != NULL) fclose(file);
  
  pthread_exit(NULL);
}