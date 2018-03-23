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


struct block* findPage(int tid){

	struct page *tmp = PAGE_LIST.head;

	while(tmp->next != NULL){
		if(tmp->tid == tid){

		return tmp;

		}


		tmp=tmp->next;
	}


}



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
                        struct block alloc_blk;
                        alloc_blk.address = tmp_curr.address;
                        alloc_blk.size = size;
                        alloc_blk.state = ALLOCATED;
                        current_page.size_of_allocated += size;
                        struct block unalloc_blk;
                        unalloc_blk.address = tmp_curr.address + size;
                        /* Size of unalloc_blk is based on whether it is the last block in the list or not. */
                        unalloc_blk.size = tmp_curr.next == NULL ? PAGE_SIZE - current_page.size_of_allocated : tmp_curr.size - alloc_blk.size;
                        unalloc_blk.state = UNALLOCATED;
                        /* Adjust the page list accordingly. */
                        tmp_prev.next = alloc_blk;
                        alloc_blk.next = unallock_blk;
                        unalloc_blk.next = tmp_curr.next == NULL ? NULL : tmp_curr.next;
                        /*
                        * Function to store block metadata goes here.
                        * store_block_metadata(size, current_page);
                        */
                        //done so should ret address here
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



	/*
	 * psuedo code
	 * if size is bigger than page size allocate right number of pages

	 * if not enough space - allocate another page
	 * figure out when to use mprotect
	 */
		// need more than 1 page - assuming page struct goes at the top of each page
		if(size > PAGE_SIZE -sizeof(struct page) ){
			int num_pages = size/ (PAGE_SIZE - sizeof(struct page));

		}
		int foundpage = 0;
        struct page *tmp = PAGE_LIST.head;
        my_pthread_t curr_tid = get_current_tid();
        while (tmp != NULL) { /* Locate page with matching tid. */
                if (tmp->tid == curr_tid) { /* This is the threads page. */
                        /* Is the page full? */
                        if (tmp->size_of_allocated + size + sizeof(struct block) > PAGE_SIZE) {
                                tmp = tmp->next;
                                continue;
                        }
                        foundpage=1;
                        allocate_block(size, tmp);
                }
                tmp = tmp->next;
        }

        // need to allocated new page
        if(!foundpage){
        	struct page *tidPage = findPage(curr_tid); //find the page associated with tid
        	struct page *tmp2 = PAGE_LIST.head;

        	while(tmp2->next !=NULL){
        			if(tmp2->isContinuous == 0 && tmp2->tid == -1){ //is free to use and is not used by another page to be continous
        				tmp2->tid = tidPage->tid;
        				tmp2->isContinuous = 1;
        				struct page *pagetmp = tidPage;

        				//add new page to end of original page linked list -
        				while(pagetmp->next != NULL){

        					pagetmp= pagetmp->next;
        				}
        				pagetmp->next = tmp2;


        			}



        	tmp2 = tmp2->next;

        	}


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


        	 struct sigaction sa;
        	        sa.sa_flags = SA_SIGINFO;
        	        sigemptyset(&sa.sa_mask);
        	        sa.sa_sigaction = signal_handler;

        	        if (sigaction(SIGSEGV, &sa, NULL) == -1)
        	        {
        	            printf("Fatal error setting up signal handler\n");
        	            exit(EXIT_FAILURE);    //explode!
        	        }

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



	struct block free = (struct block)((char *)ptr - sizeof(struct block)); // should point to block struct

	if(free->state == UNALLOCATED){
		printf("tried to free unallocated space\n");
	}

	free->state = UNALLOCATED;

		/*
		 * check to see if next is free then merges - do not need while loop will check every free
		 */
	if(free->next != NULL && free->next->state == UNALLOCATED) {
			struct block tmp = free->next;
			if(free->next != NULL) {
				free->next = free->next->next;
			}

			free->size += sizeof(struct block) + tmp->size;


		}

	/*
	 * We need a way to combine blocks before it or we will slowly lose memory -added a prev to block struct
	 */

	if(free->prev != NULL && free->prev->state == UNALLOCATED) {
			if(free->next != NULL) {
				free->next->prev = free->prev;
			}
			if(free->prev != NULL) {
				free->prev->next = free->next;
			}
			free->prev->size += sizeof(struct block) + free->size;


		}



}

void signal_handler(int signal, siginfo_t *si, void *unused) {

}


<<<<<<< HEAD
=======

>>>>>>> branch 'suva2' of https://github.com/patrickcurrie/virtual_memory_simulation.git

