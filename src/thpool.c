/* ********************************
 * Author:       Johan Hanssen Seferidis
 * License:	     MIT
 * Description:  Library providing a threading pool where you can add
 *               work. For usage, check the thpool.h file or README.md
 *
 *//** @file thpool.h *//*
 *
 ********************************/

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/prctl.h>
#endif

#include <pthread.h>

#include "thpool.h"

#ifdef THPOOL_DEBUG
#define THPOOL_DEBUG 1
#else
#define THPOOL_DEBUG 0
#endif

#if !defined(DISABLE_PRINT) || defined(THPOOL_DEBUG)
#define err(str) fprintf(stderr, str)
#else
#define err(str)
#endif

static volatile int threads_keepalive;
static volatile int threads_on_hold;



/* ========================== STRUCTURES ============================ */


/* Binary semaphore */
typedef struct bsem {
	pthread_mutex_t mutex;
	pthread_cond_t   cond;
	int v;
} bsem;


/* Job */
typedef struct job{
	struct job*  prev;                   /* pointer to previous job   */
	void   (*function)(void* arg);       /* function pointer          */
	void*  arg;                          /* function's argument       */
} job;


/* Job queue */
typedef struct jobqueue{
	pthread_mutex_t rwmutex;             /* used for queue r/w access */
	job  *front;                         /* pointer to front of queue */
	job  *rear;                          /* pointer to rear  of queue */
	bsem *has_jobs;                      /* flag as binary semaphore  */
	int   len;                           /* number of jobs in queue   */
} jobqueue;


/* Thread */
typedef struct thread{
	int       id;                        /* friendly id               */
	pthread_t pthread;                   /* pointer to actual thread  */
	struct thpool_* thpool_p;            /* access to thpool          */
} thread;


/* Threadpool */
typedef struct thpool_{
	thread**   threads;                  /* pointer to threads        */
	volatile int num_threads_alive;      /* threads currently alive   */
	volatile int num_threads_working;    /* threads currently working */
	pthread_mutex_t  thcount_lock;       /* used for thread count etc */
	pthread_cond_t  threads_all_idle;    /* signal to thpool_wait     */
	jobqueue  jobqueue;                  /* job queue                 */
} thpool_;





/* ========================== PROTOTYPES ============================ */


static int  thread_init(thpool_* thpool_p, struct thread** thread_p, int id);
static void* thread_do(struct thread* thread_p);
static void  thread_hold(int sig_id);
static void  thread_destroy(struct thread* thread_p);

static int   jobqueue_init(jobqueue* jobqueue_p);
static int  jobqueue_clear(jobqueue* jobqueue_p);
static int  jobqueue_push(jobqueue* jobqueue_p, struct job* newjob_p);
static struct job* jobqueue_pull(jobqueue* jobqueue_p);
static int  jobqueue_destroy(jobqueue* jobqueue_p);

static int  bsem_init(struct bsem *bsem_p, int value);
static int  bsem_reset(struct bsem *bsem_p);
static int  bsem_post(struct bsem *bsem_p);
static int  bsem_post_all(struct bsem *bsem_p);
static int  bsem_wait(struct bsem *bsem_p);





/* ========================== THREADPOOL ============================ */


/* Initialise thread pool */
struct thpool_* thpool_init(int num_threads){
	thpool_* thpool_p;
	int n;

	threads_on_hold   = 0;
	threads_keepalive = 1;

	if (num_threads < 0){
		num_threads = 0;
	}

	/* Make new thread pool */
	thpool_p = (struct thpool_*)malloc(sizeof(struct thpool_));
	error_check (thpool_p == NULL){
		err("thpool_init(): Could not allocate memory for thread pool\n");
		return NULL;
	}
	thpool_p->num_threads_alive   = 0;
	thpool_p->num_threads_working = 0;

	/* Initialise the job queue */
	error_check (jobqueue_init(&thpool_p->jobqueue) == -1){
		err("thpool_init(): Could not allocate memory for job queue\n");
		free(thpool_p);
		return NULL;
	}

	/* Make threads in pool */
	thpool_p->threads = (struct thread**) malloc(
		(size_t) num_threads * sizeof(struct thread *));
	error_check (thpool_p->threads == NULL){
		err("thpool_init(): Could not allocate memory for threads\n");
		jobqueue_destroy(&thpool_p->jobqueue);
		free(thpool_p);
		return NULL;
	}

	/*if (pthread_mutex_init(&(thpool_p->thcount_lock), NULL) != 0) {
		jobqueue_destroy(&thpool_p->jobqueue);
		free(thpool_p);
		return NULL;
	}*/
	thpool_p->thcount_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
	/*if (pthread_cond_init(&thpool_p->threads_all_idle, NULL) != 0) {
		pthread_mutex_destroy (&(thpool_p->thcount_lock));
		jobqueue_destroy(&thpool_p->jobqueue);
		free(thpool_p);
		return NULL;
	}*/
	thpool_p->threads_all_idle = (pthread_cond_t) PTHREAD_COND_INITIALIZER;

