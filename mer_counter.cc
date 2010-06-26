#include <pthread.h>
#include <fstream>
#include <exception>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <argp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include "mer_counting.hpp"
#include "mer_count_thread.hpp"

void thread_printf(char *fmt, ...) {
//   va_list ap;
//   pthread_t self = pthread_self();
//   unsigned long i;
  
//   va_start(ap, fmt);

//   flockfile(stdout);
//   for(i = 0; i < sizeof(pthread_t); i++) {
//     printf("%02x.", (unsigned char)*((char *)&self + i));
//   }
//   printf(" ");

//   vprintf(fmt, ap);
//   funlockfile(stdout);

//   va_end(ap);
}

/*
 * Option parsing
 */
const char *argp_program_version = "mer_counter 1.0";
const char *argp_program_bug_address = "<guillaume@marcais.net>";
static char doc[] = "Multi-threaded mer counter.";
static char args_doc[] = "fasta ...";
     
enum {
  OPT_VAL_LEN = 1024,
  OPT_BUF_SIZE,
  OPT_TIMING
};
static struct argp_option options[] = {
  {"threads",		't',		"NB",   0, "Nb of threads"},
  {"mer-len",           'm',		"LEN",  0, "Length of a mer"},
  {"counter-len",       'c',		"LEN",  0, "Length (in bits) of counting field"},
  {"output",		'o',		"FILE", 0, "Output file"},
  {"out-counter-len",   OPT_VAL_LEN,	"LEN",  0, "Length (in bytes) of counting field in output"},
  {"buffers",           'b',		"NB",   0, "Nb of buffers per thread"},
  {"buffer-size",       OPT_BUF_SIZE,	"SIZE", 0, "Size of a buffer"},
  {"hash-size",         's',		"SIZE", 0, "Initial hash size"},
  {"reprobes",          'p',    "NB",   0, "Maximum number of reprobing"},
  {"no-write",          'w',		0,      0, "Don't write hash to disk"},
  {"raw",               'r',		0,      0, "Dump raw database"},
  {"timing",            OPT_TIMING,	"FILE", 0, "Print timing information to FILE"},
  { 0 }
};

struct arguments {
  unsigned long	 nb_threads;
  unsigned long	 buffer_size;
  unsigned int	 nb_buffers;
  unsigned int	 mer_len;
  unsigned int	 counter_len;
  unsigned int	 out_counter_len;
  unsigned int   reprobes;
  unsigned long	 size;
  bool           no_write;
  bool           raw;
  char          *timing;
  char          *output;
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = (struct arguments *)state->input;

#define ULONGP(field) errno = 0; \
arguments->field = strtoul(arg,NULL,0); \
if(errno) return errno; \
break;

#define FLAG(field) arguments->field = true; break;

#define STRING(field) arguments->field = arg; break;

