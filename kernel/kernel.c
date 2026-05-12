#include "kernel.h"
#include "logger.h"
#include <sys/mman.h>

/* ══════════════════════════════════════════════
   kernel_init  –  create & initialise shared memory
   ══════════════════════════════════════════════ */
SystemState *kernel_init(int ram_mb, int storage_mb, int cores) {
    /* Remove stale segment from previous run */
    shm_unlink(SHM_NAME);

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("[LunuxOS] shm_open failed");
        return NULL;
    }
    if (ftruncate(fd, sizeof(SystemState)) < 0) {
        perror("[LunuxOS] ftruncate failed");
        close(fd);
        return NULL;
    }
    SystemState *state = (SystemState *)mmap(NULL, sizeof(SystemState),
                                              PROT_READ | PROT_WRITE,
                                              MAP_SHARED, fd, 0);
    close(fd);
    if (state == MAP_FAILED) {
        perror("[LunuxOS] mmap failed");
        return NULL;
    }

    memset(state, 0, sizeof(SystemState));

    /* ── Resources ── */
    state->total_ram_mb     = ram_mb;
    state->total_storage_mb = storage_mb * 1024; /* GB → MB */
    state->total_cores      = cores;
    state->used_ram_mb      = 0;
    state->used_storage_mb  = 0;
    state->used_cores       = 0;

    /* ── Flags ── */
    state->kernel_running      = 1;
    state->kernel_mode         = 0;
    state->shutdown_requested  = 0;

    /* ── PTHREAD_PROCESS_SHARED mutexes ── */
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&state->table_lock,    &mattr);
    pthread_mutex_init(&state->resource_lock, &mattr);
    pthread_mutexattr_destroy(&mattr);

    LOG_INFO("Kernel initialised: RAM=%dMB  Storage=%dGB  Cores=%d",
             ram_mb, storage_mb, cores);
    return state;
}

/* ══════════════════════════════════════════════
   kernel_shutdown
   ══════════════════════════════════════════════ */
void kernel_shutdown(SystemState *state) {
    if (!state) return;
    state->kernel_running = 0;
    pthread_mutex_destroy(&state->table_lock);
    pthread_mutex_destroy(&state->resource_lock);
    munmap(state, sizeof(SystemState));
    shm_unlink(SHM_NAME);
    LOG_INFO("Kernel shutdown complete");
}

/* ══════════════════════════════════════════════
   Helper: find slot by PID
   ══════════════════════════════════════════════ */
int kernel_find_slot(SystemState *state, pid_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (state->proc_table[i].valid && state->proc_table[i].pid == pid)
            return i;
    return -1;
}

/* ══════════════════════════════════════════════
   Process management
   ══════════════════════════════════════════════ */
void kernel_terminate_process(SystemState *state, pid_t pid) {
    pthread_mutex_lock(&state->table_lock);
    int idx = kernel_find_slot(state, pid);
    if (idx >= 0) {
        state->proc_table[idx].state = PROC_TERMINATED;
        state->proc_table[idx].valid = 0;
        if (state->proc_count > 0) state->proc_count--;
        state->total_terminated++;
        int r = state->proc_table[idx].ram_mb;
        int s = state->proc_table[idx].storage_mb;
        pthread_mutex_unlock(&state->table_lock);

        pthread_mutex_lock(&state->resource_lock);
        state->used_ram_mb     -= r;
        state->used_storage_mb -= s;
        if (state->used_ram_mb     < 0) state->used_ram_mb     = 0;
        if (state->used_storage_mb < 0) state->used_storage_mb = 0;
        pthread_mutex_unlock(&state->resource_lock);

        kill(pid, SIGTERM);
        LOG_INFO("Kernel: terminated PID=%d", (int)pid);
    } else {
        pthread_mutex_unlock(&state->table_lock);
        printf("[LunuxOS] PID %d not found in process table.\n", (int)pid);
    }
}

void kernel_kill_process(SystemState *state, pid_t pid) {
    pthread_mutex_lock(&state->table_lock);
    int idx = kernel_find_slot(state, pid);
    if (idx >= 0) {
        int r = state->proc_table[idx].ram_mb;
        int s = state->proc_table[idx].storage_mb;
        state->proc_table[idx].valid = 0;
        state->proc_table[idx].state = PROC_TERMINATED;
        if (state->proc_count > 0) state->proc_count--;
        state->total_terminated++;
        pthread_mutex_unlock(&state->table_lock);

        pthread_mutex_lock(&state->resource_lock);
        state->used_ram_mb     -= r;
        state->used_storage_mb -= s;
        if (state->used_ram_mb     < 0) state->used_ram_mb     = 0;
        if (state->used_storage_mb < 0) state->used_storage_mb = 0;
        pthread_mutex_unlock(&state->resource_lock);

        kill(pid, SIGKILL);
        LOG_WARN("Kernel: force-killed PID=%d", (int)pid);
    } else {
        pthread_mutex_unlock(&state->table_lock);
        printf("[LunuxOS] PID %d not found.\n", (int)pid);
    }
}

