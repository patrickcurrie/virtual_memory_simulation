#include <stdlib.h>
#include "my_pthread_t.h"



struct block {
        size_t size;
        char *data;
        struct block *next;
};

struct page {
        my_pthread_t tid;
        char *start_address;
        char *end_address;
        struct block *block_list_head;
        struct page *next;
};

struct page_list {
        struct page *head;
};



typedef enum {THREADREQ, LIBRARYREQ} REQID;
// When user calls malloc or free from a thread.

#define malloc(x) my_allocate(x, __FILE__, __LINE__, THREADREQ)

#define free(x) my_deallocate(x, __FILE__, __LINE__, THREADREQ)

void *my_allocate(int size, char *FILE, int *LINE, REQID threadreq);

void my_deallocate(void *ptr, char *FILE, int *LINE, REQID threadreq);
