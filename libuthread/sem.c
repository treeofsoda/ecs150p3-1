#include<stddef.h>
#include<stdlib.h>
#include<stdio.h>
#include<stdbool.h>

#include"queue.h"
#include"sem.h"
#include"thread.h"

// internal count
struct semaphore{
  size_t count;
  queue_t waitingQ;
};

sem_t sem_create(size_t count){
  struct semaphore *sem_init ;
  sem_init =(sem_t)malloc(sizeof(struct semaphore));
  if (sem_init == NULL)
    return NULL;
  sem_init->waitingQ = queue_create();
  sem_init->count = count;
  return sem_init;
}

int sem_destory(sem_t sem){
  if (sem == NULL)
    return -1;
  int destroy_status;
  destroy_status = queue_destroy(sem->waitingQ);
  if (destroy_status != 0){
    return -1;
  }
  free(sem);
  return 0;
}

int sem_down(sem_t sem){
  if (sem == NULL){
    return -1;
  }
  pthread_t new = pthread_self();
  pthread_t *new_addr;
  new_addr = (pthread_t*)malloc(sizeof(pthread_t));
  *new_addr = new;
  enter_critical_section();
  if(sem->count == 0){
    queue_enqueue(sem->waitingQ,new_addr);
    thread_block();
  }
  else{
    sem->count --;
  }
  exit_critical_section();
  return 0;
}

int sem_up(sem_t sem){
enter_critical_section();  
if (sem == NULL){
       exit_critical_section();
	return -1;
}
  //pthread_t *new;
  pthread_t *new_addr;
  //new_addr = (pthread_t*)malloc(sizeof(pthread_t));
  //enter_critical_section();
  //new_addr = (void*)&new;
  //sem->count++;
  if (queue_dequeue(sem->waitingQ, (void**)&new_addr) == 0){
    thread_unblock(*new_addr);
    free(new_addr);
  }
  else{
	sem->count ++;
  }
  exit_critical_section();
  return 0;
}

int sem_getvalue(sem_t sem, int *sval){
  enter_critical_section();
	if(sem == NULL || sval == NULL){
	exit_critical_section();
    return -1;
}
  else if (sem->count == 0){
    *sval = queue_length(sem->waitingQ);
    *sval *= -1;
    exit_critical_section();
    return 0;
  }
  else{
    *sval = sem->count;
    exit_critical_section();
    return 0;
  }
}
