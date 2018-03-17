// name:
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"

#define CEILING_POS(X) ((X-(int)(X)) > 0 ? (int)(X+1) : (int)(X))
#define CEILING_NEG(X) ((X-(int)(X)) < 0 ? (int)(X-1) : (int)(X))
#define CEILING(X) ( ((X) > 0) ? CEILING_POS(X) : CEILING_NEG(X) )

/* Globals */
scheduler *SCHEDULER;
int SCHEDULER_INIT = 0;
int NUMBER_LEVELS = 3;
int NUMBER_LOCKS = 0;
int TIME_SLICE = 5000;
//int TIME_SLICE = 500000;
int HAS_RUN=0;
int START = 0;
int CYCLE = 0;
int STACK_SIZE = 8192; // 8192 kbytes is default stack size for CentOS
//int STACK_SIZE = 163840;
int TID = 0;
int first_thread = 0;

int LOCKED = 1;
int UNLOCKED = 0;
int AFTER_UNLOCK_PRIORITY = -1;

/* Static internal functions */

/*
Handles argument passing to the function run by a thread, preperation for the
thread to run, and cleanup after the thread finishes.
*/
static void thread_function_wrapper(tcb *tcb_node, void *(*function) (void *), void *arg) {
        /*
        printf("\n--\nFunction wrapper entered... tid: %d\nstate: RUNNING\n--\n", tcb_node->tid);
        printf("tid %d...\n", tcb_node->tid);
        */
	tcb_node->state = RUNNING;
	tcb_node->initial_start_time = current_time();
        if (*tcb_node->return_value == NULL) {
                //printf("Return is null.\n");
        }
        void **tmp_return = malloc(sizeof(void *));
        //printf("tmp_return addr before execute:%p\n", *tmp_return);
        *tmp_return = (*function)(arg);
        //printf("tmp_return addr after execute:%p\n", *tmp_return);
	//*tcb_node->return_value = (*function)(arg);
        if (*tmp_return == 0x0) { //nothing was returned
                //printf("Return is 0x0 after function.\n");
        }
        if (tcb_node->state == TERMINATED) {
                //printf("Exit worked, state is terminated.\n");
                pause();
        } else {
                //printf("Exit did not work, state is not terminated.\n");
        }
        *tcb_node->return_value = *tmp_return;
	tcb_node->state = TERMINATED;
        //printf("In wrapper return value for tid %d is %d\n", tcb_node->tid, *(int *) *tcb_node->return_value);
        //printf("In wrapper SCHED return value for tid %d is %d\n", tcb_node->tid, *(int *) *SCHEDULER->current_tcb->return_value);
        //setcontext(&(SCHEDULER->current_tcb->context));
        //printf("\n\n--\ntid: %d\nstate: TERMINATED--\n\n", tcb_node->tid);
        pause();
}

/*
Enqueue's a tcb to a given level of the multi-level priority queue. Sets
the state of the thread tcb to READY.
*/
static int schedule_thread(tcb * tcb_node, int priority) {
	if (priority < 0 || priority > NUMBER_LEVELS - 1) {
		return -1; // Error invalid priority.
	}
	//tcb_node->state = READY;
	tcb_node->priority = priority;
        //printf("\n\n--\nIn schedule thread, just before enqueue...\n--\n\n");
	enqueue(&(SCHEDULER->multi_level_priority_queue[priority]), tcb_node);
        //printf("tid %d added to schedule.\n", tcb_node->tid);
	return 0;
}

/*
Set tcb pointer to thread with supplied tid.
*/
static tcb *get_tcb(my_pthread_t tid) {
        sigset_t signal_mask;
        sigemptyset(&signal_mask);
        sigaddset(&signal_mask, SIGALRM);
        sigprocmask(SIG_BLOCK, &signal_mask, NULL);
        int i;
        int stop = 0;
        //int size = SCHEDULER->multi_level_priority_queue[i].size;
        tcb *tcb_node;
        for (i = 0; i < NUMBER_LEVELS && !stop; i++) {
                //printf("\n* Level %d", i);
                tcb_node = SCHEDULER->multi_level_priority_queue[i].head;
                while (tcb_node != NULL) {
                        //printf("\n(L%d, %d)", i, tcb_node->tid);
                        if (tcb_node->tid == tid) {
                                //printf("\nFound it\n");
                                stop = 1;
                                break;
                        }
                        tcb_node = tcb_node->next_tcb;
                }
        }
        sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
        return tcb_node;
}

