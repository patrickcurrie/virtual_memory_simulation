
#include "memory_manager.h"
#include "my_pthread_t.h"
#include "my_pthread.c"


void * physical_memory;
page_ptr * fTable;
block_ptr head;
page_ptr current_page; // might not need

int init = 1;
int swap_file;
int pages = 0;
int block_str;
int page_size;
int r_timer;



void * lib_start;
void * swap_loc;
void * shalloc_start;

int count = 0;

	void * allocate_lib(int x){
		printf("lib call worked/n");
		void * tmp = lib_start;
					lib_start += (int)x;
					mprotect(physical_memory, 6291456, PROT_NONE);
					return tmp;

	}


void * myallocate(int x, char * file, int line, REQUEST_ID request_id) {

	r_timer = 1;
	mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);

	void * ret_tmp;
	page_ptr tmp_page;


	if (init) {
		init = 0;
		block_str = sizeof(struct block);
		page_size = sysconf(_SC_PAGESIZE);

		pages = 0;
		//Signal Handler
		struct sigaction sa;
		sa.sa_flags = SA_SIGINFO;
		sigemptyset(&sa.sa_mask);
		sa.sa_sigaction = seg_handler;
		sigaction(SIGSEGV, &sa, NULL);


		physical_memory = memalign( sysconf(_SC_PAGESIZE), 8388608);
		swap_loc = physical_memory + 1540 * 4096;


		swap_file = open("swap_file",O_RDWR | O_CREAT);
		lseek(swap_file, 8338608*2, SEEK_SET);


		lib_start = physical_memory + 1541*4096;
		shalloc_start = physical_memory + 1536*4096;

		fTable = (page_ptr *)myallocate(sizeof(page_ptr)*1536, NULL, 0, LIBRARYREQ);
		r_timer = 1;
		mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);
	}


	if(request_id == LIBRARYREQ){

		return allocate_lib(x);

		}
		if(pages > 5632){
			return NULL;

		}
	//size required is more than a page
	if (x > (page_size - block_str) ) {


		int i;
		int num_of_pages = (x + block_str) / page_size;

		if ((x+block_str) % page_size > 0) {
			num_of_pages++;
		}
		//would need to move everything out of array
		if (num_of_pages > 1536) {
						return NULL;
					}
		//First page - move them into front
		if (get_tcb2()->head == NULL) {

			for (i = 0; i < num_of_pages; i++) {
				swap_pages(i, -1);
				mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);
			}

			//REQUEST_ID state = LIBRARYREQ;
			page_ptr front_page = (page_ptr)myallocate(sizeof(struct page), NULL, 0, LIBRARYREQ);
			r_timer = 1;
			mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);
			front_page->frame = 0;
			front_page->tid = get_current_tid();
			front_page->block_head = NULL;
			front_page->next = NULL;
			front_page->front = front_page;
			get_tcb2()->head = front_page;
			front_page->num_of_pages = num_of_pages;
			front_page->size = -1;
			fTable[0] = front_page;

			//Create rest of pages needed
			for (i = 1; i < num_of_pages; i++) {
				tmp_page = (page_ptr)myallocate(sizeof(struct page), NULL, 0, LIBRARYREQ);
				r_timer = 1;
				mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);
				tmp_page->frame = i;
				tmp_page->tid = get_current_tid();
				tmp_page->next = NULL;
				fTable[i-1]->next = tmp_page;
				tmp_page->block_head = NULL;
				tmp_page->size = -1;
				tmp_page->num_of_pages = num_of_pages;
				tmp_page->front = front_page;
				fTable[i] = tmp_page;
			}

			get_tcb2()->last_frame = num_of_pages - 1;

			current_page = tmp_page;
		//DO not need to make 1st frame
		} else {
			if (get_tcb2()->last_frame + num_of_pages > 1535) {
				return NULL;
			}

			get_tcb2()->last_frame++;
			for (i = get_tcb2()->last_frame; i < get_tcb2()->last_frame + num_of_pages; i++) {
				swap_pages(i, -1);
				mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);
			}

			//front
			page_ptr front_page = (page_ptr)myallocate(sizeof(struct page), NULL, 0, LIBRARYREQ);
			r_timer = 1;
			mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);
			front_page->frame = get_tcb2()->last_frame;
			front_page->tid = get_tcb2()->tid;
			front_page->next = NULL;
			fTable[get_tcb2()->last_frame-1]->next = front_page;
			front_page->block_head = NULL;
			front_page->front = front_page;
			front_page->size= -1;
			front_page->num_of_pages = num_of_pages;

			get_tcb2()->head = front_page;
			fTable[front_page->frame] = front_page;

			//create rest of the pages
			for (i = front_page->frame + 1; i < front_page->frame + num_of_pages; i++) {
				get_tcb2()->last_frame++;
				tmp_page = (page_ptr)myallocate(sizeof(struct page), NULL, 0, LIBRARYREQ);
				r_timer = 1;
				mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);
				tmp_page->frame = i;
				tmp_page->tid = get_tcb2()->tid;
				tmp_page->next = NULL;
				fTable[i-1]->next = tmp_page;
				tmp_page->block_head = NULL;
				tmp_page->front = front_page;
				tmp_page->num_of_pages = num_of_pages;
				tmp_page->size = -1;
				fTable[i] = tmp_page;
			}

			current_page = tmp_page;
		}


		head = NULL;
		// pages allocated next to each other, will fill it out in 1 go
		ret_tmp = mymalloc(x, physical_memory + current_page->front->frame*page_size, page_size*num_of_pages);// todo: make this

		current_page->block_head = head;
		current_page->size = count_mem();
		mprotect(physical_memory, 6291456, PROT_NONE);

		r_timer = 0;
		return ret_tmp;
		//can ret memory
	}


	if (get_tcb2()->head != NULL) {
		page_ptr page_tmp = fTable[get_tcb2()->last_frame];

		//not in frame table need to swap it in
		if (page_tmp->tid != get_tcb2()->tid) {

			page_tmp = get_tcb2()->head;
			while (page_tmp->next != NULL) {
				page_tmp = page_tmp->next;
			}
			swap_pages(get_tcb2()->last_frame, page_tmp->frame);
			mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);
		}
		current_page = page_tmp;

		//front page exists so part of multiple pages
		if (current_page->front != NULL) {
			int i;
			page_ptr swap_pg;

			swap_pg = current_page->front;
			for (i = current_page->front->frame; i < current_page->frame; i++) {
				page_tmp = fTable[i];
				if (page_tmp != swap_pg) {
					swap_pages(page_tmp->frame, swap_pg->frame);
					mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);
				}
				swap_pg = swap_pg->next;
			}
		}
		head = current_page->block_head;

	} else {

		//can make more pages
		if (pages < 5632) {
			swap_pages(0, -1);
			mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);
			tmp_page = (page_ptr)myallocate(sizeof(struct page), NULL, 0, LIBRARYREQ);
			r_timer = 1;
			mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);
			tmp_page->frame = 0;
			tmp_page->tid = get_tcb2()->tid;
			tmp_page->next = NULL;
			tmp_page->block_head = NULL;
			tmp_page->front = NULL;
			tmp_page->num_of_pages = 1;
			get_tcb2()->head = tmp_page;
			fTable[0] = tmp_page;
			current_page = tmp_page;
		} else {
			return NULL;
		}
		head = NULL;
			}

	ret_tmp = mymalloc(x, physical_memory + get_tcb2()->last_frame*4096, 4096);
	current_page->block_head = head;

	current_page->size = count_mem();

	/*/
	 * no room in page, look through its prev pages
	 */
	if (ret_tmp == NULL) {

		page_ptr page_tmp = get_tcb2()->head;
		int counter = 0;
		while (page_tmp != NULL) {

			if (page_tmp->size > (int)x) {
				current_page = page_tmp;


				if (current_page->frame != counter) {
					swap_pages(counter, current_page->frame);
					mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);
				}

				if (current_page->front != NULL) {
					int i;
					page_ptr swap_pg;
					page_ptr block_tmp;

					swap_pg = current_page->front;
					for (i = current_page->front->frame; i < current_page->frame; i++) {
						block_tmp = fTable[i];
						if (block_tmp != swap_pg) {
							swap_pages(block_tmp->frame, swap_pg->frame);
							mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);
						}
						swap_pg = swap_pg->next;
					}
				}


				head = current_page->block_head;

				ret_tmp = mymalloc(x, 0, 0);
				if (ret_tmp != NULL) {
					current_page->block_head = head;
					current_page->size = count_mem();
					mprotect(physical_memory, 6291456, PROT_NONE);

					r_timer = 0;
					return ret_tmp;
				}
			}
			counter++;
			page_tmp = page_tmp->next;
		}


	//need to create new page since the prev pages didnt fit
		if (pages < 5632 && get_tcb2()->last_frame < 1536) {
			get_tcb2()->last_frame++;


			swap_pages(get_tcb2()->last_frame, -1);
			mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);
			tmp_page = (page_ptr)myallocate(sizeof(struct page), NULL, 0, LIBRARYREQ);
			r_timer = 1;
			mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);
			tmp_page->frame = get_tcb2()->last_frame;
			tmp_page->tid = get_tcb2()->tid;
			tmp_page->next = NULL;
			tmp_page->block_head = NULL;
			tmp_page->num_of_pages = 1;
			fTable[get_tcb2()->last_frame] = tmp_page;
			fTable[get_tcb2()->last_frame-1]->next = tmp_page;
			current_page = tmp_page;
		} else {
			return NULL;
		}
		head = NULL;

		ret_tmp = mymalloc(x, physical_memory + get_tcb2()->last_frame ,page_size); //todo:
		current_page->block_head = head;
		current_page->size = count_mem();
	}
	mprotect(physical_memory, 6291456, PROT_NONE);

	r_timer = 0;
	return ret_tmp;
}