void kernel_minimize_process(SystemState *state, pid_t pid) {
    int idx = kernel_find_slot(state, pid);
    if (idx >= 0) {
        state->proc_table[idx].state = PROC_WAITING;
        kill(pid, SIGSTOP);
        LOG_INFO("Kernel: minimized PID=%d", (int)pid);
        printf("[LunuxOS] Process %d minimized (SIGSTOP sent).\n", (int)pid);
    } else {
        printf("[LunuxOS] PID %d not found.\n", (int)pid);
    }
}

void kernel_resume_process(SystemState *state, pid_t pid) {
    int idx = kernel_find_slot(state, pid);
    if (idx >= 0) {
        state->proc_table[idx].state = PROC_RUNNING;
        kill(pid, SIGCONT);
        LOG_INFO("Kernel: resumed PID=%d", (int)pid);
        printf("[LunuxOS] Process %d resumed (SIGCONT sent).\n", (int)pid);
    } else {
        printf("[LunuxOS] PID %d not found.\n", (int)pid);
    }
}

void kernel_list_processes(SystemState *state) {
    const char *states[] = {"New","Ready","Running","Waiting","Blocked","Terminated"};
    const char *queues[] = {"","RR","FCFS","Priority"};
    printf("\n%-6s %-20s %-10s %-5s %-8s %-8s %-6s\n",
           "PID","Name","State","Prio","Queue","RAM(MB)","Age");
    printf("%-6s %-20s %-10s %-5s %-8s %-8s %-6s\n",
           "------","--------------------","----------","-----","--------","--------","------");
    pthread_mutex_lock(&state->table_lock);
    int found = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (state->proc_table[i].valid) {
            PCB *p = &state->proc_table[i];
            printf("%-6d %-20s %-10s %-5d %-8s %-8d %-6d\n",
                   (int)p->pid, p->name,
                   states[p->state < 6 ? p->state : 0],
                   p->priority,
                   queues[p->queue < 4 ? p->queue : 0],
                   p->ram_mb,
                   p->age);
            found++;
        }
    }
    pthread_mutex_unlock(&state->table_lock);
    if (!found) printf("  (no active processes)\n");
    printf("\n");
}

/* ══════════════════════════════════════════════
   Resource management
   ══════════════════════════════════════════════ */
int kernel_alloc_resources(SystemState *state, int ram_mb, int storage_mb) {
    pthread_mutex_lock(&state->resource_lock);
    int ok = 0;
    if ((state->used_ram_mb     + ram_mb    ) <= state->total_ram_mb     &&
        (state->used_storage_mb + storage_mb) <= state->total_storage_mb) {
        state->used_ram_mb     += ram_mb;
        state->used_storage_mb += storage_mb;
        ok = 1;
    }
    pthread_mutex_unlock(&state->resource_lock);
    if (ok) LOG_MEM("Allocated %dMB RAM, %dMB storage", ram_mb, storage_mb);
    else    LOG_WARN("Resource allocation denied: RAM=%dMB requested, %d/%d used",
                     ram_mb, state->used_ram_mb, state->total_ram_mb);
    return ok;
}

void kernel_free_resources(SystemState *state, int ram_mb, int storage_mb) {
    pthread_mutex_lock(&state->resource_lock);
    state->used_ram_mb     -= ram_mb;
    state->used_storage_mb -= storage_mb;
    if (state->used_ram_mb     < 0) state->used_ram_mb     = 0;
    if (state->used_storage_mb < 0) state->used_storage_mb = 0;
    pthread_mutex_unlock(&state->resource_lock);
    LOG_MEM("Freed %dMB RAM, %dMB storage", ram_mb, storage_mb);
}

void kernel_print_resources(SystemState *state) {
    printf("  RAM    : %4d / %4d MB  (free: %4d MB)\n",
           state->used_ram_mb, state->total_ram_mb,
           state->total_ram_mb - state->used_ram_mb);
    printf("  Storage: %4d / %4d MB  (free: %4d MB)\n",
           state->used_storage_mb, state->total_storage_mb,
           state->total_storage_mb - state->used_storage_mb);
    printf("  Cores  : %4d / %4d\n",
           state->used_cores, state->total_cores);
}
