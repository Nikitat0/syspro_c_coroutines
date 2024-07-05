#ifndef COOPMULT_H
#define COOPMULT_H

void coopmult_add_task(void (*entry)(void*), void* args);
void coopmult_run(void);
void coopmult_sleep(void);

#endif