int mydeallocate(void * x, char * file, int line, REQUEST_ID request_id){


	}




/* signal handler */

void seg_handler(int sig, siginfo_t *si, void *unused) {



	void * seg_address = (void *) si->si_addr;
	int location = (int)(seg_address - physical_memory);
	int frame = location / 4096;

	if (location < 0 || location > 1536*4096) {
		exit(EXIT_FAILURE);
	}

	if (frame > get_tcb2()->last_frame) {
		exit(EXIT_FAILURE);
	}

	if (fTable[frame]->tid != get_tcb2()->tid) {
		int i;
		page_ptr page_tmp = get_tcb2()->head;
		for (i = 0; i < frame; i++) {
			page_tmp = page_tmp->next;
		}
		swap_pages(frame, page_tmp->frame);
		mprotect(physical_memory, 6291456, PROT_NONE);
	}
	mprotect(physical_memory + frame * 4096, 4096,PROT_READ | PROT_WRITE);
	return;
}

void swap_pages(int wFrame, int rFrame) {

	count++;

	r_timer = 1;
	mprotect(physical_memory, 6291456, PROT_READ | PROT_WRITE);

	int swap_index;
	if (rFrame == -1) {

		if(pages < 1537){



			memcpy(physical_memory + pages*4096, physical_memory + wFrame * 4096, 4096);

			if (fTable[wFrame] != NULL) {
				fTable[wFrame]->frame = pages;
				fTable[pages] = fTable[wFrame];
				fTable[wFrame] = NULL;
			}
		}else{
			fTable[wFrame]->frame = pages;
			swap_index = pages - 1537;
			lseek(swap_file, swap_index*4096, SEEK_SET);
			write(swap_file, physical_memory + wFrame * 4096, 4096);
		}
		pages++;
	} else {

		if (rFrame < 1537) {
			memcpy(swap_loc, physical_memory + wFrame * 4096, 4096);
			memcpy(physical_memory + wFrame * 4096, physical_memory + rFrame * 4096, 4096);
			memcpy(physical_memory + rFrame * 4096, swap_loc, 4096);

			page_ptr tmp_p = fTable[wFrame];
			fTable[wFrame] = fTable[rFrame];
			fTable[rFrame] = tmp_p;
			fTable[wFrame]->frame = wFrame;
			fTable[rFrame]->frame = rFrame;
		} else {
			int i;
			if (get_tcb2()->tid == 2) {
				//printf("Swapping %d with %d\n", wFrame, rFrame);
			}
			page_ptr tmp_p = get_tcb2()->head;
			fTable[wFrame]->frame = rFrame;
			for (i = 0; i< wFrame; i++) {
				tmp_p = tmp_p->next;
			}
			tmp_p->frame = wFrame;
			fTable[wFrame] = tmp_p;

			memcpy(swap_loc, physical_memory + wFrame * 4096, 4096);
			lseek(swap_file, (rFrame - 1537)*4096, SEEK_SET);
			read(swap_file, physical_memory + wFrame*4096, 4096);
			lseek(swap_file, (rFrame - 1537)*4096, SEEK_SET);
			write(swap_file, swap_loc, 4096);
		}
	}
	r_timer = 0;
}

