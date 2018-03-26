CC = gcc
CFLAGS = -g -c -O0
AR = ar -rc
RANLIB = ranlib


Target: my_pthread.a

my_pthread.a: my_pthread.o memory_manager.o
	$(AR) libmy_pthread.a my_pthread.o memory_manager.o
	$(RANLIB) libmy_pthread.a

my_pthread.o: my_pthread_t.h
	$(CC) -pthread $(CFLAGS) my_pthread.c

memory_manager.o: memory_manager.h
	$(CC) $(CFLAGS) memory_manager.c

clean:
	rm -rf testfile *.o *.a
