#include <stdlib.h>
#include <stdio.h>
#include "my_pthread_t.h"
#include <malloc.h>

int * arr1;

typedef struct __myarg_t { 
	int a;
	int b; 
} myarg_t;



void *mythread1(void *arg){
	
	myarg_t *m = (myarg_t *) arg;
	
	int * intArray = (int *) malloc(sizeof(int)*2);
	//printf("intArray thread2 address: %p\n", (void *)intArray);
	intArray[0] = m->a;
	intArray[1] = m->b;
	printf("qq\n");
printf("qq\n");
printf("qq\n");
	printf("[%d, %d]\n", intArray[0], intArray[1]);
	
	
	return NULL;
}



int main() {
	
	my_pthread_t mp;
	myarg_t args1;
	args1.a = 33;
	args1.b = 22;
	printf("before %d\n",args1.a );
	my_pthread_create(&mp, NULL, mythread1, &args1);
	my_pthread_join(mp, NULL);
	
	printf("DONE\n");
	
	
		
	return 0;
}
