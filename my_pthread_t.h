// name:
// username of iLab:
// iLab Server:
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

/* include lib header files that you need here: */
#include <ucontext.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>
#include <string.h>
#include <sys/ucontext.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <malloc.h>


#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)

//#define free(x) my_deallocate(x, __FILE__, __LINE__, THREADREQ)


typedef uint my_pthread_t;
typedef enum {THREADREQ, LIBRARYREQ} REQUEST_ID;

enum BLOCK_STATE {
        UNALLOCATED,
        ALLOCATED
};

enum PAGE_STATE {
        ASSIGNED,
        UNASSIGNED
};



typedef struct page {
        enum PAGE_STATE state;
         REQUEST_ID request_id;
        my_pthread_t tid;
        int size_of_allocated;
        char *start_address;
        char *end_address;
        char *block_list_head;
        char *n2;

        int frame;
        int num_of_pages;
        struct page *next;
        struct page *front;
        struct block *block_head;
        int size;
} *page_ptr;






typedef struct block {
	int size;
	enum BLOCK_STATE state;
	struct block * next;
	struct block * prev;
}*block_ptr;









typedef enum thread_state {
        RUNNING, // Currently running.
        READY, // Scheduled (in multi-level priority queue), in line to be run.
        WAITING, // Waiting in wait queue to aquire lock to critical section.
        YIELD, // Yield thread
        TERMINATED // Thread finished running or was terminated early.
} thread_state;

typedef struct threadControlBlock {
	/* add something here */
        my_pthread_t tid;
        ucontext_t context;
        enum thread_state state;
        int priority;
        struct timeval initial_start_time;
	struct timeval recent_start_time;
        struct timeval last_yield_time;
        void **return_value;
        struct threadControlBlock *next_tcb;
        int joined;

       struct page *head;
       int last_frame;
} tcb;







typedef struct queue {
	tcb *head;
	tcb *tail;
	int size;
} queue;
/* mutex struct definition */
typedef struct my_pthread_mutex_t {
	/* add something here */
        my_pthread_t lock_owner;        // Thread owning the mutex
        queue *lock_wait_queue;
        int state; // 1 locked, 0 unlocked
} my_pthread_mutex_t;

/* define your data structures here: */

// Feel free to add your own auxiliary data structures



typedef struct {
        queue *multi_level_priority_queue;
        queue *lock_wait_queues;
        tcb *current_tcb;
        tcb *main_tcb; // Context that calls my_pthread_create function
        int *priority_time_slices;
} scheduler;


#define USE_MY_PTHREAD 1

 //(comment it if you want to use real pthread)

#ifdef USE_MY_PTHREAD
#define pthread_t my_pthread_t
#define pthread_mutex_t my_pthread_mutex_t
#define pthread_create my_pthread_create
#define pthread_exit my_pthread_exit
#define pthread_join my_pthread_join
#define pthread_mutex_init my_pthread_mutex_init
#define pthread_mutex_lock my_pthread_mutex_lock
#define pthread_mutex_unlock my_pthread_mutex_unlock
#define pthread_mutex_destroy my_pthread_mutex_destroy
#endif


/* Function Declarations: */
tcb	*get_tcb();

void signal_handler();
/* Queue Functions */
void queue_init(queue *q);

void enqueue(queue *q, tcb *tcb_node);

tcb *dequeue(queue *q);

tcb *peek(queue * q);

void print_multi_level_priority_queue();

void print_lock_queue(queue *q);

void my_pthread_yield_after_unlock_helper();

/* Get current time */
struct timeval current_time();

/* Scheduler functions */
void init_scheduler();

void scheduler_maintenance();

/* Delete tcb from multi_level_priority_queue */
//void delete_tcb(my_pthread_t tid);

/* Helper for yield */
void my_pthread_yield_helper();

/* create a new thread */
int my_pthread_create(my_pthread_t *thread, pthread_attr_t *attr, void *(*function)(void*), void *arg);

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();

/* terminate a thread */
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr);

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

/* Get tid of current thread */
my_pthread_t get_current_tid();

tcb* get_tcb2();
void * allocate_lib(int x);
void * myallocate(int x, char * file, int line, REQUEST_ID request_id);
void mydeallocate(void * x, char * file, int line, REQUEST_ID request_id);
void seg_handler(int sig, siginfo_t *si, void *unused);
void swap_pages(int wFrame, int rFrame);
int count_mem();
void lock_all_mem();
void *mymalloc(int size_needed, void* memory, int allocated);





#endif

