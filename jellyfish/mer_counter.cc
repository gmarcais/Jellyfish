#include <config.h>
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
#include "mer_counting.hpp"
#include "mer_count_thread.hpp"
#include "locks_pthread.hpp"
#include "time.hpp"

/*
 * Option parsing
 */
static char doc[] = "Count k-mers in a fasta files and write tables";
static char args_doc[] = "fasta ...";
     
enum {
  OPT_VAL_LEN = 1024,
  OPT_BUF_SIZE,
  OPT_TIMING,
  OPT_OBUF_SIZE,
  OPT_BOTH
};
static struct argp_option options[] = {
  {"threads",		't',		"NB",   0, "Nb of threads"},
  {"mer-len",           'm',		"LEN",  0, "Length of a mer"},
  {"counter-len",       'c',		"LEN",  0, "Length (in bits) of counting field"},
  {"output",		'o',		"FILE", 0, "Output file"},
  {"out-counter-len",   OPT_VAL_LEN,	"LEN",  0, "Length (in bytes) of counting field in output"},
  {"buffers",           'b',		"NB",   0, "Nb of buffers per thread"},
  {"both-strands",      OPT_BOTH,       0,      0, "Count both strands"},
  {"buffer-size",       OPT_BUF_SIZE,	"SIZE", 0, "Size of a buffer"},
  {"out-buffer-size",   OPT_OBUF_SIZE,  "SIZE", 0, "Size of output buffer per thread"},
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
  unsigned long	 out_buffer_size;
  bool           no_write;
  bool           raw;
  bool           both_strands;
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
  case OPT_BOTH: FLAG(both_strands);
  case 'o': STRING(output);
  case OPT_VAL_LEN: ULONGP(out_counter_len);
  case OPT_BUF_SIZE: ULONGP(buffer_size);
  case OPT_TIMING: STRING(timing);
  case OPT_OBUF_SIZE: ULONGP(out_buffer_size);

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
static struct argp argp = { options, parse_opt, args_doc, doc };

// TODO: the following 3 functions should be part of a class.
void initialize(int arg_st, int argc, char *argv[],
                struct arguments *arguments, struct qc *qc, struct io *io) {
  unsigned int nb_buffers = arguments->nb_threads * arguments->nb_buffers;
  unsigned int i;
  seq *buffers;
  int fd;
  struct stat stat;
  storage_t *ary = new storage_t(arguments->size, 2*arguments->mer_len,
                                 arguments->counter_len, arguments->reprobes,
                                 jellyfish::quadratic_reprobes);
  hash_dumper_t *dumper = NULL;
  if(!arguments->no_write)
    dumper = new hash_dumper_t(arguments->nb_threads, arguments->output,
                               arguments->out_buffer_size,
                               8*arguments->out_counter_len,
                               ary);

  qc->rq = new seq_queue(nb_buffers);
  qc->wq = new seq_queue(nb_buffers);
  qc->counters = new mer_counters(ary);
  qc->counters->set_dumper(dumper);

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

/* TODO: This data structure to pass arguments to the threads has become way
 * to big! And the do_it function is a mess. Refactor!!
 */
struct worker_info {
  locks::pthread::barrier       *barrier;
  thread_worker			*worker;
  pthread_t			 thread;
  pthread_mutex_t		*write_lock;
  unsigned int			 id;
  struct qc			*qc;
  struct arguments		*arguments;
};

void *start_worker(void *worker) {
  struct worker_info *info = (struct worker_info *)worker;
  bool is_serial;

  info->barrier->wait();
  try {
    info->worker->start();
  } catch(exception &e) {
    std::cerr << e.what() << std::endl;
  }

  is_serial = info->barrier->wait() == PTHREAD_BARRIER_SERIAL_THREAD;

  if(is_serial)
    info->qc->counters->dump();

  return NULL;
}

void do_it(struct arguments *arguments, struct qc *qc, struct io *io) {
  unsigned int			i;
  struct worker_info		workers[arguments->nb_threads];
  struct thread_stats		thread_stats;
  locks::pthread::barrier	worker_barrier(arguments->nb_threads);
  pthread_mutex_t		write_lock;

  memset(&thread_stats, '\0', sizeof(thread_stats));
  memset(&workers, '\0', sizeof(workers));
  pthread_mutex_init(&write_lock, NULL);
  
  for(i = 0; i < arguments->nb_threads; i++) {
    workers[i].barrier = &worker_barrier;
    workers[i].worker = new thread_worker(i+1, arguments->mer_len, qc, io, 
                                          &thread_stats);
    workers[i].worker->set_both_strands(arguments->both_strands);
    workers[i].write_lock = &write_lock;
    workers[i].id = i;
    workers[i].qc = qc;
    workers[i].arguments = arguments;
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
}

int count_main(int argc, char *argv[]) {
  struct arguments arguments;
  int arg_st;
  struct qc qc;
  struct io io;

  arguments.nb_threads = 1;
  arguments.mer_len = 12;
  arguments.counter_len = 32;
  arguments.out_counter_len = 4;
  arguments.size = 1000000UL;
  arguments.reprobes = 50;
  arguments.nb_buffers = 100;
  arguments.buffer_size = 4096;
  arguments.out_buffer_size = 20000000UL;
  arguments.no_write = false;
  arguments.raw = false;
  arguments.both_strands = false;
  arguments.timing = NULL;
  arguments.output = (char *)"mer_counts.hash";
  argp_parse(&argp, argc, argv, 0, &arg_st, &arguments);
  if(arg_st == argc) {
    fprintf(stderr, "Missing arguments\n");
    argp_help(&argp, stderr, ARGP_HELP_SEE, argv[0]);
    exit(1);
  }
  if(arguments.raw)
    die("--raw switch not supported anymore. Fix me!");

  Time start;
 
  initialize(arg_st, argc, argv, &arguments, &qc, &io);

  Time after_init;

  do_it(&arguments, &qc, &io);

  Time all_done;

  if(arguments.timing) {
    std::ofstream timing_fd(arguments.timing);
    if(!timing_fd.good()) {
      fprintf(stderr, "Can't open timing file '%s': %s\n",
	      arguments.timing, strerror(errno));
    } else {
      Time writing = qc.counters->get_writing_time();
      Time counting = (all_done - after_init) - writing;
      timing_fd << "Init      " << (after_init - start).str() << std::endl;
      timing_fd << "Counting  " << counting.str() << std::endl;
      timing_fd << "Writing   " << writing.str() << std::endl;
    }
  }

  return 0;
}
