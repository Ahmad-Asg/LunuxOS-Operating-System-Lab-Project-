#include "deadlock.h"
#include "logger.h"

/* deadlock_init */
void deadlock_init(DeadlockState *ds, int procs, int res) {
    memset(ds, 0, sizeof(DeadlockState));
    ds->num_processes = procs  < MAX_PROCESSES ? procs  : MAX_PROCESSES;
    ds->num_resources = res    < MAX_RESOURCES ? res    : MAX_RESOURCES;
}

/* deadlock_detect  –  Banker's safety algorithm
   Returns 1 if deadlock (unsafe state) detected*/
int deadlock_detect(DeadlockState *ds) {
    int  n = ds->num_processes;
    int  m = ds->num_resources;
    int  work[MAX_RESOURCES];
    int  finish[MAX_PROCESSES];

    /* work = available */
    for (int j = 0; j < m; j++) work[j] = ds->available[j];
    memset(finish, 0, sizeof(finish));

    /* Try to find a safe sequence */
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = 0; i < n; i++) {
            if (finish[i]) continue;
            /* Check if request[i] <= work */
            int can = 1;
            for (int j = 0; j < m; j++) {
                if (ds->request[i][j] > work[j]) { can = 0; break; }
            }
            if (can) {
                /* Simulate finishing process i */
                for (int j = 0; j < m; j++)
                    work[j] += ds->allocation[i][j];
                finish[i] = 1;
                changed   = 1;
            }
        }
    }

    /* Any unfinished process = deadlock */
    for (int i = 0; i < n; i++)
        if (!finish[i]) return 1;
    return 0;
}

/*   deadlock_print_state*/
void deadlock_print_state(DeadlockState *ds) {
    int n = ds->num_processes;
    int m = ds->num_resources;

    printf("\n  ── Allocation Matrix ──\n");
    printf("  P\\R  ");
    for (int j = 0; j < m; j++) printf("R%d  ", j);
    printf("\n");
    for (int i = 0; i < n; i++) {
        printf("  P%d   ", i);
        for (int j = 0; j < m; j++)
            printf("%-4d", ds->allocation[i][j]);
        printf("\n");
    }

    printf("\n  ── Request Matrix ──\n");
    printf("  P\\R  ");
    for (int j = 0; j < m; j++) printf("R%d  ", j);
    printf("\n");
    for (int i = 0; i < n; i++) {
        printf("  P%d   ", i);
        for (int j = 0; j < m; j++)
            printf("%-4d", ds->request[i][j]);
        printf("\n");
    }

    printf("\n  ── Available ──\n  ");
    for (int j = 0; j < m; j++) printf("R%d:%-3d  ", j, ds->available[j]);
    printf("\n");
}

/* 
   deadlock_run_check  –  builds a state from
   the live process table and runs detection
    */
void deadlock_run_check(SystemState *sys_state) {
    printf("\n  [Deadlock] Scanning process table...\n");

    DeadlockState ds;
    int n = 0, m = 2; /* 2 resources: RAM slots, storage slots */
    deadlock_init(&ds, 0, m);

    pthread_mutex_lock(&sys_state->table_lock);
    for (int i = 0; i < MAX_PROCESSES && n < MAX_PROCESSES; i++) {
        PCB *p = &sys_state->proc_table[i];
        if (!p->valid) continue;
        /* Allocation = what it currently holds */
        ds.allocation[n][0] = p->ram_mb / 32;     /* normalise */
        ds.allocation[n][1] = p->storage_mb / 32;
        /* Request = simulated (same as allocation for running tasks) */
        ds.request[n][0] = (p->state == PROC_WAITING || p->state == PROC_BLOCKED)
                           ? (p->ram_mb / 32) : 0;
        ds.request[n][1] = 0;
        n++;
    }
    pthread_mutex_unlock(&sys_state->table_lock);

    ds.num_processes  = n;
    ds.num_resources  = m;
    ds.available[0]   = (sys_state->total_ram_mb - sys_state->used_ram_mb) / 32;
    ds.available[1]   = (sys_state->total_storage_mb - sys_state->used_storage_mb) / 32;

    deadlock_print_state(&ds);

    if (n == 0) {
        printf("  [Deadlock] No processes to check.\n\n");
        return;
    }

    int result = deadlock_detect(&ds);
    if (result) {
        printf("\n  ╔══════════════════════════════════════════╗\n");
        printf("  ║  Deadlock detected among processes !!!   ║\n");
        printf("  ╚══════════════════════════════════════════╝\n\n");
        sys_state->deadlock_count++;
        LOG_DEAD("Deadlock detected among %d processes", n);
    } else {
        printf("\n  [Deadlock] System is in a SAFE state. No deadlock.\n\n");
        LOG_INFO("Deadlock check: safe state");
    }
}
