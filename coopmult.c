#include "coopmult.h"

#include "queue.h"

#include <stdbool.h>
#include <stdlib.h>

#define STACK_SIZE 0x4000 // 16KiB

typedef struct {
  void (*entry)(void*);
  void* args;
} Task;

QUEUE(Tasks, tasks, Task)

typedef struct {
  char *stack, *continuation;
} Context;

QUEUE(Contexts, contexts, Context)

QUEUE(Stacks, stacks, char*)

static Tasks queuedTasks;
static Contexts suspendedTasks;
static Stacks cachedStacks;

static char* entryContinuation;
static char* activeStack;

extern void coopmult_suspend(void);
extern void coopmult_continue(char* continuation);
void coopmult_schedule(char* continuation);
static void coopmult_begin_task(Task task);
static void coopmult_end_task(void);

void coopmult_add_task(void (*entry)(void*), void* args) {
  Task task = {.entry = entry, .args = args};
  tasksPush(&queuedTasks, task);
}

void coopmult_run(void) {
  entryContinuation = NULL;
  coopmult_suspend();
  while (!stacksIsEmpty(cachedStacks))
    free(stacksPop(&cachedStacks));
}

void coopmult_sleep(void) {
  coopmult_suspend();
}

static void coopmult_begin_task(Task task) {
  activeStack = stacksIsEmpty(cachedStacks) ? malloc(STACK_SIZE) : stacksPop(&cachedStacks);
#if defined(__x86_64__)
  __asm__ volatile(
      "movq %0, %%rsp\n"
      "pushq %1\n"
      "jmp *%3\n"
      :
      : "r"(activeStack + STACK_SIZE), "r"(coopmult_end_task), "D"(task.args), "r"(task.entry));
#elif defined(__i386__)
  __asm__ volatile(
      "movl %0, %%esp\n"
      "pushl %1\n"
      "pushl %2\n"
      "jmp *%3\n"
      :
      : "r"(activeStack + STACK_SIZE - 12), "r"(task.args), "r"(coopmult_end_task), "r"(task.entry));
#else
  #error Unsupported platform
#endif
}

static void coopmult_end_task(void) {
  stacksPush(&cachedStacks, activeStack);
  coopmult_schedule(NULL);
}

void coopmult_schedule(char* continuation) {
  if (entryContinuation == NULL)
    entryContinuation = continuation;
  else if (continuation != NULL) {
    Context co = {.stack = activeStack, .continuation = continuation};
    contextsPush(&suspendedTasks, co);
  }

  if (!tasksIsEmpty(queuedTasks))
    coopmult_begin_task(tasksPop(&queuedTasks));
  else if (!contextsIsEmpty(suspendedTasks)) {
    Context ctx = contextsPop(&suspendedTasks);
    activeStack = ctx.stack;
    coopmult_continue(ctx.continuation);
  } else
    coopmult_continue(entryContinuation);
}

#if defined(__x86_64__)
__asm__(
    "coopmult_suspend:\n"
    "pushq %rbp\n"
    "pushq %rbx\n"
    "pushq %r12\n"
    "pushq %r13\n"
    "pushq %r14\n"
    "pushq %r15\n"
    "sub $8, %rsp\n"
    "lea -8(%rsp), %rdi\n"
    "call coopmult_schedule\n"
    "add $8, %rsp\n"
    "popq %r15\n"
    "popq %r14\n"
    "popq %r13\n"
    "popq %r12\n"
    "popq %rbx\n"
    "popq %rbp\n"
    "ret\n"
    "coopmult_continue:\n"
    "mov %rdi, %rsp\n"
    "ret\n");
#elif defined(__i386__)
__asm__(
    "coopmult_suspend:\n"
    "pushl %ebp\n"
    "pushl %ebx\n"
    "pushl %esi\n"
    "pushl %edi\n"
    "subl $8, %esp\n"
    "leal -8(%esp), %eax\n"
    "pushl %eax\n"
    "call coopmult_schedule\n"
    "add $12, %esp\n"
    "popl %edi\n"
    "popl %esi\n"
    "popl %ebx\n"
    "popl %ebp\n"
    "ret\n"
    "coopmult_continue:\n"
    "movl 4(%esp), %esp\n"
    "ret\n");
#else
  #error Unsupported platform
#endif
