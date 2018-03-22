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
* Sets memory metadata and initial page metadata.
* Everything in here needs to be associated with a block.
* Two blocks need to be created, one to point to this metadata, the other to point to the rest.
*/
static void init_memory_metadata(REQUEST_ID request_id) {
        /* Set memory metadata. */
        PAGE_SIZE = (int) sysconf(_SC_PAGE_SIZE);
        NUMBER_PAGES = 8388608 / PAGE_SIZE;
        struct page pg;
        if (request_id = LIBRARYREQ)
                pg.tid = NULL;
        else
                pg.tid = get_current_tid();
        pg.size_of_allocated = 0; /* A block needs to be allocated for this metadata */
        pg.start_address = PHYSICAL_MEMORY;
        pg.end_address = PHYSICAL_MEMORY + (PAGE_SIZE - 1);
        pg.block_list_head = NULL;
        pg.next = NULL;
        memcpy(PHYSICAL_MEMORY, &pg, sizeof(pg));//undefined ref to sizeof
        PAGE_LIST.head = PHYSICAL_MEMORY;
}

/* Given a page and a size, allocates an appropriate block of memory within that page. */
static void *allocate_block(int size, struct page *current_page) {
        struct block *tmp_curr = current_page->block_list_head;
        struct block *tmp_prev;
        while (tmp_curr != NULL) {
                /* Find a suitable block and allocate it. */
                if (tmp_curr->state == UNALLOCATED && tmp_curr->size >= size) {
                        /*
                        * Split block into block of given size and size of rest.
                        * Set the addresses that the the two blocks point to.
                        * Store the two new blocks in metadata region of memory.
                        * Adjust the page's block list to account for the two new blocks.
                        */
                        break;
                }
                tmp_prev = tmp_curr;
                tmp_curr = tmp_curr->next;
        }

        return NULL;
}

/* Allocates memory for the scheduler. */
static void allocate_for_scheduler(int size) {
        struct page *tmp = PAGE_LIST.head;
        my_pthread_t curr_tid = get_current_tid();
        while (tmp != NULL) { /* Locate correct page for scheduler. */
                if (tmp->tid == NULL) { /* tid is NULL for scheduler pages. */
                        /* Is the page full? */
                        if (tmp->size_of_allocated >= PAGE_SIZE) {
                                tmp = tmp->next;
                                continue;
                        }
                        allocate_block(size, tmp);
                }
                tmp = tmp->next;
        }
}

/* Allocates memory for a thread. */
static void allocate_for_thread(int size) {
        struct page *tmp = PAGE_LIST.head;
        my_pthread_t curr_tid = get_current_tid();
        while (tmp != NULL) { /* Locate page with matching tid. */
                if (tmp->tid == curr_tid) { /* This is the threads page. */
                        /* Is the page full? */
                        if (tmp->size_of_allocated + size > PAGE_SIZE) {
                                tmp = tmp->next;
                                continue;
                        }
                        allocate_block(size, tmp);
                }
                tmp = tmp->next;
        }
}

/*
* Returns a void pointer to allocated memory.
* Finds the page associated with the calling thread and allocated memory from it.
* If there is not enough memory in the page to allocate from, return a NULL pointer.
* Should communicate with scheduler to know which thread made the request so it knows which page to allocate from.
*/
void *my_allocate(int size, char *FILE, int *LINE, REQUEST_ID request_id) {
        if (FIRST_ALLOCATE == 1) {
                init_memory_metadata(request_id);
                FIRST_ALLOCATE = 0;
        } else if (request_id == LIBRARYREQ) {
                allocate_for_scheduler(size);
        } else if (request_id == THREADREQ) {
                allocate_for_thread(size);
        } else {
                exit(-1); /* Unknown value for request_id given. */
        }
        return NULL;
}


/*
* Finds the page associated with the calling thread and frees allocated memory from it.
* Should communicate with scheduler to know which thread made the request so it knows which page to free the allocation from.
*/
void my_deallocate(void *ptr, char *FILE, int *LINE, REQUEST_ID request_id) {

}