/* Removes the tcb with the given tid, and returns a pointer to that tcb. */
static tcb *get_and_remove_tcb(my_pthread_t tid) {
        sigset_t signal_mask;
        sigemptyset(&signal_mask);
        sigaddset(&signal_mask, SIGALRM);
        sigprocmask(SIG_BLOCK, &signal_mask, NULL);
        int i;
        int stop = 0;
        tcb *tcb_node;
        tcb *tcb_node_prev;
        for (i = 0; i < NUMBER_LEVELS && !stop; i++) {
                //printf("\n* Level %d", i);
                tcb_node = SCHEDULER->multi_level_priority_queue[i].head;
                tcb_node_prev = tcb_node;
                while (tcb_node != NULL) {
                        //printf("\n(L%d, %d)", i, tcb_node->tid);
                        if (tcb_node->tid == tid) {
                                //printf("\nFound it\n");
                                if (tcb_node == tcb_node_prev) { // tcb_node is head.
                                        SCHEDULER->multi_level_priority_queue[i].head = tcb_node->next_tcb;
                                } else if (tcb_node->next_tcb == NULL) { // tcb_node is tail.
                                        SCHEDULER->multi_level_priority_queue[i].tail = tcb_node_prev;
                                        tcb_node_prev->next_tcb = NULL;
                                } else {
                                        tcb_node_prev->next_tcb = tcb_node->next_tcb;
                                }
                                stop = 1;
                                break;
                        }
                        tcb_node_prev = tcb_node;
                        tcb_node = tcb_node->next_tcb;
                }
        }

        sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
        return tcb_node;
}

/* Delete tcb from multi level priority queue. */
static void delete_tcb(my_pthread_t tid) {
        sigset_t signal_mask;
        sigemptyset(&signal_mask);
        sigaddset(&signal_mask, SIGALRM);
        sigprocmask(SIG_BLOCK, &signal_mask, NULL);
        int i;
        int stop = 0;
        tcb *tcb_node;
        tcb *tcb_node_prev;
        for (i = 0; i < NUMBER_LEVELS && !stop; i++) {
                //printf("\n* Level %d", i);
                tcb_node = SCHEDULER->multi_level_priority_queue[i].head;
                tcb_node_prev = tcb_node;
                while (tcb_node != NULL) {
                        //printf("\n(L%d, %d)", i, tcb_node->tid);
                        if (tcb_node->tid == tid) {
                                //printf("\nFound it\n");
                                if (tcb_node == tcb_node_prev) { // tcb_node is head.
                                        SCHEDULER->multi_level_priority_queue[i].head = tcb_node->next_tcb;
                                } else if (tcb_node->next_tcb == NULL) { // tcb_node is tail.
                                        SCHEDULER->multi_level_priority_queue[i].tail = tcb_node_prev;
                                        tcb_node_prev->next_tcb = NULL;
                                } else {
                                        tcb_node_prev->next_tcb = tcb_node->next_tcb;
                                }
                                stop = 1;
                                break;
                        }
                        tcb_node_prev = tcb_node;
                        tcb_node = tcb_node->next_tcb;
                }
        }

        /* Free tcb_node */
        //printf("\nFreeing tcb that was just joined...\n");
        free(tcb_node->return_value);
        //free(tcb_node->next_tcb);
        free(tcb_node);
        sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
}

/* Maintenance function operations and quick select algorithm implimentation */
static int get_num_scheduled_tcb() {
        int i;
        int count = 0;
        tcb *tcb_node;
        for (i = 0; i < NUMBER_LEVELS; i++) {
                tcb_node = SCHEDULER->multi_level_priority_queue[i].head;
                while (tcb_node != NULL) {
                        count++;
                        tcb_node = tcb_node->next_tcb;
                }
        }

        return count;
}

