#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "queue.h"
#include "thread.h"
#include "tps.h"

queue_t tpsqueue = NULL; // same as P2

typedef struct page {
	void* pageaddr;
	int refcount;
} page;

typedef struct tps{ //API initialize similar to p2 tcb
	pthread_t tid;
	page* tpage;
} tps;
/* TODO: Phase 2 */

int findtid(void *data, void *arg){ // using same function as p2 for most part
	tps *tempdata = data;
	if(tempdata->tid == (*(pthread_t*)arg)){
		return 1;
	}
	return 0;
}

int findpage(void *data, void *arg){ //find matching page here after casting data
	tps *tempdata = data;
	if(tempdata->tpage->pageaddr == arg){
		return 1;
	}
	return 0;
}
static void segv_handler(int sig, siginfo_t *si, __attribute__((unused)) void *context) //copied from project3.html
{
	tps* faultcheck = NULL;
    /*
     * Get the address corresponding to the beginning of the page where the
     * fault occurred
     */
    void *p_fault = (void*)((uintptr_t)si->si_addr & ~(TPS_SIZE - 1));

    /*
     * Iterate through all the TPS areas and find if p_fault matches one of them
     */
    queue_iterate(tpsqueue, findpage, p_fault, (void**)&faultcheck);
    if (faultcheck != NULL)
        /* Printf the following error message */
        fprintf(stderr, "TPS protection error!\n");

    /* In any case, restore the default signal handlers */
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    /* And transmit the signal again in order to cause the program to crash */
    raise(sig);
}
int tps_init(int segv) // same general structure as p2 init
{
	if(tpsqueue != NULL){
		return -1; // make sure init isn't called twice
	}
	/* TODO: Phase 2 */
	tpsqueue=queue_create();
	if(tpsqueue == NULL){
		return -1; // if queue_create fails
	}

	if(segv){
		struct sigaction sa; // copied from project3.html 
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = segv_handler;
        sigaction(SIGBUS, &sa, NULL);
        sigaction(SIGSEGV, &sa, NULL);
	}
	return 0;
}

int tps_create(void)
{
	pthread_t currenttid = pthread_self(); // get threadid here
	tps *tempdata = NULL; //check for thread

	enter_critical_section();// danger zone
	queue_iterate(tpsqueue, findtid, (void*)currenttid, (void**)&tempdata);
	if(tempdata != NULL){
		exit_critical_section();
		return -1; // check to see if tps already exists
	}
	tempdata = malloc(sizeof(struct tps));
	tempdata->tpage = malloc(sizeof(struct page));
	if(tempdata == NULL || tempdata->tpage == NULL){
	exit_critical_section();
	return -1; //check for initializaiton failure
	}
	tempdata->tid = currenttid;	
	tempdata->tpage->refcount = 1;
	tempdata->tpage->pageaddr = mmap(NULL, TPS_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANON, 0, 0); //Didn't know whether to use private or anon, Raza helped https://piazza.com/class/k52pjbhb7ku3ws?cid=347
	if(tempdata->tpage->pageaddr == MAP_FAILED){
		exit_critical_section();
		return -1; // map init failure here
	}
	queue_enqueue(tpsqueue, (void*)tempdata); // add to queue
	exit_critical_section(); // exit danger zone
	return 0; // success
	/* TODO: Phase 2 */
}

int tps_destroy(void)
{
	pthread_t currenttid = pthread_self();
	tps *tempdata = NULL;
	enter_critical_section(); // danger zone
	//same as create but opposite conditions, check for existance
	queue_iterate(tpsqueue, findtid, (void*)currenttid, (void**)&tempdata);
	if(tempdata == NULL){ // mirror of create
		exit_critical_section();
		return -1;
	}
	queue_delete(tpsqueue,(void*)tempdata);//remove from queue if not null
	if(tempdata->tpage->refcount == 1){
		munmap(tempdata->tpage->pageaddr, TPS_SIZE);
		free(tempdata->tpage);
		free(tempdata);
	}
	if(tempdata->tpage->refcount > 1){
		tempdata->tpage->refcount--; // not the correct pag ref, decrement here
	}
	exit_critical_section(); //exit danger zone
	return 0;//success
	/* TODO: Phase 2 */
}

