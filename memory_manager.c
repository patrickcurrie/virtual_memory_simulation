#include "memory_manager.h"
#include "my_pthread_t.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
void * swap_loc;
static char PHYSICAL_MEMORY[8388608]; /* 8 MB to simulate physical memory. */
char *mem_pointer = PHYSICAL_MEMORY; //easier to use
PHYSICAL_MEMORY[0] = NULL; /* NULL if memory metadata is not initialized. */
struct memory_metadata MEMORY_METADATA;
int page_mem_ftr;
struct page *fTable; // todo: need to allocate sizeof(struct page) * (8388608/4096) using lib id
int swap_file;
int pages;
int page_mem_ftr;


/*
 * signal handler, finds frame that it tried to access. if it's tid does not match wrong frame. Swap in right one
 * else just unprotect it cause tid matched
 */
static void seg_handler(int sig, siginfo_t *si, void *unused){
	void * segfault = (void *) si->si_addr;
		int loc = (int)(segfault - mem_pointer);
		int frame = loc / 4096;

		if (fTable[frame].tid != get_tcb()->tid) {
			int i;
					struct page tmp = get_tcb()->head;
					for (i = 0; i < frame; i++) {
						tmp = tmp.n2;
					}
					swap_pages(frame, tmp->frame);
					mprotect(mem_pointer, 8388608, PROT_NONE);



		}


		mprotect(mem_pointer + frame * 4096, 4096,PROT_READ | PROT_WRITE);

        }

/*
 * handles all the swapping between the physical memory with itself and the virtual memory.
 */
static void swap(int x, int y){
/*/
 * y is y is what need to swapped in based on index
 */


	int swap_index;
	if (y == -1) {

		if(pages < 2048){


			//Swap
			memcpy(mem_pointer + pages*4096, mem_pointer + x * 4096, 4096);

			//Update page table
			if (fTable[x] != NULL) {
				fTable[x].frame = pages;
				fTable[pages] = fTable[x];
				fTable[x] = NULL;
			}
		}else{
			fTable[x].frame = pages;
			swap_index = pages - 2048;
			lseek(swap_file, swap_index*4096, SEEK_SET);
			write(swap_file, mem_pointer + x * 4096, 4096);
		}
		pages++;
	} else {

		if (y < 2048) {
			memcpy(swap_loc, mem_pointer + x * 4096, 4096);
			memcpy(mem_pointer + x * 4096, mem_pointer + y * 4096, 4096);
			memcpy(mem_pointer + y * 4096, swap_loc, 4096);

			//Update page table
			struct page  tmp_page = fTable[x];
			fTable[x] = fTable[y];
			fTable[y] = tmp_page;
			fTable[x].frame = x;
			fTable[y].frame = y;
		} else {
			int i;

			struct page  tmp_page = get_tcb()->head;
			fTable[x].frame = y;
			for (i = 0; i< x; i++) {
				tmp_page = tmp_page->next;
			}
			tmp_page. = x;
			fTable[x] = tmp_page;

			memcpy(swap_loc, mem_pointer + x * 4096, 4096);
			lseek(swap_file, (y - 2048)*4906, SEEK_SET);
			read(swap_file, mem_pointer + x*4096, 4096);
			lseek(swap_file, (y - 2048)*4906, SEEK_SET);
			write(swap_file, swap_loc, 4096);
		}
	}

}