  switch(key) {
  case 't': ULONGP(nb_threads); 
  case 's': ULONGP(size);
  case 'b': ULONGP(nb_buffers);
  case 'm': ULONGP(mer_len);
  case 'c': ULONGP(counter_len);
  case 'p': ULONGP(reprobes);
  case 'w': FLAG(no_write);
  case 'r': FLAG(raw);
  case 'o': STRING(output);
  case OPT_VAL_LEN: ULONGP(out_counter_len);
  case OPT_BUF_SIZE: ULONGP(buffer_size);
  case OPT_TIMING: STRING(timing);

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
static struct argp argp = { options, parse_opt, args_doc, doc };

void initialize(int arg_st, int argc, char *argv[],
                struct arguments *arguments, struct qc *qc, struct io *io) {
  unsigned int nb_buffers = arguments->nb_threads * arguments->nb_buffers;
  unsigned int i;
  seq *buffers;
  int fd;
  struct stat stat;

  qc->rq = new seq_queue(nb_buffers);
  qc->wq = new seq_queue(nb_buffers);
  qc->counters = new mer_counters(new storage_t(arguments->size, 
                                                2*arguments->mer_len, 
                                                arguments->counter_len, 
                                                arguments->reprobes,
                                                jellyfish::quadratic_reprobes)); 

  buffers = new seq[nb_buffers];
  memset(buffers, '\0', sizeof(seq) * nb_buffers);

  for(i = 0; i < nb_buffers; i++)
    qc->wq->enqueue(&buffers[i]);

  // Init io. Mmap all input fast files.
  for( ; arg_st < argc; arg_st++) {
    fd = open(argv[arg_st], O_RDONLY);
    if(fd < 0)
      die((char *)"Can't open file '%s': %s\n", argv[arg_st], strerror(errno));

    if(fstat(fd, &stat) < 0)
      die((char *)"Can't stat file '%s': %s\n", argv[arg_st], strerror(errno));

    char *map_base = (char *)mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if(map_base == MAP_FAILED)
      die((char *)"Can't mmap file '%s': %s\n", argv[arg_st], strerror(errno));
    close(fd);
    madvise(map_base, stat.st_size, MADV_SEQUENTIAL);
    io->mapped_files.push_back(mapped_file(map_base, stat.st_size));
  }
  io->current_file = io->mapped_files.begin();
  madvise(io->current_file->base, io->current_file->length, MADV_WILLNEED);
  io->map_base	   = io->current_file->base;
  io->current	   = io->current_file->base;
  io->map_end	   = io->current_file->end;
  io->thread_id	   = 0;
  io->buffer_size  = arguments->buffer_size;
  io->nl	   = true;
  io->ns	   = false;
}

/* This data structure to pass arguments to the threads has become way
 * to big! And the do_it function is a mess. Refactor!!
 */

struct trailer {
  uint64_t unique;
  uint64_t distinct;
  uint64_t total;
};

struct worker_info {
  pthread_barrier_t *barrier;
  thread_worker     *worker;
  pthread_t          thread;
  pthread_mutex_t   *write_lock;
  unsigned int       id;
  ostream           *out;
  struct qc         *qc;
  struct arguments  *arguments;
  struct timeval    *after_hashing;
  volatile bool     *header_written;
  struct trailer    *trailer;
};

void *start_worker(void *worker) {
  struct worker_info *info = (struct worker_info *)worker;

  pthread_barrier_wait(info->barrier);
  try {
    info->worker->start();
  } catch(exception &e) {
    std::cerr << e.what() << std::endl;
  }

  if(pthread_barrier_wait(info->barrier) == PTHREAD_BARRIER_SERIAL_THREAD) {
    gettimeofday(info->after_hashing, NULL);
  }

  if(info->arguments->no_write)
    return NULL;

  if(!info->arguments->raw) {
    compacter_t compacter(10000000UL, info->arguments->mer_len,
                          info->arguments->out_counter_len);
    // Write header    
    if(!pthread_mutex_trylock(info->write_lock)) {
      if(!*info->header_written) {
        compacter.write_header(info->out, 
                               info->qc->counters->get_size());
        *info->header_written = true;
      }
      pthread_mutex_unlock(info->write_lock);
    }

    mer_iterator_t it = 
      info->qc->counters->iterator_slice(info->id, info->arguments->nb_threads);
    compacter.dump(info->out, it, info->write_lock);
    __sync_add_and_fetch(&info->trailer->unique, compacter.get_unique());
    __sync_add_and_fetch(&info->trailer->distinct, compacter.get_distinct());
    __sync_add_and_fetch(&info->trailer->total, compacter.get_total());

    if(pthread_barrier_wait(info->barrier) == PTHREAD_BARRIER_SERIAL_THREAD) {
      compacter.update_stats(info->out, info->trailer->unique,
                             info->trailer->distinct,
                             info->trailer->total);
      std::cout << info->trailer->unique << " " << info->trailer->distinct << " " << info->trailer->total << " " << std::endl;
    }
  } else { // dump raw
    if(!pthread_mutex_trylock(info->write_lock)) {
      if(!*info->header_written) {
        *info->header_written = true;
        info->qc->counters->write_raw(*info->out);
        pthread_mutex_unlock(info->write_lock);
      }
    }
  }
  
  return NULL;
}

void do_it(struct arguments *arguments, struct qc *qc, struct io *io, ostream *out, struct timeval *after_hashing) {
  unsigned int        i;
  struct worker_info  workers[arguments->nb_threads];
  struct thread_stats thread_stats;
  pthread_barrier_t   worker_barrier;
  pthread_mutex_t     write_lock;
  volatile bool       header_written;
  struct trailer      trailer;

  memset(&thread_stats, '\0', sizeof(thread_stats));
  memset(&workers, '\0', sizeof(workers));
  memset(&trailer, '\0', sizeof(trailer));
  pthread_barrier_init(&worker_barrier, NULL, arguments->nb_threads);
  pthread_mutex_init(&write_lock, NULL);
  
  for(i = 0; i < arguments->nb_threads; i++) {
    workers[i].barrier = &worker_barrier;
    workers[i].worker = new thread_worker(i+1, arguments->mer_len, qc, io, 
                                          &thread_stats);
    workers[i].write_lock = &write_lock;
    workers[i].id = i;
    workers[i].qc = qc;
    workers[i].out = out;
    workers[i].arguments = arguments;
    workers[i].after_hashing = after_hashing;
    workers[i].header_written = &header_written;
    workers[i].trailer = &trailer;
    if(pthread_create(&workers[i].thread, NULL, start_worker, &workers[i])) {
      perror("Can't create thread");
      exit(1);
    }
  }

  for(i = 0; i < arguments->nb_threads; i++) {
    if(pthread_join(workers[i].thread, NULL)) {
      perror("Join failed");
      exit(1);
    }
    delete workers[i].worker;
  }

  pthread_barrier_destroy(&worker_barrier);

  //  printf("empry rq: %u new reader: %u\n", thread_stats.empty_rq,
  //         thread_stats.new_reader);
}

int main(int argc, char *argv[]) {
  struct arguments arguments;
  int arg_st;
  struct qc qc;
  struct io io;

  arguments.nb_threads = 1;
  arguments.mer_len = 12;
  arguments.counter_len = 32;
  arguments.out_counter_len = 4;
  arguments.size = 1000000;
  arguments.reprobes = 50;
  arguments.nb_buffers = 100;
  arguments.buffer_size = 4096;
  arguments.no_write = false;
  arguments.raw = false;
  arguments.timing = NULL;
  arguments.output = (char *)"mer_counts.hash";
  argp_parse(&argp, argc, argv, 0, &arg_st, &arguments);
  if(arg_st == argc) {
    fprintf(stderr, "Missing arguments\n");
    argp_help(&argp, stderr, ARGP_HELP_SEE, argv[0]);
    exit(1);
  }

  struct timeval start, after_init, after_hashing, after_writing;

  gettimeofday(&start, NULL);
  ofstream output(arguments.output);
 
  initialize(arg_st, argc, argv, &arguments, &qc, &io);
  gettimeofday(&after_init, NULL);

  do_it(&arguments, &qc, &io, &output, &after_hashing);
  gettimeofday(&after_writing, NULL);
  
  output.close();
  gettimeofday(&after_writing, NULL);

  if(arguments.timing) {
#define DIFF_SECONDS(e, s)                                      \
    (e.tv_sec - s.tv_sec - ((e.tv_usec > s.tv_usec) ? 0 : 1))
#define DIFF_MICRO(e, s)                                                \
    (e.tv_usec - s.tv_usec + ((s.tv_usec > e.tv_usec) ? 1000000 : 0))

    FILE *timing_fd = fopen(arguments.timing, "w");
    if(!timing_fd) {
      fprintf(stderr, "Can't open timing file '%s': %s\n",
	      arguments.timing, strerror(errno));
    } else {
      fprintf(timing_fd, "Init:     %5ld.%06ld seconds\n",
	      DIFF_SECONDS(after_init, start), DIFF_MICRO(after_init, start));
      fprintf(timing_fd, "Counting: %5ld.%06ld seconds\n",
	      DIFF_SECONDS(after_hashing, after_init), 
	      DIFF_MICRO(after_hashing, after_init));
      fprintf(timing_fd, "Writing:  %5ld.%06ld seconds\n",
	      DIFF_SECONDS(after_writing, after_hashing),
	      DIFF_MICRO(after_writing, after_hashing));
      fclose(timing_fd);
    }
  }

  return 0;
}
