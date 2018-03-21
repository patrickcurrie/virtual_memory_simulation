#include "memory_manager.h"
#include "my_pthread_t.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static char PHYSICAL_MEMORY[8388608]; /* 8 MB to simulate physical memory */
int FIRST_ALLOCATE;
int PAGE_SIZE;
int NUMBER_PAGES;
struct page_list PAGE_LIST;


/*
* Gets memory metadata.
*/
static void init_memory_metadata(int size, char *FILE, int *LINE, REQID threadreq) {
        /* Set memory metadata. */
        PAGE_SIZE = (int) sysconf(_SC_PAGE_SIZE);
        NUMBER_PAGES = 8388608 / PAGE_SIZE;

        struct page pg;
        if (threadreq = LIBRARYREQ)
                pg.tid = NULL;
        else
                pg.tid = get_current_tid();
        pg.start_address = PHYSICAL_MEMORY;
        pg.end_address = PHYSICAL_MEMORY + (PAGE_SIZE - 1);
        pg.block_list_head = NULL;
        pg.next = NULL;
        memcpy(PHYSICAL_MEMORY, &pg, sizof(pg));//undefined ref to sizeof
        PAGE_LIST.head = PHYSICAL_MEMORY;
}


/*
* Returns a void pointer to allocated memory.
* Finds the page associated with the calling thread and allocated memory from it.
* If there is not enough memory in the page to allocate from, return a NULL pointer.
* Should communicate with scheduler to know which thread made the request so it knows which page to allocate from.
*/
void *my_allocate(int size, char *FILE, int *LINE, REQID threadreq) {
        if (FIRST_ALLOCATE == 1) {
              // init_memory_metadata(); - argument passing
                FIRST_ALLOCATE = 0;
        }
        struct page *tmp = PAGE_LIST.head;
        my_pthread_t curr_tid = get_current_tid();
        while (tmp != NULL) { /* Locate page with matching tid. */
                if (tmp->tid == curr_tid) { /* This is the threads page */
                        /* Call allocate_for_thread(...) here. */
                }
        }

        return NULL;
}


/*
* Finds the page associated with the calling thread and frees allocated memory from it.
* Should communicate with scheduler to know which thread made the request so it knows which page to free the allocation from.
*/
void my_deallocate(void *ptr, char *FILE, int *LINE, REQID threadreq) {

}


int main(int argc, char **argv){
printf("test\n");
return 1;
}
