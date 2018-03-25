CC = gcc
CFLAGS = -g -c -O0
AR = ar -rc
RANLIB = ranlib


Target: memory_manager.a

memory_manager.a: memory_manager.o
	$(AR) libmemory_manager.a memory_manager.o
	$(RANLIB) libmemory_manager.a

memory_manager.o: memory_manager.h
	$(CC) -pthread $(CFLAGS) memory_manager.c

clean:
	rm -rf testfile *.o *.a
