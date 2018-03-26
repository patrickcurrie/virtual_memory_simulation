all: my_pthread.o memory_manager.h memory_manager.o
	gcc -Wall -o test memory_manager.c my_pthread.o -g

my_pthread.o: my_pthread.c my_pthread_t.h
	gcc -Wall -c my_pthread.c -g
	
memory_manager.o: memory_manager.h
	gcc -Wall -c memory_manager.c -g
	
test: memory_manager.o test.c
	gcc -Wall -o test test.c memory_manager.o -g

clean:
	rm *.o test swap_file -f
