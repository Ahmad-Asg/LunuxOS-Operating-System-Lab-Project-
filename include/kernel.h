#ifndef KERNEL_H
#define KERNEL_H

#include "lunuxos.h"

/* ── Lifecycle ── */
SystemState *kernel_init(int ram_mb, int storage_mb, int cores);
void         kernel_shutdown(SystemState *state);

/* ── Process management ── */
void kernel_terminate_process(SystemState *state, pid_t pid);
void kernel_kill_process(SystemState *state, pid_t pid);
void kernel_minimize_process(SystemState *state, pid_t pid);
void kernel_resume_process(SystemState *state, pid_t pid);
void kernel_list_processes(SystemState *state);
int  kernel_find_slot(SystemState *state, pid_t pid);

/* ── Resource management ── */
int  kernel_alloc_resources(SystemState *state, int ram_mb, int storage_mb);
void kernel_free_resources(SystemState *state, int ram_mb, int storage_mb);
void kernel_print_resources(SystemState *state);

#endif /* KERNEL_H */
