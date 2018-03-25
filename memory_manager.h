#include <stdlib.h>
#include "my_pthread_t.h"

enum REQUEST_ID {
        LIBRARYREQ,
        THREADREQ
};

enum BLOCK_STATE {
        UNALLOCATED,
        ALLOCATED
};

enum PAGE_STATE {
        ASSIGNED,
        UNASSIGNED
}

struct block {
        enum BLOCK_STATE state;
        int data_size;
        int total_size;
        char *block_address;
        char *data_address;
        char *next;
};

struct page {
        enum PAGE_STATE state;
        enum REQUEST_ID request_id;
        my_pthread_t tid;
        int size_of_allocated;
        char *start_address;
        char *end_address;
        char *block_list_head;
        char *next;
        int frame;
      struct page *n2;
      struct  page *front;
      int num_of_pages;
};

struct memory_metadata {
        int memory_metadata_init;
        int page_size;
        int number_pages;
        char *address;
        char *page_list_head;
};

// When user calls malloc or free from a thread.
#define malloc(x) my_allocate(x, __FILE__, __LINE__, REQUEST_ID)

#define free(x) my_deallocate(x, __FILE__, __LINE__, REQUEST_ID)

void *my_allocate(int size, char *FILE, int *LINE, REQUEST_ID request_id);

void my_deallocate(void *ptr, char *FILE, int *LINE, REQUEST_ID request_id);