void lock_mem() {
	mprotect(physical_memory, 6291456, PROT_NONE);
}


int count_mem() {
	block_ptr tmp_block = head;
	int rtn = 0;

	while(tmp_block != NULL) {
		if (tmp_block->state == UNALLOCATED) {
			rtn += tmp_block->size;
		}
		tmp_block = tmp_block->next;
	}

	return rtn;
}


void *mymalloc(int size_needed, void* memory, int allocated){

	//unsigned int x, char * file, int line, void * memptr, int size
	//x, file, line, myblock_ptr + current_page_meta->front_meta->page_frame*4096, 4096*num_of_pages
	if ( head == NULL) {
			head = (block_ptr)memory;
			head->size = size_needed - sizeof(struct block);
			head->state = UNALLOCATED;
			head->next = NULL;
		}

	block_ptr tmp_block = head;


			while(tmp_block != NULL){			//Checks for free space to the right of the middle.
				if(tmp_block->state!= ALLOCATED) {
					if(tmp_block->size >= size_needed) {
						if(tmp_block->size > (size_needed + sizeof(struct block))) {			
							block_ptr tmp = (block_ptr)(((char *)tmp_block) + sizeof(struct block) + size_needed);	

												
							tmp->size = tmp_block->size - sizeof(struct block) - size_needed;
							tmp->state=UNALLOCATED ;
							tmp_block -> next = tmp;



							tmp_block->size = size_needed;													
							tmp_block->state =ALLOCATED;


							
							return tmp_block+1;					
						}else {
							tmp_block->state = ALLOCATED;

							
							return tmp_block+1;					
						}
					}else {
						tmp_block = tmp_block->next;
					}
				}else {
					tmp_block = tmp_block->next;
				}
			}




}

void myfree(){



}

int printCount(){return count;}



