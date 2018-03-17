#include <stdio.h>
#include <string.h>

enum REQID {
        LIBRARYREQ,
        THREADREQ
};

// When user calls malloc or free from a thread.
#define malloc(x) my_allocate(x, __FILE__, __LINE__, THREADREQ)
#define free(x) my_deallocate(x, __FILE__, __LINE__, THREADREQ)

void *my_allocate(int size, char *FILE, int *LINE, REQID threadreq);
void my_deallocate(void *ptr, char *FILE, int *LINE, REQID threadreq);
