#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tps.h>
#include <sem.h>

static sem_t semt1, semt2; // global vars
void *pageaddr;
void *__real_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
void *__wrap_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off){
	pageaddr = __real_mmap(addr, len, prot, flags, fildes, off); // overload here as specified on piazza
	return pageaddr;
}

void inittest(void){
	int test = tps_init(1); // double init failure test
	if(test == -1){
		printf("Error Success\n");
	}
}

void createtest(void){
	int test1,test2;
	test1 = tps_create(); // should be successful
	if(test1 == 0){
	printf("Test1 successful\n");
	}
	test2 = tps_create(); // should spit out error
	if(test2 == -1){
		printf("Test2 error successful\n");
	}
}
void readtest(void){
 	char buffer[10];
	int len = 10;
	int offset = 0;
	int test;
	test = tps_read(offset,len,buffer);
	if(test == 0){
		printf("Error check 1 works: %s with offset %d\n",buffer,offset);
	}
	offset = 4096; //testing for size check
	test = tps_read(offset,len,buffer);
	if(test == -1){
		printf("Error check 2 works: %s with offset %d\n", buffer,offset);
	}
	srand(time(NULL));//testing random variables between 1 and 10 here
	offset = rand() %10 + 1;
	test = tps_read(offset,len,buffer);
	if(test == 0){
		printf("Error check 3 works: %s with offset %d\n", buffer,offset);
}
}
void writetest(void){
	char *buffer = "Write test string";
	int len = sizeof(buffer)/sizeof(buffer[0]);
	int offset = 0; // test if works
	int test;
	test = tps_write(offset, len, buffer);
	if(test == 0){
		printf("Write error check 1 successful: %s\n",buffer);
}
	offset = 4096; // out of bounds
	test = tps_write(offset,len,buffer);
	if(test == -1){
		printf("Write error check 2 successful: %s\n",buffer);
	}
}
void *testthread1(void *arg){
tps_create();
sem_up(semt1);
sem_down(semt2);
return NULL;
}
void *testthread2(void *arg){
pthread_t tid;
int test;
pthread_create(&tid, NULL, testthread1, NULL);
test = tps_clone(tid);
if(test == -1){
	printf("Clone error check 1 successful\n");
}
tps_create();
sem_down(semt1);
test = tps_clone(tid);
if(test == -1){
	printf("Clone error check 2 successful\n");
}
sem_up(semt2);
return NULL;
}
void clonetest(void){
pthread_t tid;
semt1 = sem_create(0);
semt2 = sem_create(0);
pthread_create(&tid, NULL, testthread2, NULL);
pthread_join(tid, NULL);
sem_destroy(semt1);
sem_destroy(semt2);
}
void destroytest(void){
 int test;
 test = tps_destroy();
 if(test == 0){
	 printf("Destroy successful\n");
 }
 test = tps_destroy();
 if(test == -1){
	 printf("Destroy error successful\n");
 }
}
void *testthread3(void* arg){
	char *testaddr; //also check the overload
	tps_create();
	testaddr = pageaddr;
	testaddr[0] = '\0';//accessing null variable check
	tps_destroy();
	return NULL;
}
void segtest(void) {
	pthread_t tid;
	pthread_create(&tid, NULL, testthread3, NULL);
	pthread_join(tid, NULL);
}
int main(){
	inittest();
	createtest();
	readtest();
	writetest();
	destroytest();
	clonetest();
	segtest();
	return 0;
}
