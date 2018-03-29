/*
 * parameter - page wanting to swap in, the index frame where the page is in the txt file, the page you want to swap out
 * 
 * want you need to add to global, dont need to put in struct- global counter = 0; int swap_file;
 * in function  init_memory_metadata() - add 
 * swap_file = open("swap_file",O_RDWR | O_CREAT);
		lseek(swap_file, 8338608*2, SEEK_SET);
 */
void swap_pages(struct page swap_in,read_frame, struct page swap_out) {

	
	// returns 1 if enough pages, if ran out of pages returns -1;
	counter ++; // global counter starts at 0. - max pages in txt 2048 *2
		
		
		if(counter < 2048 * 2){
			lseek(swap_file,counter*4096,SEEK_SET); // go to next free location - writing in order
			
			
			page tmp = swap_out;
			while(tmp != NULL){
				if(counter < 2048*2){
								// after incrementing below if no more pages left.
								return -1;
							}
					write(swap_file, tmp.start_address, 4096); // write to swap_file from page address
					
					tmp = tmp.next_owner_page;
					counter++;
				
				}
			
		}else{
			
			// no free pages
			return -1;
		}
		

	

	// read in 1st page to front
	lseek(swap_file,read_frame*4096,SEEK_SET); // go to page in swap_file
	read(swap_file, MEMORY_METADATA.address , 4096); // read to front the 1st page of linked list
	
	int index = 1;
	
	// then swap in rest of linked list
	page tmp2 = swap_in.next_owner_page;
	while(tmp2 != NULL){
		read(swap_file, MEMORY_METADATA.address + index*4096, 4096); // we lseek to location so continue reading to front next page
		index++;
		
	}
	
	
	return 1;
	
	

}

