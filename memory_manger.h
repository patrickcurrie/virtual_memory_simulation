#include <stdlib.h>
#include "my_pthread_t.h"

enum REQUEST_ID {
        LIBRARYREQ,
        THREADREQ
};

struct block {
        int size;
        char *data;
        struct block *next;
}

struct page {
        my_pthread_t tid;
        int size_of_allocated;
        char *start_address;
        char *end_address;
        struct block *block_list_head;
        struct page *next;
}

struct page_list {
        struct page *head;
}

// When user calls malloc or free from a thread.
#define malloc(x) my_allocate(x, __FILE__, __LINE__, REQUEST_ID)

#define free(x) my_deallocate(x, __FILE__, __LINE__, REQUEST_ID)

void *my_allocate(int size, char *FILE, int *LINE, REQUEST_ID request_id);

void my_deallocate(void *ptr, char *FILE, int *LINE, REQUEST_ID request_id);