// Returnd tcb pointer must be freed.
static tcb *fold_mlpq_to_array() {
        int num_tcb = get_num_scheduled_tcb();
        tcb *tcb_array = malloc(sizeof(tcb) * num_tcb);
        tcb *tmp_tcb_array = tcb_array;
        int i;
        int tcb_count = 0;
        tcb *tcb_node;
        for (i = 0; i < NUMBER_LEVELS; i++) {
                tcb_node = SCHEDULER->multi_level_priority_queue[i].head;
                while (tcb_node != NULL) {
                        *tmp_tcb_array = *tcb_node;
                        tcb_node = tcb_node->next_tcb;
                        tmp_tcb_array++;
                }
        }

        return tcb_array;
}

static tcb quick_select(tcb *tcb_array, int len, int k) {
        int from = 0;
        int to = len - 1;
        while (from < to) {
                int r = from;
                int w = to;
                tcb mid = tcb_array[(r + w) / 2];
                while (r < w) {
                        if (timercmp(&(tcb_array[r].initial_start_time), &(mid.initial_start_time), >=) == 1) { // put the large values at the end
                                tcb tmp = tcb_array[w];
                                tcb_array[w] = tcb_array[r];
                                tcb_array[r] = tmp;
                                w--;
                        } else { // the value is smaller than the pivot, skip
                                r++;
                        }
                }

                // if we stepped up (r++) we need to step one down
                if (timercmp(&(tcb_array[r].initial_start_time), &(mid.initial_start_time), >) == 1) {
                        r--;
                }
                // the r pointer is on the end of the first k elements
                if (k <= r) {
                        to = r;
                } else {
                        from = r + 1;
                }
        }

        return tcb_array[k];
}

/* Promotes the given tid to the top priority level. */
static void change_priority(my_pthread_t tid, int priority) {
        tcb *tcb_node = get_and_remove_tcb(tid);
        schedule_thread(tcb_node, priority);
}

/* Queue Functions */

/* Initializes a queue */
void queue_init(queue * q) {
	q->head = NULL;
	q->tail = NULL;
	q->size = 0;
}

/*
If the queue is empty, than the head and tail will point to the same node after
enqueue. If it is not empty, then node will be inserted before the tail of the
queue, and become the new tail.
*/
void enqueue(queue * q, tcb * tcb_node) {
	if (q->size == 0) {
		q->head = tcb_node;
		q->tail = tcb_node;
        tcb_node->next_tcb = NULL;
		q->size++;
	} else {
		q->tail->next_tcb = tcb_node;
		q->tail = tcb_node;
        q->tail->next_tcb=NULL;
		q->size++;
	}
}

/*
If queue is empty, returns NULL. If queue has onlu 1 node, set tmp to what
head points to, then set head and tail to NULL and return tmp. Else, set tmp to
what head points to, and adjust head.
*/
tcb *dequeue(queue * q) {
	if (q->size == 0) {
		return NULL;
	}
	tcb * tmp;
	if (q->size == 1) {
		tmp = q->head;
		q->head = NULL;
		q->tail = NULL;
	} else {
		tmp = q->head;
		q->head = q->head->next_tcb;
	}
	tmp->next_tcb = NULL;
	q->size--;
	return tmp;
}

tcb *peek(queue * q) {
	return q->head;
}

/* Get current time */
struct timeval current_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv;
}

/* Initialize scheduler */
void init_scheduler() {
	SCHEDULER = malloc(sizeof(scheduler));
	SCHEDULER->multi_level_priority_queue = malloc(sizeof(queue) * NUMBER_LEVELS);
        int i;
	for (i = 0; i < NUMBER_LEVELS; i++) {
		queue_init(&(SCHEDULER->multi_level_priority_queue[i]));
	}

	SCHEDULER->main_tcb = NULL; // Set only after first call to my_pthread_create funtion.
	SCHEDULER->current_tcb = NULL; // New tcb malloced in my_pthread_create function.
	SCHEDULER->priority_time_slices = malloc(sizeof(int) * NUMBER_LEVELS);
	for (i = 0; i < NUMBER_LEVELS; i++) {
		SCHEDULER->priority_time_slices[i] = TIME_SLICE * (i + 1);
	}
}

