#pragma once

typedef struct _ArenaAllocator
{
    char* memory;
    u64   size;
    u64   used;
} ArenaAllocator;

typedef struct _Memory
{
    ArenaAllocator* pPersistentStorage;
    ArenaAllocator* pTransientStorage;
} Memory;

extern Memory* g_memory;

static inline bool DROP_MakeArena(ArenaAllocator* pArena, u64 size)
{
    ASSERT_MSG(pArena, "Arena pointer is null.");
    ASSERT_MSG(size > 0, "Size must be greater than zero.");

    char* memory = (char*) ALLOC(char, size);
    if (!memory)
    {
        ASSERT_MSG(false, "Failed to allocate the memory.");
        return false;
    }

    ZERO_MEM(memory, size);

    pArena->memory = memory;
    pArena->size   = size;
    pArena->used   = 0;

    return true;
}

static inline char* DROP_Allocate(ArenaAllocator* pArena, u64 size)
{
    u64 allignedSize = (size + 15) & ~15;

    if (pArena->used + allignedSize > pArena->size)
    {
        ASSERT_MSG(false, "The size are greater than remaining memory in arena.");
        return NULL;
    }

    char* memory = pArena->memory + pArena->used;
    pArena->used += allignedSize;

    return memory;
}

static inline void DROP_ClearArena(ArenaAllocator* pArena)
{
    pArena->used = 0;
}
