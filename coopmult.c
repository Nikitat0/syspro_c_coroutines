#include "coopmult.h"

#include "queue.h"

#include <setjmp.h>
#include <stdbool.h>
#include <stdlib.h>

#define STACK_SIZE 0x4000 // 16KiB

typedef struct {
  void (*entry)(void*);
  void* args;
} Task;

QUEUE(Tasks, tasks, Task)

typedef struct {
  void* stack;
  jmp_buf env;
} Coroutine;

QUEUE(Coroutines, coroutines, Coroutine)

QUEUE(Stacks, stacks, char*)

static Tasks queuedTasks;
static Stacks cachedStacks;
static Coroutines coroutines;

static jmp_buf entrypoint;
static char* activeStack;

static void coopmult_begin_task(Task task);
static void coopmult_end_task(void);
static void coopmult_continue(void);

void coopmult_add_task(void (*entry)(void*), void* args) {
  Task task = {.entry = entry, .args = args};
  tasksPush(&queuedTasks, task);
}

void coopmult_run(void) {
  if (!setjmp(entrypoint))
    coopmult_continue();
  while (!stacksIsEmpty(cachedStacks))
    free(stacksPop(&cachedStacks));
}

void coopmult_sleep(void) {
  Coroutine co = {.stack = activeStack};
  if (setjmp(co.env))
    return;
  coroutinesPush(&coroutines, co);
  coopmult_continue();
}

static void coopmult_begin_task(Task task) {
  activeStack = stacksIsEmpty(cachedStacks) ? malloc(STACK_SIZE) : stacksPop(&cachedStacks);
#if defined(__x86_64__)
  __asm__ volatile(
      "movq %0, %%rsp\n"
      "pushq %1\n"
      "jmp *%3"
      :
      : "r"(activeStack + STACK_SIZE), "r"(coopmult_end_task), "D"(task.args), "r"(task.entry));
#elif defined(__i386__)
  __asm__ volatile(
      "movl %0, %%esp\n"
      "pushl %1\n"
      "pushl %2\n"
      "jmp *%3"
      :
      : "r"(activeStack + STACK_SIZE), "r"(task.args), "r"(coopmult_end_task), "r"(task.entry));
#else
  #error Unsupported platform
#endif
}

static void coopmult_end_task(void) {
  stacksPush(&cachedStacks, activeStack);
  coopmult_continue();
}

static void coopmult_continue(void) {
  if (!tasksIsEmpty(queuedTasks))
    coopmult_begin_task(tasksPop(&queuedTasks));
  else if (!coroutinesIsEmpty(coroutines)) {
    Coroutine co = coroutinesPop(&coroutines);
    activeStack = co.stack;
    longjmp(co.env, true);
  } else
    longjmp(entrypoint, true);
}