void init_timer() {
        //setup signal
        struct itimerval timer;
        /*
        struct sigaction act, oact;
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGALRM);
        act.sa_handler = signal_handler;
        act.sa_flags = SA_SIGINFO;
        */
        //
        stack_t ss;
        ss.ss_size = SIGSTKSZ;
        ss.ss_sp = malloc(SIGSTKSZ);
        struct sigaction action;
        sigemptyset(&action.sa_mask);
        sigaddset(&action.sa_mask, SIGALRM);
        action.sa_handler = signal_handler;
        action.sa_flags = SA_ONSTACK | SA_SIGINFO;

        if (sigaltstack(&ss, NULL) == -1) {
               perror("sigaltstack");
               exit(EXIT_FAILURE);
           }

        //
        sigaction(SIGALRM, &action, NULL);
        timer.it_value.tv_sec = 0;
        timer.it_value.tv_usec = 25000;
        timer.it_interval.tv_sec = 0;
        timer.it_interval.tv_usec = 25000;
        setitimer(ITIMER_REAL, &timer, NULL); //(2)
}

void signal_handler(int signo, siginfo_t *info, void *context) {
        CYCLE++;
        if (CYCLE == 5) {
                CYCLE=0;
                //scheduler_maintenance();
                //print_multi_level_priority_queue();
        }
        // Did the thread pass it's allotted time slice?
        if (time_compare(SCHEDULER->current_tcb->recent_start_time,current_time(), SCHEDULER->priority_time_slices[SCHEDULER->current_tcb->priority]) != -1) {
                ucontext_t *return_context = (ucontext_t *) context; // Save this context.
                SCHEDULER->current_tcb->context = *return_context;
                if (AFTER_UNLOCK_PRIORITY != -1) {
                        my_pthread_yield_after_unlock_helper();
                } else {
                        my_pthread_yield_helper();
                }
        } else {
                /*
                // Without this it is basically round robin, everything stays on the first level.
                // Did not finish in allotted time. ***TEST***
                if (SCHEDULER->current_tcb->priority == NUMBER_LEVELS - 1) { // Is at bottom level, can't demote any lower.
                        my_pthread_yield_helper();
                }

                AFTER_UNLOCK_PRIORITY = SCHEDULER->current_tcb->priority;
                change_priority(SCHEDULER->current_tcb->tid, SCHEDULER->current_tcb->priority - 1);
                my_pthread_yield_after_unlock_helper();
                */



                /*
                if (SCHEDULER->current_tcb->priority == NUMBER_LEVELS - 1) { // Is at bottom level, can't demote any lower.
                        if (AFTER_UNLOCK_PRIORITY != -1) {
                                my_pthread_yield_after_unlock_helper();
                        } else {
                                my_pthread_yield_helper();
                        }
                        return;
                }
                change_priority(SCHEDULER->current_tcb->tid, SCHEDULER->current_tcb->priotiry - 1);
                AFTER_UNLOCK_PRIORITY = SCHEDULER->current_tcb->priotiry;
                */
        }
        return;
}

/* I hate this function. It's so ugly.
A helper function for maintain to compare time
return 1 if the gap between start and end is larger than gap
return 0 if the gap between start and end is the same as the gap
return -1 if the gap between start and end is smaller than the gap
PS: gap's unit is in microsecond*/
int time_compare(struct timeval start, struct timeval end, int gap){
    int gap_second = 0;
    int gap_microsecond = 0;
    int start_sec = start.tv_sec;
    int start_micro = start.tv_usec;
    int end_sec = end.tv_sec;
    int end_micro = end.tv_usec;
    if(gap>999999){
        gap_second = gap-(gap%1000000);
        gap_microsecond = gap%1000000;
    }else{
        gap_microsecond = gap;
    }
    if(end_sec-start_sec>gap_second){
        return 1;
    }else if(end_sec-start_sec==gap_second){
        if(end_micro-start_micro>gap_microsecond){
            return 1;
        }else if(end_micro-start_micro==gap_microsecond){
            return 0;
        }else{
            return -1;
        }
    }else{
        return -1;
    }
}