	/* Thread init */
	for (n=0; n<num_threads; n++){
		error_check (thread_init(thpool_p, &thpool_p->threads[n], n) != 0) {
			int k;
			for (k = 0; k < n; k++)
				thread_destroy (thpool_p->threads[k]);
			pthread_cond_destroy (&thpool_p->threads_all_idle);
			pthread_mutex_destroy (&(thpool_p->thcount_lock));
			jobqueue_destroy(&thpool_p->jobqueue);
			free(thpool_p);
			return NULL;
		}
#if THPOOL_DEBUG
			printf("THPOOL_DEBUG: Created thread %d in pool \n", n);
#endif
	}

	/* Wait for threads to initialize */
	TODO (this looks busy or somehow optimize-able)
	while (thpool_p->num_threads_alive != num_threads) {}

	return thpool_p;
}


/* Add work to the thread pool */
int thpool_add_work(
	thpool_* thpool_p,
	void (*function_p)(void*), void* arg_p) {
	job* newjob;

	newjob=(struct job*)malloc(sizeof(struct job));
	error_check (newjob==NULL){
		err("thpool_add_work(): Could not allocate memory for new job\n");
		return -1;
	}

	/* add function and argument */
	newjob->function=function_p;
	newjob->arg=arg_p;

	/* add job to queue */
	error_check (jobqueue_push(&thpool_p->jobqueue, newjob) != 0) return -2;

	return 0;
}


/* Wait until all jobs have finished */
int thpool_wait(thpool_* thpool_p){
	error_check (pthread_mutex_lock(&thpool_p->thcount_lock) != 0) return -1;
	TODO (this looks kinda busy or otherwise optimize-able)
	while (thpool_p->jobqueue.len || thpool_p->num_threads_working) {
		error_check (pthread_cond_wait(
			&thpool_p->threads_all_idle,
			&thpool_p->thcount_lock) != 0) return -2;
	}
	error_check (pthread_mutex_unlock(&thpool_p->thcount_lock) != 0)
		return -3;
	return 0;
}


/* Destroy the threadpool */
int thpool_destroy(thpool_* thpool_p){
	volatile int threads_total;

	double TIMEOUT = 1.0;
	time_t start, end;
	double tpassed = 0.0;

	int n;

	/* No need to destory if it's NULL */
	error_check (thpool_p == NULL) return 0;

	threads_total = thpool_p->num_threads_alive;

	/* End each thread 's infinite loop */
	threads_keepalive = 0;

	/* Give one second to kill idle threads */
	error_check (time (&start) == (time_t) -1) return -1;
	TODO (busy?)
	while (tpassed < TIMEOUT && thpool_p->num_threads_alive){
		error_check (bsem_post_all(thpool_p->jobqueue.has_jobs) != 0)
			return -2;
		error_check (time (&end) == (time_t) -1) return -3;
		tpassed = difftime(end,start);
	}

	/* Poll remaining threads */
	TODO (optimize-able?)
	while (thpool_p->num_threads_alive){
		error_check (bsem_post_all(thpool_p->jobqueue.has_jobs) != 0)
			return -4;
		sleep(1);
	}

	/* Job queue cleanup */
	error_check (jobqueue_destroy(&thpool_p->jobqueue) != 0) return -5;
	/* Deallocs */
	for (n=0; n < threads_total; n++){
		thread_destroy(thpool_p->threads[n]);
	}
	free(thpool_p->threads);
	free(thpool_p);
	return 0;
}


/* Pause all threads in threadpool */
int thpool_pause(thpool_* thpool_p) {
	int n;
	for (n=0; n < thpool_p->num_threads_alive; n++){
		error_check (pthread_kill(
			thpool_p->threads[n]->pthread, SIGUSR1) != 0)
			return -1;
	}
	return 0;
}


/* Resume all threads in threadpool */
void thpool_resume(thpool_* thpool_p) {
    /* // resuming a single threadpool hasn't been
    // implemented yet, meanwhile this supresses
    // the warnings */
    (void)thpool_p;

	threads_on_hold = 0;
}


int thpool_num_threads_working(thpool_* thpool_p){
	return thpool_p->num_threads_working;
}





/* ============================ THREAD ============================== */


/* Initialize a thread in the thread pool
 *
 * @param thread        address to the pointer of the thread to be created
 * @param id            id to be given to the thread
 * @return 0 on success, -1 otherwise.
 */
