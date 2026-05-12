#ifndef DEADLOCK_H
#define DEADLOCK_H

#include "lunuxos.h"

#define MAX_RESOURCES 10

typedef struct {
    int allocation[MAX_PROCESSES][MAX_RESOURCES];
    int request[MAX_PROCESSES][MAX_RESOURCES];
    int available[MAX_RESOURCES];
    int num_processes;
    int num_resources;
} DeadlockState;

void deadlock_init(DeadlockState *ds, int procs, int res);
int  deadlock_detect(DeadlockState *ds);         /* returns 1 if deadlock found */
void deadlock_print_state(DeadlockState *ds);
void deadlock_run_check(SystemState *sys_state); /* integrated kernel check     */

#endif /* DEADLOCK_H */