/*
Maintenance done on the multi-level priority queue to handle the SIGALRM signal.

Responsible for:
- Deleting threads with TERMINATED state from multi-level priotity queue (free tcb and adjust queue).
- Promoting and demoting threads in multi-level priority queue.
- Check if current running tcb (SCHEDULER->current_tcb) has used up its time slice, swap context and adjust accordingly if so.
*/
void scheduler_maintenance() {
        tcb *tcb_array = fold_mlpq_to_array();
        int num_scheduled = get_num_scheduled_tcb();
        tcb *tmp_tcb_array = malloc(sizeof(tcb) * num_scheduled);
        memcpy(tmp_tcb_array, tcb_array, num_scheduled * sizeof(tcb)); // Because quick_select modifies array.
        int oldest_tcb_array_size = CEILING(num_scheduled * 0.15);
        tcb *oldest_tcb_array = malloc(sizeof(tcb) * oldest_tcb_array_size);
        tcb oldest_tcb = quick_select(tmp_tcb_array, num_scheduled, oldest_tcb_array_size - 1);
        free(tmp_tcb_array);
        printf("\n\n--\nPrinting all initial times of all scheduled threads...\n");
        int i;
        for (i = 0; i < num_scheduled; i++) {
                if (tcb_array[i].initial_start_time.tv_sec == 0 && tcb_array[i].initial_start_time.tv_usec == 0) {
                        printf("tid %d: Has not run yet.\n", tcb_array[i].tid);
                        continue;
                }
                printf("tid %d: %ld.%06ld\n", tcb_array[i].tid, tcb_array[i].initial_start_time.tv_sec, tcb_array[i].initial_start_time.tv_usec);
        }
        printf("--\n\n");
        oldest_tcb_array[0] = oldest_tcb;
        int oldest_count = 1;
        for (i = 1; i < num_scheduled && oldest_count < oldest_tcb_array_size; i++) {
                if (timercmp(&(tcb_array[i].initial_start_time), &(oldest_tcb_array[0].initial_start_time), <=) == 1) {
                        //oldest_tcb_array[oldest_count++] = tcb_array[i];
                        oldest_tcb_array[oldest_count] = tcb_array[i];
                        //printf("i: %d, num_scheduled: %d, tcb_array[i].tid: %d, oldest_tcb_array[i].tid: %d\n\n", i, num_scheduled, tcb_array[i].tid, oldest_tcb_array[i].tid);
                        oldest_count++;
                }
        }

        printf("\n\n--\nOldest threads...\n\n");
        for (i = 0; i < oldest_tcb_array_size; i++) {
                printf("tid: %d\n", oldest_tcb_array[i].tid);
        }

        printf("--\n\n");
        printf("***Printing mlpq before priotiry change...****\n");
        print_multi_level_priority_queue();
        // --Test-
        for (i = 0; i < oldest_tcb_array_size; i++) {
                change_priority(oldest_tcb_array[i].tid, 0);
        }
        // ------
        printf("\n***Printing mlpq after priotiry change...****\n");
        print_multi_level_priority_queue();
        free(tcb_array);
        free(oldest_tcb_array);
}

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function) (void *), void *arg) {
	if (SCHEDULER_INIT == 0) { // Init scheduler if this is first time my_pthread_create is called.
                SCHEDULER_INIT = 1;
                init_scheduler();
	}
        // Create tcb for main context.
        // Main context is the current on if create is running for the first time.
        if (TID == 0) {
                tcb *tcb_main = malloc(sizeof(tcb));
        	tcb_main->tid = TID;
                tcb_main->return_value = malloc(sizeof(void *));
                if (getcontext(&(tcb_main->context)) != 0) {
                        return -1; // Error getthing context
                }
                // Configure main context stack.
                //printf("\nThread scheduled...\ntid %d\n", tcb_main->tid);
                tcb_main->state = READY;
                schedule_thread(tcb_main, 0);
                SCHEDULER->current_tcb = tcb_main;
        }

	// Create new tcb for thread.
	// Get current context.
	tcb *tcb_node = malloc(sizeof(tcb));
        TID++;
	tcb_node->tid = TID;//*thread;
        tcb_node->return_value = malloc(sizeof(void *));
        *thread = TID;
	if (getcontext(&(tcb_node->context)) != 0) {
		return -1; // Error getthing context
	}
	// Configure context stack.
	tcb_node->context.uc_link = 0;
	tcb_node->context.uc_stack.ss_sp = malloc(STACK_SIZE);
	tcb_node->context.uc_stack.ss_flags = 0;
	tcb_node->context.uc_stack.ss_size = STACK_SIZE;
	makecontext(&(tcb_node->context), (void *) thread_function_wrapper, 3, tcb_node, function, arg);
        //printf("\nThread scheduled...\ntid %d\n", tcb_node->tid);
        tcb_node->state = READY;
        schedule_thread(tcb_node, 0); // Adds to mlpq and sets state to READY.
        if(first_thread == 0) {
                first_thread = 1;
                init_timer();
        }
	return 0;
}

