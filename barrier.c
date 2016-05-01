#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

// #define SOL

static int nthread = 1;

struct barrier {
  pthread_mutex_t barrier_mutex;
  pthread_cond_t barrier_cond;
  int nthread;        // Number of threads that have reached this round of the barrier
  int nthread_exited; // Number of threads that have exited this round of the barrier
  int round;          // Barrier round
} bstate;

static void
barrier_init(void)
{
  assert(pthread_mutex_init(&bstate.barrier_mutex, NULL) == 0);
  assert(pthread_cond_init(&bstate.barrier_cond, NULL) == 0);
  bstate.nthread = 0;
  bstate.nthread_exited = nthread;
}

static void 
barrier()
{
  int is_last_reached = 1;

  pthread_mutex_lock(&bstate.barrier_mutex);

  // Wait for other threads to exit the last round
  while (bstate.nthread_exited < nthread)  
	pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);

  bstate.nthread++;
  while (bstate.nthread < nthread) {  // The real barrier
    is_last_reached = 0;
	pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
  }
  
  if (is_last_reached) { 
    // We are the last to reach the barrier in a round
    bstate.round++;
    bstate.nthread_exited = 0;
	// Wake up all threads waiting for the condition:
	//     bstate.nthread == nthread
  	pthread_cond_broadcast(&bstate.barrier_cond);
  }
  
  if (++bstate.nthread_exited == nthread) {
  	// We are the last to exit current barrier round
	bstate.nthread = 0;
	// Wake up all threads waiting for the condition:
	//     bstate.nthread_exited == nthread
  	pthread_cond_broadcast(&bstate.barrier_cond);
  }
  
  pthread_mutex_unlock(&bstate.barrier_mutex);
}

static void *
thread(void *xa)
{
  long n = (long) xa;
  long delay;
  int i;

  for (i = 0; i < 20000; i++) {
    int t = bstate.round;
    assert (i == t);
    barrier();
    usleep(random() % 100);
  }
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  long i;
  double t1, t0;

  if (argc < 2) {
    fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);

  barrier_init();

  for(i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
  }
  for(i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  printf("OK; passed\n");
}