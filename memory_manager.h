

#ifndef MEMORY_MANAGER_H_
#define MEMORY_MANAGER_H_
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ucontext.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <malloc.h>
#include "my_pthread_t.h"








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


#define malloc(x) my_allocate(x, __FILE__, __LINE__, THREADREQ)

#define free(x) my_deallocate(x, __FILE__, __LINE__, THREADREQ)

void * myallocate(int x, char * file, int line, REQUEST_ID request_id);

void my_deallocate(void *ptr, char *FILE, int LINE, REQUEST_ID request_id);

void seg_handler(int sig, siginfo_t *si, void *unused);
void *mymalloc(int size_needed, void* memory, int allocated);

#endif /* MEMORY_MANAGER_H_ */

