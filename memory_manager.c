#include "memory_manager.h"
#include "my_pthread_t.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static char physical_memory[8388608]; /* 8 MB to simulate physical memory */
int FIRST_ALLOCATE;
int PAGE_SIZE;
int NUMBER_PAGES;

/*
* Gets memory metadata.
*/
static void init_memory_metadata(int size, char *FILE, int *LINE, REQID threadreq) {
        /* Set memory metadata. */
        PAGE_SIZE = (int) sysconf(_SC_PAGE_SIZE);
        NUMBER_PAGES = 8388608 / page_size;

        struct page;
        if (threadreq = LIBRARYREQ)
                page.tid = NULL;
        else
                page.tid = NULL; // Call function in my_pthread_t.h to get this.
        page.start_address = physical_memory;
        page.end_address = physical_memory + (PAGE_SIZE - 1);
        page.next = NULL;
}

/*
* Returns a void pointer to allocated memory.
* Finds the page associated with the calling thread and allocated memory from it.
* If there is not enough memory in the page to allocate from, return a NULL pointer.
* Should communicate with scheduler to know which thread made the request so it knows which page to allocate from.
*/
void *my_allocate(int size, char *FILE, int *LINE, REQID threadreq) {
        if (FIRST_ALLOCATE == 1) {
                init_memory_metadata();
                FIRST_ALLOCATE = 0;
        }
        return NULL;
}

/*
* Finds the page associated with the calling thread and frees allocated memory from it.
* Should communicate with scheduler to know which thread made the request so it knows which page to free the allocation from.
*/
void my_deallocate(void *ptr, char *FILE, int *LINE, REQID threadreq) {

}
