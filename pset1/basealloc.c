#define M61_DISABLE 1
#include "m61.h"


// This file contains a base memory allocator guaranteed not to
// overwrite freed allocations. No need to understand it.


typedef struct base_allocation {
    void* ptr;
    size_t sz;
} base_allocation;

static base_allocation* allocs;
static size_t nallocs;
static size_t alloc_capacity;
static size_t* frees;
static size_t nfrees;
static size_t free_capacity;
static int disabled;

static unsigned alloc_random(void) {
    static uint64_t x = 8973443640547502487ULL;
    x = x * 6364136223846793005ULL + 1ULL;
    return x >> 32;
}

static void base_alloc_atexit(void);

void* base_malloc(size_t sz) {
    if (disabled)
        return malloc(sz);

    static int base_alloc_atexit_installed = 0;
    if (!base_alloc_atexit_installed) {
        atexit(base_alloc_atexit);
        base_alloc_atexit_installed = 1;
    }

    unsigned r = alloc_random();
    // try to use a previously-freed block 75% of the time
    if (r % 4 != 0)
        for (unsigned try = 0; try < 10 && try < nfrees; ++try) {
            size_t freenum = alloc_random() % nfrees;
            size_t i = frees[freenum];
            if (allocs[i].sz >= sz) {
                frees[freenum] = frees[nfrees - 1];
                --nfrees;
                return allocs[i].ptr;
            }
        }
    // need a new allocation
    if (nallocs == alloc_capacity) {
        alloc_capacity = alloc_capacity ? alloc_capacity * 2 : 64;
        allocs = realloc(allocs, alloc_capacity * sizeof(base_allocation));
        if (!allocs)
            abort();
    }
    void* ptr = malloc(sz);
    if (ptr) {
        allocs[nallocs].ptr = ptr;
        allocs[nallocs].sz = sz;
        ++nallocs;
    }
    return ptr;
}

void base_free(void* ptr) {
    if (disabled || !ptr) {
        free(ptr);
        return;
    }
    if (nfrees == free_capacity) {
        free_capacity = free_capacity ? free_capacity * 2 : 64;
        frees = realloc(frees, free_capacity * sizeof(size_t));
        if (!frees)
            abort();
    }
    for (size_t i = 0; i < nallocs; ++i)
        if (allocs[i].ptr == ptr) {
            frees[nfrees] = i;
            ++nfrees;
            return;
        }
    // if we get here, invalid free; silently ignore it
}

void base_disablealloc(int d) {
    disabled = d;
}

static void base_alloc_atexit(void) {
    for (size_t i = 0; i < nfrees; ++i)
        free(allocs[frees[i]].ptr);
    free(frees);
    free(allocs);
}
