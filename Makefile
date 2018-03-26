all: my_pthread_t.h my_pthread.c memory_manager.h memory_manager.c
	gcc -Wall -c memory_manager.c -g

clean:
	rm *.o test swap_file -f