static int thread_init (
	thpool_* thpool_p,
	struct thread** thread_p,
	int id){

	*thread_p = (struct thread*)malloc(sizeof(struct thread));
	error_check (thread_p == NULL){
		err("thread_init(): Could not allocate memory for thread\n");
		return -1;
	}

	(*thread_p)->thpool_p = thpool_p;
	(*thread_p)->id       = id;

	error_check (pthread_create(
		&(*thread_p)->pthread, NULL,
		(void *(*) (void *))thread_do, (*thread_p)) != 0)
		return -2;
	error_check (pthread_detach((*thread_p)->pthread) != 0)
		return -3;
	return 0;
}


/* Sets the calling thread on hold */
static void thread_hold(int sig_id) {
    (void)sig_id;
	threads_on_hold = 1;
	TODO (looks busy or optimize-able)
	while (threads_on_hold){
		sleep(1);
	}
}


/* What each thread is doing
*
* In principle this is an endless loop. The only time this loop gets interuppted is once
* thpool_destroy() is invoked or the program exits.
*
* @param  thread        thread that will run this function
* @return nothing
*/
static void* thread_do(struct thread* thread_p){

	/* Set thread name for profiling and debuging */
	char thread_name[128] = {0};
	int err = snprintf(thread_name, sizeof (thread_name) - 1, "thread-pool-%d", thread_p->id);
	thpool_* thpool_p;
	struct sigaction act;
	error_check (err < 0 || (unsigned int) err >= sizeof (thread_name))
		return NULL;

#if defined(__linux__)
	/* Use prctl instead to prevent using _GNU_SOURCE flag and implicit declaration */
	prctl(PR_SET_NAME, thread_name);
#elif defined(__APPLE__) && defined(__MACH__)
	pthread_setname_np(thread_name);
#else
	err("thread_do(): pthread_setname_np is not supported on this system");
#endif

	/* Assure all threads have been created before starting serving */
	thpool_p = thread_p->thpool_p;

	/* Register signal handler */
	error_check (sigemptyset(&act.sa_mask) != 0) return NULL;
	act.sa_flags = 0;
	act.sa_handler = thread_hold;
	TODO (de-optimize non-critical error?)
	error_check (sigaction(SIGUSR1, &act, NULL) == -1) {
		err("thread_do(): cannot handle SIGUSR1");
	}

	/* Mark thread as alive (initialized) */
	error_check (pthread_mutex_lock(&thpool_p->thcount_lock) != 0) return NULL;
	thpool_p->num_threads_alive += 1;
	error_check (pthread_mutex_unlock(&thpool_p->thcount_lock) != 0) return NULL;

	TODO (optimize-able?)
	while(threads_keepalive){

		error_check (bsem_wait(thpool_p->jobqueue.has_jobs) != 0) return NULL;

		if (threads_keepalive){
			void (*func_buff)(void*);
			void*  arg_buff;
			job* job_p;

			error_check (pthread_mutex_lock(&thpool_p->thcount_lock) != 0) return NULL;
			thpool_p->num_threads_working++;
			error_check (pthread_mutex_unlock(&thpool_p->thcount_lock) != 0) return NULL;

			/* Read job from queue and execute it */
			job_p = jobqueue_pull(&thpool_p->jobqueue);
			if (job_p) {
				func_buff = job_p->function;
				arg_buff  = job_p->arg;
				func_buff(arg_buff);
				free(job_p);
			}

			error_check (pthread_mutex_lock(&thpool_p->thcount_lock) != 0) return NULL;
			thpool_p->num_threads_working--;
			if (!thpool_p->num_threads_working) {
				error_check (pthread_cond_signal(&thpool_p->threads_all_idle) != 0) return NULL;
			}
			error_check (pthread_mutex_unlock(&thpool_p->thcount_lock) != 0) return NULL;
		}
	}
	error_check (pthread_mutex_lock(&thpool_p->thcount_lock) != 0) return NULL;
	thpool_p->num_threads_alive --;
	error_check (pthread_mutex_unlock(&thpool_p->thcount_lock) != 0) return NULL;

	return NULL;
}


/* Frees a thread  */
static void thread_destroy (thread* thread_p){
	free(thread_p);
}





/* ============================ JOB QUEUE =========================== */


/* Initialize queue */
static int jobqueue_init(jobqueue* jobqueue_p){
	jobqueue_p->len = 0;
	jobqueue_p->front = NULL;
	jobqueue_p->rear  = NULL;

	jobqueue_p->has_jobs = (struct bsem*)malloc(sizeof(struct bsem));
	error_check (jobqueue_p->has_jobs == NULL){
		return -1;
	}

	error_check (pthread_mutex_init(&(jobqueue_p->rwmutex), NULL) != 0) return -2;
	error_check (bsem_init(jobqueue_p->has_jobs, 0) != 0) return -3;

	return 0;
}