/* give CPU pocession to other user level threads voluntarily */
void my_pthread_yield_helper() {
	int current_priority = SCHEDULER->current_tcb->priority;
	tcb *tcb_node = dequeue(&(SCHEDULER->multi_level_priority_queue[current_priority]));
	if (tcb_node->state != TERMINATED) {
		tcb_node->state = READY;
	}
	schedule_thread(tcb_node, tcb_node->priority);
    	// Check to see if we need to move on to the next queue level.
    	if (HAS_RUN >= SCHEDULER->multi_level_priority_queue[current_priority].size) {
                HAS_RUN = 0; //running a new queue set the counter to 0
                int c = 0;
                if (current_priority != NUMBER_LEVELS - 1) {
                        c = current_priority + 1;
                }
                int i = 0;
                int final = NUMBER_LEVELS - 2;
                for (i = 0; i <= final; i++) {
                        SCHEDULER->current_tcb = peek(&(SCHEDULER->multi_level_priority_queue[c]));
                        if (SCHEDULER->current_tcb != NULL) {
                                break;
                        } else {
                                if (c == NUMBER_LEVELS - 1) {
                                        c = 0;
                                } else {
                                        c++;
                                }
                        }
                }
                // This if is unnecessary, just check if current priority is empty and run it if it isn't, return 0 otherwise
                if (SCHEDULER->current_tcb == NULL){
                        //look for the if there is still threads left
                        int i = 0;
                        for (i = 0; i < NUMBER_LEVELS; i++) {
                                if (SCHEDULER->multi_level_priority_queue[i].size > 0) {
                                        SCHEDULER->current_tcb = peek(&(SCHEDULER->multi_level_priority_queue[i]));
                                        break;
                                }
                        }

                        if (SCHEDULER->current_tcb == NULL) {
                                return;
                        }
                }
        } else {
                SCHEDULER->current_tcb = peek(&(SCHEDULER->multi_level_priority_queue[current_priority]));
        }
        if (SCHEDULER->current_tcb->state == YIELD) {
                SCHEDULER->current_tcb->state = READY;
                my_pthread_yield_helper();
                return;
        }
	if (SCHEDULER->current_tcb->state == TERMINATED) { // Don't run context if TERMINATED
                my_pthread_yield_helper();
                return;
	}
	SCHEDULER->current_tcb->state = RUNNING;
	tcb_node->last_yield_time = current_time();
        SCHEDULER->current_tcb->recent_start_time = current_time();
        HAS_RUN++;
        setcontext(&(SCHEDULER->current_tcb->context));
	return;
}

