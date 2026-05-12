#ifndef TASK_COMMON_H
#define TASK_COMMON_H

#include "lunuxos.h"
#include "logger.h"

/* ══════════════════════════════════════════════
   Per-task global state
   ══════════════════════════════════════════════ */
static SystemState *g_state    = NULL;
static int          g_shm_fd   = -1;
static int          g_slot     = -1;   /* index in proc_table */
static int          g_ram_mb   = 0;
static int          g_stor_mb  = 0;

/* ── Open shared memory (call once at task start) ── */
static inline SystemState *task_open_shm(void) {
    g_shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (g_shm_fd < 0) return NULL;  /* kernel not running – standalone mode */
    g_state = (SystemState *)mmap(NULL, sizeof(SystemState),
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED, g_shm_fd, 0);
    if (g_state == MAP_FAILED) { g_state = NULL; }
    return g_state;
}

/* ── Request resources from kernel (returns 1 = granted, 0 = denied) ── */
static inline int task_request_resources(int ram_mb, int storage_mb) {
    if (!g_state) return 1;   /* standalone: always allow */
    pthread_mutex_lock(&g_state->resource_lock);
    int ok = 0;
    if ((g_state->used_ram_mb     + ram_mb    ) <= g_state->total_ram_mb     &&
        (g_state->used_storage_mb + storage_mb) <= g_state->total_storage_mb) {
        g_state->used_ram_mb     += ram_mb;
        g_state->used_storage_mb += storage_mb;
        ok = 1;
    }
    pthread_mutex_unlock(&g_state->resource_lock);
    return ok;
}

/* ── Register this task in the process table ── */
static inline void task_register(const char *name, int ram_mb, int storage_mb,
                                  int priority, QueueLevel queue) {
    g_ram_mb  = ram_mb;
    g_stor_mb = storage_mb;
    if (!g_state) return;
    pthread_mutex_lock(&g_state->table_lock);
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!g_state->proc_table[i].valid) {
            PCB *p         = &g_state->proc_table[i];
            p->pid         = getpid();
            strncpy(p->name, name, MAX_NAME_LEN - 1);
            p->name[MAX_NAME_LEN-1] = '\0';
            p->state       = PROC_RUNNING;
            p->priority    = priority;
            p->queue       = queue;
            p->ram_mb      = ram_mb;
            p->storage_mb  = storage_mb;
            p->valid       = 1;
            p->created_at  = time(NULL);
            p->burst_time  = 20;
            p->remaining_time = 20;
            p->wait_time   = 0;
            p->age         = 0;
            g_slot         = i;
            g_state->proc_count++;
            g_state->total_created++;
            break;
        }
    }
    pthread_mutex_unlock(&g_state->table_lock);
    LOG_INFO("Task registered: %s PID=%d RAM=%dMB", name, (int)getpid(), ram_mb);
}

/* ── Deregister and release resources ── */
static inline void task_deregister(void) {
    if (g_state) {
        if (g_slot >= 0) {
            pthread_mutex_lock(&g_state->table_lock);
            g_state->proc_table[g_slot].valid = 0;
            g_state->proc_table[g_slot].state = PROC_TERMINATED;
            if (g_state->proc_count > 0) g_state->proc_count--;
            g_state->total_terminated++;
            pthread_mutex_unlock(&g_state->table_lock);

            pthread_mutex_lock(&g_state->resource_lock);
            g_state->used_ram_mb     -= g_ram_mb;
            g_state->used_storage_mb -= g_stor_mb;
            if (g_state->used_ram_mb     < 0) g_state->used_ram_mb     = 0;
            if (g_state->used_storage_mb < 0) g_state->used_storage_mb = 0;
            pthread_mutex_unlock(&g_state->resource_lock);
        }
        munmap(g_state, sizeof(SystemState));
        g_state = NULL;
    }
    if (g_shm_fd >= 0) { close(g_shm_fd); g_shm_fd = -1; }
    LOG_INFO("Task deregistered PID=%d", (int)getpid());
}

/* ── Convenience macro for full task startup sequence ── */
#define TASK_START(task_name, ram, stor, prio, q)               \
    do {                                                         \
        task_open_shm();                                         \
        if (!task_request_resources((ram), (stor))) {            \
            fprintf(stderr,                                      \
                "\n[LunuxOS] ERROR: Insufficient resources for " \
                task_name "!\n"                                  \
                "  Requested: %dMB RAM, %dMB Storage\n"         \
                "  Available: check RAM monitor\n"               \
                "Press Enter to close...\n", (ram), (stor));     \
            getchar();                                           \
            exit(1);                                             \
        }                                                        \
        task_register((task_name), (ram), (stor), (prio), (q)); \
    } while(0)

#define TASK_END()  task_deregister()

#endif /* TASK_COMMON_H */
