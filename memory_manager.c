#include "memory_manager.h"
#include "my_pthread_t.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static char PHYSICAL_MEMORY[8388608]; /* 8 MB to simulate physical memory. */
PHYSICAL_MEMORY[0] = NULL; /* NULL if memory metadata is not initialized. */
struct memory_metadata *MEMORY_METADATA;

/* Allocates a new page in physical memory. */
static void assign_new_page() {
        /* Unallocated block of memory for the new page. */

        /*
        struct block unalloc_blk;
        unalloc_blk.block_address = pg.start_address + sizeof(struct page);
        unalloc_blk.data_address = unalloc_blk.block_address + sizeof(struct block);
        unalloc_blk.data_size = NULL;
        unalloc_blk.total_size = MEMORY_METADATA.page_size - sizeof(struct page);
        unalloc_blk.state = UNALLOCATED;
        */

}

/*
* Sets memory metadata and creates an unassigned page.
*/
static void init_memory_metadata(REQUEST_ID request_id) {
        /* Initialize memory metadata. */
        struct memory_metadata mem_meta;
        mem_meta.page_size = (int) sysconf(_SC_PAGE_SIZE);
        mem_meta.number_pages = 8388608 / mem_meta.page_size;
        mem_meta.address = PHYSICAL_MEMORY;
        mem_meta.page_list_head = NULL;
        memcpy(mem_meta.address, &mem_meta, sizeof(struct memory_metadata));
        MEMORY_METADATA = mem_meta.address;
        /*
        * Create the first unassigned page.
        * This will be split into assigned pages as new pages are added.
        */
        struct page pg;
        pg.request_id = NULL;
        pg.tid = NULL;
        pg.size_of_allocated = 0;
        pg.start_address = MEMORY_METADATA.address + sizeof(struct memory_metadata);
        pg.end_address = &PHYSICAL_MEMORY[sizeof(PHYSICAL_MEMORY) - 1];
        pg.block_list_head = NULL;
        pg.next = NULL;
        pg.state = UNASSIGNED;
        memcpy(pg.start_address, &pg, sizeof(struct page));
        MEMORY_METADATA.page_list_head = pg.start_address;
}

/*
* Given a page and a size, allocates an appropriate block of memory within that page,
* Stores block metadata in physical memory.
*/
static void *allocate_block(int size, struct page *current_page) {
        struct block *tmp_curr = current_page->block_list_head;
        struct block *tmp_prev;
        while (tmp_curr != NULL) {
                /* Find a suitable block and allocate it. */
                if (tmp_curr->state == UNALLOCATED && tmp_curr->size >= size) {
                        if (tmp_curr->total_size == size) { /* Don't need to split the block. */
                                struct block alloc_blk; /* Allocated block of the split */
                                alloc_blk.block_address = tmp_curr->block_address;
                                alloc_blk.data_address = alloc_blk.block_address + sizeof(struct block);
                                alloc_blk.data_size = size;
                                alloc_blk.total_size = size + sizeof(struct block);
                                alloc_blk.state = ALLOCATED;
                                current_page.size_of_allocated += alloc_blk.total_size;
                                /* Adjust the new blocks' next pointers accordingly to maintain this page's block list. */
                                tmp_prev->next = alloc_blk.block_address;
                                alloc_blk.next = tmp_curr->next;
                                /* Copy new blocks to physical memory. */
                                memcpy(alloc_blk.block_address, &alloc_blk, alloc_blk.total_size);
                                return (void *) alloc_blk.data_address;
                        } else { /* Need to split the block into an allocated and unallocated block. */
                                struct block alloc_blk; /* Allocated block of the split */
                                alloc_blk.block_address = tmp_curr->block_address;
                                alloc_blk.data_address = alloc_blk.block_address + sizeof(struct block);
                                alloc_blk.data_size = size;
                                alloc_blk.total_size = size + sizeof(struct block);
                                alloc_blk.state = ALLOCATED;
                                current_page.size_of_allocated += alloc_blk.total_size;
                                struct block unalloc_blk; /* Unallocated block of the split. */
                                unalloc_blk.block_address = alloc_blk.block_address + alloc_blk.total_size;
                                unalloc_blk.data_address = unalloc_blk.block_address + sizeof(struct block);
                                unalloc_blk.data_size = NULL;
                                unalloc_blk.total_size = tmp_curr->total_size - alloc_blk.total_size;
                                unalloc_blk.state = UNALLOCATED;
                                /* Adjust the new blocks' next pointers accordingly to maintain this page's block list. */
                                tmp_prev->next = alloc_blk.block_address;
                                alloc_blk.next = unalloc_blk.block_address;
                                unalloc_blk.next = tmp_curr->next;
                                /* Copy new blocks to physical memory. */
                                memcpy(alloc_blk.block_address, &alloc_blk, alloc_blk.total_size);
                                memcpy(unalloc_blk.block_address, &unalloc_blk, unalloc_blk.total_size);
                                return (void *) alloc_blk.data_address;
                        }
                }
                tmp_prev = tmp_curr;
                tmp_curr = tmp_curr->next;
        }

        return NULL;
}

/* Allocates memory for the scheduler. */
static void allocate_for_scheduler(int size) {
        struct page *tmp = MEMORY_METADATA->page_list_head;
        my_pthread_t curr_tid = get_current_tid();
        while (tmp != NULL) { /* Locate correct page for scheduler. */
                if (tmp->request_id == LIBRARYREQ) { /* tid is NULL for scheduler pages. */
                        /* Is the page full? */
                        if (tmp->size_of_allocated >= MEMORY_METADATA->page_size) {
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
        struct page *tmp = MEMORY_METADATA->page_list_head;
        my_pthread_t curr_tid = get_current_tid();
        while (tmp != NULL) { /* Locate page with matching tid. */
                if (tmp->request_id == THREADREQ && tmp->tid == curr_tid) { /* This is the threads page. */
                        /* Is the page full? */
                        if (tmp->size_of_allocated + size > MEMORY_METADATA->page_size) {
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
        if (PHYSICAL_MEMORY[0] == NULL)
                init_memory_metadata(request_id);
        if (request_id == LIBRARYREQ)
                allocate_for_scheduler(size);
        else (request_id == THREADREQ)
                allocate_for_thread(size);
        return NULL;
}


/*
* Finds the page associated with the calling thread and frees allocated memory from it.
* Should communicate with scheduler to know which thread made the request so it knows which page to free the allocation from.
*/
void my_deallocate(void *ptr, char *FILE, int *LINE, REQUEST_ID request_id) {

}