/* Special case for schedule yielding after the most recent thread unlocked a mutex, runs first thread instead of rescheduling it and running the next.*/
int my_pthread_yield_after_unlock_helper() {
        int current_priority = AFTER_UNLOCK_PRIORITY;
        AFTER_UNLOCK_PRIORITY = -1;
        if (HAS_RUN >= SCHEDULER->multi_level_priority_queue[current_priority].size) {
                HAS_RUN = 0; //running a new queue set the counter to 0
                int c = 0;
                if (current_priority != NUMBER_LEVELS - 1) {
                        c = current_priority + 1;
                }
                int i = 0;
                int final = NUMBER_LEVELS - 2;
                for (i = 0; i <= final; i++) {
                        SCHEDULER->current_tcb = peek(&(SCHEDULER->multi_level_priority_queue[c]));
                        if (SCHEDULER->current_tcb != NULL) {
                                break;
                        } else {
                                if (c == NUMBER_LEVELS - 1) {
                                        c = 0;
                                } else {
                                        c++;
                                }
                        }
                }
                // This if is unnecessary, just check if current priority is empty and run it if it isn't, return 0 otherwise
                if (SCHEDULER->current_tcb == NULL){
                        //look for the if there is still threads left
                        int i = 0;
                        for (i = 0; i < NUMBER_LEVELS; i++) {
                                if (SCHEDULER->multi_level_priority_queue[i].size > 0) {
                                        SCHEDULER->current_tcb = peek(&(SCHEDULER->multi_level_priority_queue[i]));
                                        break;
                                }
                        }

                        if (SCHEDULER->current_tcb == NULL) {
                                return;
                        }
                }
        } else {
                SCHEDULER->current_tcb = peek(&(SCHEDULER->multi_level_priority_queue[current_priority]));
        }
        if (SCHEDULER->current_tcb->state == YIELD) {
                SCHEDULER->current_tcb->state = READY;
                my_pthread_yield_helper();
                return;
        }
	if (SCHEDULER->current_tcb->state == TERMINATED) { // Don't run context if TERMINATED
                my_pthread_yield_helper();
                return;
	}
	SCHEDULER->current_tcb->state = RUNNING;
        SCHEDULER->current_tcb->recent_start_time = current_time();
        HAS_RUN++;
        setcontext(&(SCHEDULER->current_tcb->context));
	return;
}

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
        raise(SIGALRM);
        return 0;
}

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
        sigset_t signal_mask;
        sigemptyset(&signal_mask);
        sigaddset(&signal_mask, SIGALRM);
        sigprocmask(SIG_BLOCK, &signal_mask, NULL);
        *SCHEDULER->current_tcb->return_value = value_ptr;
        //printf("In exit return value for tid %d is %d\n", SCHEDULER->current_tcb->tid, *(int *) value_ptr);
        //printf("In exit return value for tid %d is %d\n", SCHEDULER->current_tcb->tid, *(int *) *SCHEDULER->current_tcb->return_value);
        SCHEDULER->current_tcb->state = TERMINATED;
        sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
        // Block schedule handler until thread to wait on is identified in multi_level_priority_queue.
        //print_multi_level_priority_queue();
        //printf("Thread to be joind: %d\n", thread);
        //printf("tid of main thread: %d\n", SCHEDULER->current_tcb->tid);
        tcb *tcb_node = get_tcb(thread);
        //printf("tid of fetch tcb in join: %d\n", tcb_node->tid);
        if (tcb_node == NULL) {
                //printf("Error, thread is not scheduled or in a wait queue.\n--\n");
                return -1;
        }
        while (1) {
                if (tcb_node->state == TERMINATED) {
                        break;
                }
        }

        //printf("Got through\n");
        if (value_ptr != NULL) {
                //printf("value_ptr does not equal NULL.\n");
                *value_ptr = *tcb_node->return_value;
        }
        /*
        printf("\n--\n\n");

        printf("\n\n--\n");
        printf("Printing mlpq...\n");
        print_multi_level_priority_queue();
        printf("Deleting tcb that was just joined...\n");
        delete_tcb(thread);
        printf("Printing mlpq after tcb was deleted...\n");
        print_multi_level_priority_queue();
        printf("--\n\n");
        */


        return 0;
}

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
        sigset_t signal_mask;
        sigemptyset(&signal_mask);
        sigaddset(&signal_mask, SIGALRM);
        sigprocmask(SIG_BLOCK, &signal_mask, NULL);
        if (SCHEDULER_INIT == 0) {
                init_scheduler();
        }
        ++NUMBER_LOCKS;
        mutex->lock_wait_queue = malloc(sizeof(queue));
        queue_init(mutex->lock_wait_queue);
        mutex->state = UNLOCKED;
        /*
        printf("\n\n--\nIn mutex init, mutex address is: %p\n\n", mutex);
        printf("In mutex init, mutex->lock_wait_queue address is: %p\n--\n\n", mutex->lock_wait_queue);
        */
        sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);

        return 0;
}

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
        /*
        while (__sync_lock_test_and_set(mutex->state, LOCKED)) {

        }
        */

        /*
        If lock is locked, add the thread to the wait queue.
        If lock is locked, add lock to wait queue.
        Set AFTER_LOCK = 1 --> Runs special after_lock_yield_helper_function().
        If a lock hits unlock, it takes the head thread on the wait queue and puts it back on the run queue, first level.
        */
        int current_priority = SCHEDULER->current_tcb->priority;
        tcb *tcb_node = NULL;
        while (1) {
                if (__sync_lock_test_and_set(&(mutex->state), LOCKED) == UNLOCKED) {
                        break;
                } else {
                        tcb_node = dequeue(&(SCHEDULER->multi_level_priority_queue[current_priority]));
                        enqueue(mutex->lock_wait_queue, tcb_node);
                        tcb_node->last_yield_time = current_time();
                        pause();
                }
        }

        return 0;
}

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
        __sync_lock_release(&(mutex->state));
        tcb *tcb_node = dequeue(mutex->lock_wait_queue);
        if (tcb_node != NULL) {
                schedule_thread(tcb_node, 0);
        }
        AFTER_UNLOCK_PRIORITY = SCHEDULER->current_tcb->priority;
        return 0;
}

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
        return 0;
}

