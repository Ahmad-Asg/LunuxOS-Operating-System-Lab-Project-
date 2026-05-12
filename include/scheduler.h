#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "lunuxos.h"

void scheduler_start(SystemState *state);
void scheduler_stop(void);
void scheduler_tick(SystemState *state);
void scheduler_print_queues(SystemState *state);

#endif /* SCHEDULER_H */
