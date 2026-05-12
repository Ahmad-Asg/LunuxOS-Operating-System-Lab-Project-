#include "scheduler.h"
#include "kernel.h"
#include "logger.h"

static pthread_t      sched_thread;
static volatile int   sched_running = 0;
static SystemState   *sched_state   = NULL;

/* ══════════════════════════════════════════════
   Aging: promote long-waiting lower-queue procs
   ══════════════════════════════════════════════ */
static void apply_aging(SystemState *state) {
    pthread_mutex_lock(&state->table_lock);
    for (int i = 0; i < MAX_PROCESSES; i++) {
        PCB *p = &state->proc_table[i];
        if (!p->valid) continue;
        if (p->state == PROC_READY) {
            p->age++;
            p->wait_time++;
            /* Promote from Q3→Q2 after 20 ticks, Q2→Q1 after 30 */
            if (p->queue == QUEUE_PRIO && p->age >= 20) {
                p->queue = QUEUE_FCFS;
                p->age   = 0;
                LOG_SCHED("Aging: %s promoted Q3→Q2", p->name);
            } else if (p->queue == QUEUE_FCFS && p->age >= 30) {
                p->queue = QUEUE_RR;
                p->age   = 0;
                LOG_SCHED("Aging: %s promoted Q2→Q1", p->name);
            }
        }
    }
    pthread_mutex_unlock(&state->table_lock);
}

/* ══════════════════════════════════════════════
   Round Robin (Queue 1) – interactive
   ══════════════════════════════════════════════ */
static void schedule_rr(SystemState *state) {
    static int rr_idx = 0;
    pthread_mutex_lock(&state->table_lock);
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        int idx = (rr_idx + i) % MAX_PROCESSES;
        PCB *p  = &state->proc_table[idx];
        if (!p->valid || p->queue != QUEUE_RR) continue;
        if (p->state == PROC_RUNNING || p->state == PROC_READY) {
            p->remaining_time -= TIME_QUANTUM;
            if (p->remaining_time <= 0) p->remaining_time = TIME_QUANTUM;
            rr_idx = (idx + 1) % MAX_PROCESSES;
            count++;
            break;
        }
    }
    (void)count;
    pthread_mutex_unlock(&state->table_lock);
}

/* ══════════════════════════════════════════════
   FCFS (Queue 2) – batch
   ══════════════════════════════════════════════ */
static void schedule_fcfs(SystemState *state) {
    pthread_mutex_lock(&state->table_lock);
    time_t earliest = 0;
    int    found    = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        PCB *p = &state->proc_table[i];
        if (!p->valid || p->queue != QUEUE_FCFS) continue;
        if (p->state != PROC_RUNNING && p->state != PROC_READY) continue;
        if (found < 0 || p->created_at < earliest) {
            earliest = p->created_at;
            found    = i;
        }
    }
    if (found >= 0) {
        state->proc_table[found].remaining_time -= 1;
        if (state->proc_table[found].remaining_time < 0)
            state->proc_table[found].remaining_time = 0;
    }
    pthread_mutex_unlock(&state->table_lock);
}

/* ══════════════════════════════════════════════
   Priority Scheduling (Queue 3) – background
   ══════════════════════════════════════════════ */
static void schedule_priority(SystemState *state) {
    pthread_mutex_lock(&state->table_lock);
    int highest_prio = -1;
    int found        = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        PCB *p = &state->proc_table[i];
        if (!p->valid || p->queue != QUEUE_PRIO) continue;
        if (p->state != PROC_RUNNING && p->state != PROC_READY) continue;
        if (p->priority > highest_prio) {
            highest_prio = p->priority;
            found        = i;
        }
    }
    if (found >= 0) {
        state->proc_table[found].remaining_time -= 1;
        if (state->proc_table[found].remaining_time < 0)
            state->proc_table[found].remaining_time = 0;
    }
    pthread_mutex_unlock(&state->table_lock);
}

/* ══════════════════════════════════════════════
   Update queue snapshots for display
   ══════════════════════════════════════════════ */
static void update_queues(SystemState *state) {
    pthread_mutex_lock(&state->table_lock);
    state->ready_q_count   = 0;
    state->wait_q_count    = 0;
    state->blocked_q_count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        PCB *p = &state->proc_table[i];
        if (!p->valid) continue;
        if (p->state == PROC_READY   && state->ready_q_count   < MAX_PROCESSES)
            state->ready_q[state->ready_q_count++]     = i;
        if (p->state == PROC_WAITING && state->wait_q_count    < MAX_PROCESSES)
            state->wait_q[state->wait_q_count++]       = i;
        if (p->state == PROC_BLOCKED && state->blocked_q_count < MAX_PROCESSES)
            state->blocked_q[state->blocked_q_count++] = i;
    }
    pthread_mutex_unlock(&state->table_lock);
}

/* ══════════════════════════════════════════════
   Main scheduler tick
   ══════════════════════════════════════════════ */
void scheduler_tick(SystemState *state) {
    schedule_rr(state);
    schedule_fcfs(state);
    schedule_priority(state);
    apply_aging(state);
    update_queues(state);
}

/* ══════════════════════════════════════════════
   Scheduler thread
   ══════════════════════════════════════════════ */
static void *scheduler_thread_func(void *arg) {
    SystemState *state = (SystemState *)arg;
    LOG_SCHED("MLQ Scheduler started (RR|FCFS|Priority)");
    while (sched_running && state->kernel_running) {
        scheduler_tick(state);
        sleep(1);
    }
    LOG_SCHED("Scheduler stopped");
    return NULL;
}

void scheduler_start(SystemState *state) {
    sched_state   = state;
    sched_running = 1;
    pthread_create(&sched_thread, NULL, scheduler_thread_func, state);
}

void scheduler_stop(void) {
    sched_running = 0;
    pthread_join(sched_thread, NULL);
}

/* ══════════════════════════════════════════════
   Print queue status
   ══════════════════════════════════════════════ */
void scheduler_print_queues(SystemState *state) {
    const char *states[] = {"New","Ready","Running","Waiting","Blocked","Terminated"};
    printf("\n  ┌── Queue 1 (Round Robin) ───────────────────────┐\n");
    int found = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        PCB *p = &state->proc_table[i];
        if (p->valid && p->queue == QUEUE_RR) {
            printf("  │  PID=%-6d  %-18s  %s\n",
                   (int)p->pid, p->name,
                   states[p->state < 6 ? p->state : 0]);
            found++;
        }
    }
    if (!found) printf("  │  (empty)\n");
    printf("  └───────────────────────────────────────────────┘\n");

    printf("  ┌── Queue 2 (FCFS) ──────────────────────────────┐\n");
    found = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        PCB *p = &state->proc_table[i];
        if (p->valid && p->queue == QUEUE_FCFS) {
            printf("  │  PID=%-6d  %-18s  %s\n",
                   (int)p->pid, p->name,
                   states[p->state < 6 ? p->state : 0]);
            found++;
        }
    }
    if (!found) printf("  │  (empty)\n");
    printf("  └───────────────────────────────────────────────┘\n");

    printf("  ┌── Queue 3 (Priority) ──────────────────────────┐\n");
    found = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        PCB *p = &state->proc_table[i];
        if (p->valid && p->queue == QUEUE_PRIO) {
            printf("  │  PID=%-6d  %-18s  Prio:%-2d  %s\n",
                   (int)p->pid, p->name, p->priority,
                   states[p->state < 6 ? p->state : 0]);
            found++;
        }
    }
    if (!found) printf("  │  (empty)\n");
    printf("  └───────────────────────────────────────────────┘\n\n");
}
