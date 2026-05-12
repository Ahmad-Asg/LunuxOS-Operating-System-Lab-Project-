#ifndef MEMORY_H
#define MEMORY_H

#include "lunuxos.h"

#define PAGE_SIZE_KB  4
#define MAX_PAGES    16384   /* 64 MB at 4KB pages – simulation limit */

typedef struct {
    int total_pages;
    int used_pages;
    char bitmap[MAX_PAGES]; /* 0=free 1=used */
    pthread_mutex_t lock;
} MemoryManager;

void memory_init(MemoryManager *mm, int total_ram_mb);
int  memory_alloc(MemoryManager *mm, int size_kb);    /* returns start page or -1 */
void memory_free(MemoryManager *mm, int start, int size_kb);
void memory_print_status(MemoryManager *mm);
int  memory_get_used_kb(MemoryManager *mm);
int  memory_get_free_kb(MemoryManager *mm);

#endif /* MEMORY_H */