/* Allocates a new page in physical memory. */
static void assign_page(REQUEST_ID request_id, my_pthread_t tid) {
        char *tmp_curr = MEMORY_METADATA.page_list_head;
        char *tmp_prev;
        struct page pg;
        while (tmp_curr != NULL) {
                /* Find a suitable page and assign it. */
                memcpy(&pg, tmp_curr->start_address, sizeof(struct page));
                if (pg.state == UNASSIGNED) {
                        if (pg.end_address - pg.start_address + 1 == MEMORY_METADATA.page_size) {
                                struct page assign_pg
                                assign_pg.request_id = request_id;
                                assign_pg.tid = tid;
                                assign_pg.size_of_allocated = sizeof(struct page);
                                assign_pg.start_address = pg.start_address;
                                assign_pg.end_address = pg.start_address + MEMORY_METADATA.page_size - 1;
                                //assign_pg.block_list_head = NULL; /* Create new big block for this. */
                                assign_pg.next = pg.next;
                                /* Create a new unallocated block for this page's block list. */
                                struct block unalloc_blk;
                                unalloc_blk.block_address = assign_pg.start_address + sizeof(struct page);
                                unalloc_blk.data_address = unalloc_blk.block_address + sizeof(struct block);
                                unalloc_blk.data_size = NULL;
                                unalloc_blk.total_size = MEMORY_METADATA.page_size - sizeof(struct page);
                                unalloc_blk.state = UNALLOCATED;
                                /* Assign this block to the newly assigned page's block list. */
                                memcpy(unalloc_blk.block_address, &unalloc_blk, sizeof(struct block));
                                assign_pg.block_list_head = unalloc_blk.block_address;
                                assign_pg.state = ASSIGNED;
                                memcpy(assign_pg.start_address, &assign_pg, sizeof(struct page));
                                //return assign_pg.start_address;
                                return;
                        } else { /* Split unassigned page int an assigned page and unassigned page. */
                                /* Create assigned page. */
                                struct page assign_pg
                                assign_pg.request_id = request_id;
                                assign_pg.tid = tid;
                                assign_pg.size_of_allocated = sizeof(struct page);
                                assign_pg.start_address = pg.start_address;
                                assign_pg.end_address = pg.start_address + MEMORY_METADATA.page_size - 1;
                                /* Create an unallocated block for assigned page. */
                                struct block unalloc_blk;
                                unalloc_blk.block_address = assign_pg.start_address + sizeof(struct page);
                                unalloc_blk.data_address = unalloc_blk.block_address + sizeof(struct block);
                                unalloc_blk.data_size = NULL;
                                unalloc_blk.total_size = MEMORY_METADATA.page_size - sizeof(struct page);
                                unalloc_blk.state = UNALLOCATED;
                                /* Assign this block to the newly assigned page's block list. */
                                memcpy(unalloc_blk.block_address, &unalloc_blk, sizeof(struct block));
                                assign_pg.block_list_head = unalloc_blk.block_address;
                                /* Create unassigned page. */
                                struct page unassign_pg;
                                unassign_pg.request_id = NULL;
                                unassign_pg.tid = NULL;
                                unassign_pg.size_of_allocated = sizeof(struct page);
                                unassign_pg.start_address = assign_pg.end_address + 1;
                                unassign_pg.end_address = pg.end_address;
                                unassign_pg.block_list_head = NULL;
                                /* Adjust the page list accordingly. */
                                struct page prev_pg;
                                memcpy(&prev_pg, tmp_prev, sizeof(struct page));
                                prev_pg.next = assign_pg.start_address;
                                assign_pg.next = unassign_pg.start_address;
                                unassign_pg.next = pg.next;
                                assign_pg.state = ASSIGNED;
                                unassign_pg.state = UNASSIGNED;
                                /* Copy newly adjusted pages into physical memory. */
                                memcpy(prev_pg.start_address, &prev_pg, sizeof(struct page));
                                memcpy(assign_pg.start_address, &assign_pg, sizeof(struct page));
                                memcpy(unassign_pg.start_address, &unassign_pg, sizeof(struct page));
                                //return assign_pg.start_address;
                                return;
                        }
                }
                tmp_prev = tmp_curr;
                tmp_curr = pg.next;
        }

        /*
        * No room for new page, write pages out to a swap file to create room.
        * Call assign_page(request_id, tid) again to assign a new page now that there is room.
        */
}

