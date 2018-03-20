#include <stdlib.h>
#include "my_pthread_t.h"

enum REQID {
        LIBRARYREQ,
        THREADREQ
};

struct page {
        my_pthread_t tid;
        char *start_address;
        char *end_address;
        struct page *next;
}

struct block {
        size_t size;
        char *data;
        struct block *next;
}

// When user calls malloc or free from a thread.
#define malloc(x) my_allocate(x, __FILE__, __LINE__, THREADREQ)

#define free(x) my_deallocate(x, __FILE__, __LINE__, THREADREQ)

void *my_allocate(int size, char *FILE, int *LINE, REQID threadreq);

void my_deallocate(void *ptr, char *FILE, int *LINE, REQID threadreq);
