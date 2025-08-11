#pragma once

typedef struct _Allocation
{
    void*               ptr;
    u64                 size;
    const char*         file;
    i32                 line;
    struct _Allocation* next;
} Allocation;

extern Allocation* g_allocations;
extern u64         g_totalAllocs;
extern i32         g_allocCount;

static inline void* _DebugMalloc(u64 size, const char* file, i32 line)
{
    void* ptr = malloc(size);
    if (ptr)
    {
        Allocation* alloc = malloc(sizeof(Allocation));
        if (!alloc)
        {
            free(ptr);
            _Log("[ALLOC]", __FILE__, __LINE__, TEXT_COLOR_BRIGHT_RED,
                 "Failed to allocate tracking node for %zu bytes at %s:%d",
                 size, file, line);
            return NULL;
        }

        alloc->ptr  = ptr;
        alloc->size = size;
        alloc->file = file;
        alloc->line = line;
        alloc->next = g_allocations;

        g_allocations = alloc;
        g_totalAllocs += size;
        ++g_allocCount;

        _Log("[ALLOC]", __FILE__, __LINE__, TEXT_COLOR_BRIGHT_BLUE,
             "%zu bytes at %p (%s:%d) - Total: %zu bytes, Count: %d",
             size, ptr, file, line, g_totalAllocs, g_allocCount);
    }
    else
    {
        _Log("[ALLOC]", __FILE__, __LINE__, TEXT_COLOR_BRIGHT_RED,
             "Failed to allocate %zu bytes at %s:%d",
             size, file, line);
    }

    return ptr;
}

static inline void _DebugFree(void* ptr, const char* file, i32 line)
{
    if (!ptr)
    {
        _Log("[FREE]", __FILE__, __LINE__, TEXT_COLOR_BRIGHT_YELLOW,
             "Freeing null pointer at %s:%d", file, line);
        return;
    }

    Allocation** current = &g_allocations;
    while (*current)
    {
        if ((*current)->ptr == ptr)
        {
            Allocation* toRemove = *current;
            g_totalAllocs -= toRemove->size;
            --g_allocCount;

            _Log("[FREE]", __FILE__, __LINE__, TEXT_COLOR_BRIGHT_GREEN,
                 "%zu bytes at %p (%s:%d) - Total: %zu bytes, Count: %d",
                 toRemove->size, ptr, file, line, g_totalAllocs, g_allocCount);

            *current = toRemove->next;
            free(toRemove);
            free(ptr);
            return;
        }

        current = &(*current)->next;
    }

    _Log("[FREE]", __FILE__, __LINE__, TEXT_COLOR_BRIGHT_YELLOW,
         "Freeing untracked pointer %p (%s:%d)",
         ptr, file, line);
    free(ptr);
}

static inline void _DebugPrintLeaks()
{
    if (!g_allocations)
    {
        _Log("[MEMORY]", __FILE__, __LINE__, TEXT_COLOR_BRIGHT_BLUE,
             "No memory leaks detected.");
        return;
    }

    _Log("[MEMORY]", __FILE__, __LINE__, TEXT_COLOR_BRIGHT_RED,
         "=== MEMORY LEAKS DETECTED! ===");
    _Log("[MEMORY]", __FILE__, __LINE__, TEXT_COLOR_BRIGHT_RED,
         "Total leaked detected: %zu bytes in %d allocations.",
         g_totalAllocs, g_allocCount);

    Allocation* current = g_allocations;
    while (current)
    {
        _Log("[LEAK]", __FILE__, __LINE__, TEXT_COLOR_BRIGHT_RED,
             "%zu bytes at %p (%s:%d)",
             current->size, current->ptr, current->file, current->line);
        current = current->next;
    }
}

static inline void _DebugCleanup()
{
    Allocation* current = g_allocations;
    while (current)
    {
        Allocation* next = current->next;
        free(current);
        current = next;
    }

    g_allocations = NULL;
    g_totalAllocs = 0;
    g_allocCount  = 0;
}