int tps_read(size_t offset, size_t length, void *buffer)
{
	pthread_t currenttid = pthread_self();
	tps *tempdata = NULL;
	enter_critical_section(); // danger zone
	queue_iterate(tpsqueue, findtid, (void*)currenttid, (void**)&tempdata);
	if(tempdata == NULL || buffer == NULL || offset > TPS_SIZE || offset+length > TPS_SIZE){
		exit_critical_section();
		return -1; // error here
	}
	mprotect(tempdata->tpage->pageaddr, TPS_SIZE, PROT_READ);//change read permissions here
	memcpy(buffer, (tempdata->tpage->pageaddr+offset), length);//copy contents with offset and length passed
	mprotect(tempdata->tpage->pageaddr, TPS_SIZE, PROT_NONE); // flip read permissions back here
	exit_critical_section(); //exit danger zone
	return 0; //success
	/* TODO: Phase 2 */
}

int tps_write(size_t offset, size_t length, void *buffer)
{
	pthread_t currenttid = pthread_self();
	tps *tempdata = NULL;
	enter_critical_section(); // danger zone
	queue_iterate(tpsqueue, findtid, (void*)currenttid, (void**)&tempdata);
	if(tempdata == NULL || buffer == NULL || offset > TPS_SIZE || offset+length > TPS_SIZE){
		exit_critical_section();
		return -1; // error here
	}
	if(tempdata->tpage->refcount > 1){ //same as page create, but different count check for new page allocation
		page* temppage = malloc(sizeof(struct page));
	
	if(temppage == NULL){
		exit_critical_section();
		return -1; // init failure
	}
	temppage->pageaddr = mmap(NULL, TPS_SIZE, PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);
	if(temppage->pageaddr == MAP_FAILED){
		exit_critical_section();
		return -1; //map init failure here
		}
	temppage->refcount=1;
	mprotect(tempdata->tpage->pageaddr, TPS_SIZE, PROT_READ | PROT_WRITE);//change read/write permissions here
	memcpy(temppage->pageaddr,tempdata->tpage->pageaddr, TPS_SIZE); // copy old address to new
	mprotect(tempdata->tpage->pageaddr, TPS_SIZE,PROT_NONE);//original goes back to protected here
	mprotect(temppage->pageaddr, TPS_SIZE, PROT_NONE);//new page protected here
	tempdata->tpage->refcount--;
	tempdata->tpage = temppage; // point to new page
	}
		mprotect(tempdata->tpage->pageaddr, TPS_SIZE, PROT_READ | PROT_WRITE); // allow read/write permissions here
		memcpy(tempdata->tpage->pageaddr, buffer, length); // copy contents over from buff
		mprotect(tempdata->tpage->pageaddr, TPS_SIZE, PROT_NONE); // flip permissions back here
	
	exit_critical_section(); //exit danger zone
	return 0;//success
	/* TODO: Phase 2 */
}

int tps_clone(pthread_t tid)
{
	pthread_t currenttid = pthread_self();
	tps *tempdata = NULL; // thread to find and copy
	tps *targetdata = NULL; // thread to get
	enter_critical_section();//danger zone
	queue_iterate(tpsqueue, findtid,(void*)currenttid, (void**)&tempdata);
	if(tempdata != NULL){
		exit_critical_section();
		return -1;//init error here
	}
	tempdata = malloc(sizeof(tps)); // mem alloc here if null
	if(tempdata == NULL){
		exit_critical_section();
		return -1; //init error here
	}
	tempdata->tid = currenttid; // point to current thread
	queue_iterate(tpsqueue,findtid,(void*)tid,(void**)&targetdata);
	if(targetdata == NULL){
		exit_critical_section();
		return -1; //init error here
	}
	tempdata->tpage = targetdata->tpage;//copy here for cloning
	tempdata->tpage->refcount++;//increment here
	queue_enqueue(tpsqueue, (void*)tempdata);//add to queue here
	exit_critical_section();//exit danger zone
	return 0;// success
	/* TODO: Phase 2 */
}

