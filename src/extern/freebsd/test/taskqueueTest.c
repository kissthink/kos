#include "sys/param.h"
#include "sys/systm.h"
#include "sys/kernel.h"
#include "sys/malloc.h"
#include "sys/queue.h"
#include "sys/taskqueue.h"

void func1(void *context, int pending) {
  printf("func1 ran!\n");
}

void func2(void *context, int pending) {
  printf("func2 ran!\n");
}

void taskqueueTest1() {
  printf("\nrunning taskqueueTest1...\n");
  struct task *t = malloc(sizeof(struct task), 0, M_WAITOK);
  TASK_INIT(t, 0, func1, NULL);
  taskqueue_enqueue(taskqueue_swi, t);
  taskqueue_drain(taskqueue_swi, t);
  printf("taskqueueTest1 done!\n");
  free(t, 0);
}

void taskqueueTest2() {
  printf("\nrunning taskqueueTest2...\n");
  struct task *t = malloc(sizeof(struct task), 0, M_WAITOK);
  TASK_INIT(t, 0, func2, NULL);
  taskqueue_enqueue(taskqueue_swi, t);
  taskqueue_drain(taskqueue_swi, t);
  printf("taskqueueTest2 done!\n");
  free(t, 0);
}

//SYSINIT(taskqueueTest1, SI_SUB_RUN_SCHEDULER, SI_ORDER_FIRST, taskqueueTest1, NULL);
//SYSINIT(taskqueueTest2, SI_SUB_RUN_SCHEDULER, SI_ORDER_FIRST, taskqueueTest2, NULL);
