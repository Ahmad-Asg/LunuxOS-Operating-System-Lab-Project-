#include "memory.h"
#include "logger.h"

/* ══════════════════════════════════════════════
   memory_init
   ══════════════════════════════════════════════ */
void memory_init(MemoryManager *mm, int total_ram_mb) {
    memset(mm->bitmap, 0, sizeof(mm->bitmap));
    mm->total_pages = (total_ram_mb * 1024) / PAGE_SIZE_KB;
    if (mm->total_pages > MAX_PAGES) mm->total_pages = MAX_PAGES;
    mm->used_pages  = 0;
    pthread_mutex_init(&mm->lock, NULL);
    LOG_MEM("Memory init: %d pages (%d KB each) for %dMB RAM",
            mm->total_pages, PAGE_SIZE_KB, total_ram_mb);
}

/* ══════════════════════════════════════════════
   memory_alloc  –  first-fit page allocator
   Returns starting page index or -1 on failure
   ══════════════════════════════════════════════ */
int memory_alloc(MemoryManager *mm, int size_kb) {
    int pages_needed = (size_kb + PAGE_SIZE_KB - 1) / PAGE_SIZE_KB;
    pthread_mutex_lock(&mm->lock);
    int start = -1, run = 0;
    for (int i = 0; i < mm->total_pages; i++) {
        if (!mm->bitmap[i]) {
            if (run == 0) start = i;
            if (++run == pages_needed) {
                for (int j = start; j < start + pages_needed; j++)
                    mm->bitmap[j] = 1;
                mm->used_pages += pages_needed;
                pthread_mutex_unlock(&mm->lock);
                LOG_MEM("Alloc: %dKB at page %d (%d pages)", size_kb, start, pages_needed);
                return start;
            }
        } else {
            run = 0; start = -1;
        }
    }
    pthread_mutex_unlock(&mm->lock);
    LOG_WARN("Memory alloc failed: %dKB requested, not enough contiguous pages", size_kb);
    return -1;
}

/* ══════════════════════════════════════════════
   memory_free
   ══════════════════════════════════════════════ */
void memory_free(MemoryManager *mm, int start, int size_kb) {
    if (start < 0) return;
    int pages = (size_kb + PAGE_SIZE_KB - 1) / PAGE_SIZE_KB;
    pthread_mutex_lock(&mm->lock);
    for (int i = start; i < start + pages && i < mm->total_pages; i++) {
        if (mm->bitmap[i]) {
            mm->bitmap[i] = 0;
            mm->used_pages--;
        }
    }
    pthread_mutex_unlock(&mm->lock);
    LOG_MEM("Free: %dKB at page %d", size_kb, start);
}

int memory_get_used_kb(MemoryManager *mm) { return mm->used_pages  * PAGE_SIZE_KB; }
int memory_get_free_kb(MemoryManager *mm) {
    return (mm->total_pages - mm->used_pages) * PAGE_SIZE_KB;
}

/* ══════════════════════════════════════════════
   memory_print_status  –  bar chart
   ══════════════════════════════════════════════ */
void memory_print_status(MemoryManager *mm) {
    pthread_mutex_lock(&mm->lock);
    int used  = mm->used_pages;
    int total = mm->total_pages;
    pthread_mutex_unlock(&mm->lock);

    int pct    = total ? (used * 100) / total : 0;
    int bar_w  = 40;
    int filled = (pct * bar_w) / 100;

    printf("\n  Memory (Paging Simulation)\n");
    printf("  Pages used: %d / %d  (%d KB used of %d KB)\n",
           used, total, used * PAGE_SIZE_KB, total * PAGE_SIZE_KB);
    printf("  [");
    for (int i = 0; i < bar_w; i++)
        printf("%s", i < filled ? "█" : "░");
    printf("] %d%%\n\n", pct);

    /* Page map – first 80 pages */
    printf("  Page map (first 80): ");
    int limit = total < 80 ? total : 80;
    pthread_mutex_lock(&mm->lock);
    for (int i = 0; i < limit; i++)
        printf("%c", mm->bitmap[i] ? '#' : '.');
    pthread_mutex_unlock(&mm->lock);
    if (total > 80) printf("...");
    printf("\n  (# = used, . = free)\n\n");
}