void print_multi_level_priority_queue() {
        int i=0;
        int x=0;
        int isHead;
        printf("\n\n--\n");
        printf("Printing SCHEDULER->multi_level_priority_queue...\n");
        for(i=0; i < NUMBER_LEVELS; i++) {
                int size = SCHEDULER->multi_level_priority_queue[i].size;
                tcb* p = SCHEDULER->multi_level_priority_queue[i].head;
                isHead = 0;
                while(p!=NULL) {
                        if (isHead == 0 && p->next_tcb == NULL) {
                                isHead = 1;
                                if (p->state == TERMINATED) {
                                        printf("(Level:%d, Head and Tail, tid:%d, TERMINATED) ", i, p->tid);
                                } else if (p->state == READY) {
                                        printf("(Level:%d, Head and Tail, tid:%d, READY) ", i, p->tid);
                                } else if (p->state == RUNNING) {
                                        printf("(Level:%d, Head and Tail, tid:%d, RUNNING) ", i, p->tid);
                                } else {
                                        printf("(Level:%d, Head and Tail, tid:%d, UNKNOWN) ", i,  p->tid);
                                }
                                p = p->next_tcb;
                                continue;
                        } else if (isHead == 0) {
                                isHead = 1;
                                if (p->state == TERMINATED) {
                                        printf("(Level:%d, Head, tid:%d, TERMINATED) ", i, p->tid);
                                } else if (p->state == READY) {
                                        printf("(Level:%d, Head, tid:%d, READY) ", i, p->tid);
                                } else if (p->state == RUNNING) {
                                        printf("(Level:%d, Head, tid:%d, RUNNING) ", i, p->tid);
                                } else {
                                        printf("(Level:%d, Head, tid:%d, UNKNOWN) ", i, p->tid);
                                }
                                p = p->next_tcb;
                                continue;
                        } else if (p->next_tcb == NULL) {
                                if (p->state == TERMINATED) {
                                        printf("(Level:%d, Tail, tid:%d, TERMINATED) ", i, p->tid);
                                } else if (p->state == READY) {
                                        printf("(Level:%d, Tail, tid:%d, READY) ", i, p->tid);
                                } else if (p->state == RUNNING) {
                                        printf("(Level:%d, Tail, tid:%d, RUNNING) ", i, p->tid);
                                } else {
                                        printf("(Level:%d, Tail, tid:%d, UNKNOWN) ", i, p->tid);
                                }
                                p = p->next_tcb;
                                continue;
                        }


                        if (p->state == TERMINATED) {
                                printf("(Level:%d, tid:%d, TERMINATED) ", i, p->tid);
                        } else if (p->state == READY) {
                                printf("(Level:%d, tid:%d, READY) ", i, p->tid);
                        } else if (p->state == RUNNING) {
                                printf("(Level:%d, tid:%d, RUNNING) ", i, p->tid);
                        } else {
                                printf("(Level:%d, tid:%d, UNKNOWN) ", i, p->tid);
                        }
                        p = p->next_tcb;
                }
                printf("\n*End Level:%d*\n", i);
        }
        printf("\n--\n\n");
}


void print_lock_queue(queue *q) {
        int size = q->size;
        tcb* p = q->head;
        printf("\n--\n");
        while(p != NULL) {
                if (p->state == TERMINATED) {
                        printf("(tid:%d, TERMINATED) ", p->tid);
                } else if (p->state == READY) {
                        printf("(tid:%d, READY) ", p->tid);
                } else if (p->state == RUNNING) {
                        printf("(tid:%d, RUNNING) ", p->tid);
                } else {
                        printf("(tid:%d, UNKNOWN) ", p->tid);
                }
                p = p->next_tcb;
        }
                printf("\n--\n");
}