/*
* Sets memory metadata and creates an unassigned page.
*/
static void init_memory_metadata(REQUEST_ID request_id) {
	mem_pointer = memalign( sysconf(_SC_PAGESIZE), 8388608);
	swap_file = open("swap_file",O_RDWR | O_CREAT);
			lseek(swap_file, 8338608*2, SEEK_SET);


			swap_loc = mem_pointer + 2047 * 4096;

	struct sigaction sa;
			sa.sa_flags = SA_SIGINFO;
			sigemptyset(&sa.sa_mask);
			sa.sa_sigaction = seg_handler;


        /* Initialize memory metadata. */

	page_mem_ftr=(sizeof(struct page) + sizeof(struct block);
        struct memory_metadata mem_meta;
        mem_meta.page_size = (int) sysconf(_SC_PAGE_SIZE);
        mem_meta.number_pages = 8388608 / mem_meta.page_size;
        mem_meta.address = PHYSICAL_MEMORY;
        mem_meta.page_list_head = NULL;
        memcpy(mem_meta.address, &mem_meta, sizeof(struct memory_metadata));
        //MEMORY_METADATA = mem_meta.address;
        memcpy(MEMORY_METADATA, mem_meta.address, sizeof(struct memory_metadata));
        /*
        * Create the first unassigned page.
        * This will be split into assigned pages as new pages are added.
        */
        struct page pg;
        pg.request_id = NULL;
        pg.tid = NULL;
        pg.size_of_allocated = sizeof(struct page);
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
* Return NULL if suitable block is not found, meaning a different page should be searched instead.
*/
static void *allocate_block(int size, char *page_address) {
        struct page current_page;
        memcpy(&current_page, page_address, sizeof(struct page));
        struct char *tmp_curr = current_page.block_list_head;
        struct char *tmp_prev = NULL;
        struct block blk;
        while (tmp_curr != NULL) {
                /* Find a suitable block and allocate it. */
                memcpy(&blk, tmp_curr, sizeof(struct block));
                if (blk.state == UNALLOCATED && blk.total_size >= size) {
                        if (blk.total_size == size) { /* Don't need to split the block. */
                                struct block alloc_blk; /* Allocated block of the split */
                                alloc_blk.block_address = blk.block_address;
                                alloc_blk.data_address = alloc_blk.block_address + sizeof(struct block);
                                alloc_blk.data_size = size;
                                alloc_blk.total_size = size + sizeof(struct block);
                                alloc_blk.state = ALLOCATED;
                                current_page.size_of_allocated += alloc_blk.total_size;
                                /* Adjust the new blocks' next pointers accordingly to maintain this page's block list. */
                                alloc_blk.next = blk.next;
                                /* Copy new block to physical memory. */
                                memcpy(alloc_blk.block_address, &alloc_blk, sizeof(struct block));
                                /* Copy adjusted page to physical memory. */
                                memcpy(current_page.address, &current_page, sizeof(struct page));
                                return (void *) alloc_blk.data_address;
                        } else { /* Need to split the block into an allocated and unallocated block. */
                                struct block alloc_blk; /* Allocated block of the split */
                                alloc_blk.block_address = blk.block_address;
                                alloc_blk.data_address = alloc_blk.block_address + sizeof(struct block);
                                alloc_blk.data_size = size;
                                alloc_blk.total_size = size + sizeof(struct block);
                                alloc_blk.state = ALLOCATED;
                                current_page.size_of_allocated += alloc_blk.total_size;
                                struct block unalloc_blk; /* Unallocated block of the split. */
                                unalloc_blk.block_address = alloc_blk.block_address + alloc_blk.total_size;
                                unalloc_blk.data_address = unalloc_blk.block_address + sizeof(struct block);
                                unalloc_blk.data_size = NULL;
                                unalloc_blk.total_size = blk.total_size - alloc_blk.total_size;
                                unalloc_blk.state = UNALLOCATED;
                                /* Adjust the new blocks' next pointers accordingly to maintain this page's block list. */
                                if (tmp_prev == NULL) { /* First block is being split, there is no previous block. */
                                        alloc_blk.next = unalloc_blk.block_address;
                                        unalloc_blk.next = blk.next;
                                        /* Copy adjusted blocks to physical memory. */
                                        memcpy(alloc_blk.block_address, &alloc_blk, sizeof(struct block));
                                        memcpy(unalloc_blk.block_address, &unalloc_blk, sizeof(struct block));
                                } else { /* Any block other than the first is being split, there is a previous block. */
                                        struct block prev_blk;
                                        memcpy(&prev_blk, tmp_prev, sizeof(struct block));
                                        prev_blk.next = alloc_blk.block_address;
                                        alloc_blk.next = unalloc_blk.block_address;
                                        unalloc_blk.next = blk.next;
                                        /* Copy adjusted blocks to physical memory. */
                                        memcpy(prev_blk.block_address, &prev_blk, sizeof(struct block));
                                        memcpy(alloc_blk.block_address, &alloc_blk, sizeof(struct block));
                                        memcpy(unalloc_blk.block_address, &unalloc_blk, sizeof(struct block));
                                }
                                /* Copy adjusted page to physical memory. */
                                memcpy(current_page.address, &current_page, sizeof(struct page));
                                return (void *) alloc_blk.data_address;
                        }
                }
                tmp_prev = tmp_curr;
                tmp_curr = blk.next;
        }

        /*
        * Suitable block is not found. Find the next page to search.
        */

        return NULL;
}

/* Allocates memory for the scheduler. */
static void allocate_for_scheduler(int size) {
        char *tmp = MEMORY_METADATA.page_list_head;
        struct page pg;
        while (tmp != NULL) { /* Locate correct page for scheduler. */
                memcpy(&pg, tmp, sizeof(struct page));
                if (pg.request_id == LIBRARYREQ) { /* tid is NULL for scheduler pages. */
                        /* Is the page full? */
                        if (pg.size_of_allocated >= MEMORY_METADATA.page_size) {
                                tmp = pg.next;
                                continue;
                        }
                        void *blk_addr = allocate_block(size, pg.start_address);
                        if (blk_addr != NULL)
                                return blk_addr;
                }
                tmp = pg.next;
        }

        assign_page(LIBRARYREQ, NULL);
        allocate_for_scheduler(size);
}

/* Allocates memory for a thread. */
static void allocate_for_thread(int size) {
	/* New
	 * if need multiple pages
	 */
	 struct page tmp_page;
	 struct page front;

	if(size >  (4096 - (sizeof(struct page) + sizeof(struct block))  )  ){

				int i;
				int num_of_pages = (size + page_mem_ftr) / 4096;

				if ((size+page_mem_ftr) % 4096 > 0) {
					num_of_pages++;
				}

			if(get_tcb() -> head == NULL ){

				 front.frame = 0;
				 front.tid = get_current_tid();
				 front.n2 = NULL;
				 front.front = front;

				 front.num_of_pages = num_of_pages;
				 			front.state = ASSIGNED;
				 			get_tcb()->head = front;
				 			fTable[0] = front;


				 for(i=1;i<num_of_pages;i++){

				tmp_page.frame = i;
				tmp_page.tid = get_current_tid();
				tmp_page.n2 = NULL;
				fTable[i-1]n2 = tmp_page;

				tmp_page.front = front;
				tmp_page.num_of_pages = num_of_pages;
				tmp_page.state = ASSIGNED;
				fTable[i] = tmp_page;
				 }
				 get_tcb()->last = num_of_pages - 1;
			}else{

				get_tcb()->last++;


							front.frame = get_tcb()->last;
							front.tid = get_current_tid;
							front.n2 = NULL;
							fTable[get_tcb()->last-1]->n2 = front;

							front.front = front;
							front.num_of_pages = num_of_pages;
							front.state = ASSIGNED;
							get_tcb()->head = front;
							fTable[front.frame] = front;

							for (i = front.frame + 1; i < front.frame + num_of_pages; i++) {
											get_tcb()->last++;


											tmp_page.frame = i;
											tmp_page.tid = get_current_tid();
											tmp_page.n2 = NULL;
											fTable[i-1].n2 = tmp_page;

											tmp_page.front = front;
											tmp_page.num_of_pages = num_of_pages;
											tmp_page.state = ASSIGNED;
											fTable[i] = tmp_page;
										}



			}

			//call block code here
			//ret
			void *blk_addr = allocate_block(4096-sizeof(struct block), front.start_address);
			return blk_addr;

	}



        char *tmp = MEMORY_METADATA.page_list_head;
        struct page pg;
        my_pthread_t curr_tid = get_current_tid();
        while (tmp != NULL) { /* Locate page with matching tid. */
                memcpy(&pg, tmp, sizeof(struct page));
                if (pg.request_id == THREADREQ && pg.tid == curr_tid) { /* This is the threads page. */
                        /* Is the page full? */
                        if (pg.size_of_allocated + size >= MEMORY_METADATA.page_size) {
                                tmp = pg.next;
                                continue;
                        }
                        void *blk_addr = allocate_block(size, pg.start_address);
                        if (blk_addr != NULL)
                                return blk_addr;
                }
                tmp = pg.next;
        }

        assign_page(THREADREQ, tid);
        allocate_for_thread(size);
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