/* Clear the queue */
static int jobqueue_clear(jobqueue* jobqueue_p){
	while(jobqueue_p->len){
		struct job* j = jobqueue_pull(jobqueue_p);
		error_check (j == NULL) return -1;
		free(j);
	}

	jobqueue_p->front = NULL;
	jobqueue_p->rear  = NULL;
	error_check (bsem_reset(jobqueue_p->has_jobs) != 0) return -2;
	jobqueue_p->len = 0;
	return 0;
}


/* Add (allocated) job to queue
 */
static int jobqueue_push(jobqueue* jobqueue_p, struct job* newjob){

	error_check (pthread_mutex_lock(&jobqueue_p->rwmutex)) return -1;
	newjob->prev = NULL;

	switch(jobqueue_p->len){

		case 0:  /* if no jobs in queue */
					jobqueue_p->front = newjob;
					jobqueue_p->rear  = newjob;
					break;

		default: /* if jobs in queue */
					jobqueue_p->rear->prev = newjob;
					jobqueue_p->rear = newjob;

	}
	jobqueue_p->len++;

	error_check (bsem_post(jobqueue_p->has_jobs) != 0) return -2;
	error_check (pthread_mutex_unlock(&jobqueue_p->rwmutex) != 0) return -3;
	return 0;
}


/* Get first job from queue(removes it from queue)
 *
 * Notice: Caller MUST hold a mutex
 */
static struct job* jobqueue_pull(jobqueue* jobqueue_p){
	job* job_p;

	error_check (pthread_mutex_lock(&jobqueue_p->rwmutex) != 0) return NULL;
	job_p = jobqueue_p->front;

	switch(jobqueue_p->len){

		case 0:  /* if no jobs in queue */
		  			break;

		case 1:  /* if one job in queue */
					jobqueue_p->front = NULL;
					jobqueue_p->rear  = NULL;
					jobqueue_p->len = 0;
					break;

		default: /* if >1 jobs in queue */
					jobqueue_p->front = job_p->prev;
					jobqueue_p->len--;
					/* more than one job in queue -> post it */
					error_check (bsem_post(jobqueue_p->has_jobs) != 0) return NULL;

	}

	error_check (pthread_mutex_unlock(&jobqueue_p->rwmutex) != 0) return NULL;
	return job_p;
}


/* Free all queue resources back to the system */
static int jobqueue_destroy(jobqueue* jobqueue_p){
	error_check (jobqueue_clear(jobqueue_p) != 0) return -1;
	free(jobqueue_p->has_jobs);
	return 0;
}





/* ======================== SYNCHRONISATION ========================= */


/* Init semaphore to 1 or 0 */
static int bsem_init(bsem *bsem_p, int value) {
	error_check (value < 0 || value > 1) {
		err("bsem_init(): Binary semaphore can take only values 1 or 0");
		/*exit(1);*/
		return -1;
	}
	/*if (pthread_mutex_init(&(bsem_p->mutex), NULL) != 0) return -2;*/
	bsem_p->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
	/*if (pthread_cond_init(&(bsem_p->cond), NULL) != 0) {
		pthread_mutex_destroy (&(bsem_p->mutex));
		return -3;
	}*/
	bsem_p->cond = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
	bsem_p->v = value;
	return 0;
}


/* Reset semaphore to 0 */
static int bsem_reset(bsem *bsem_p) {
	return bsem_init(bsem_p, 0);
}


/* Post to at least one thread */
static int bsem_post(bsem *bsem_p) {
	error_check (pthread_mutex_lock(&bsem_p->mutex) != 0) return -1;
	bsem_p->v = 1;
	error_check (pthread_cond_signal(&bsem_p->cond) != 0) return -2;
	error_check (pthread_mutex_unlock(&bsem_p->mutex) != 0) return -3;
	return 0;
}


/* Post to all threads */
static int bsem_post_all(bsem *bsem_p) {
	error_check (pthread_mutex_lock(&bsem_p->mutex) != 0) return -1;
	bsem_p->v = 1;
	error_check (pthread_cond_broadcast(&bsem_p->cond) != 0) return -2;
	error_check (pthread_mutex_unlock(&bsem_p->mutex) != 0) return -3;
	return 0;
}


/* Wait on semaphore until semaphore has value 0 */
static int bsem_wait(bsem* bsem_p) {
	error_check (pthread_mutex_lock(&bsem_p->mutex) != 0) return -1;
	TODO (looks busy or optimize-able)
	while (bsem_p->v != 1) {
		error_check (pthread_cond_wait(&bsem_p->cond, &bsem_p->mutex) != 0) return -2;
	}
	bsem_p->v = 0;
	error_check (pthread_mutex_unlock(&bsem_p->mutex) != 0) return -3;
	return 0;
}
