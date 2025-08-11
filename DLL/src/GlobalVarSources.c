#include "pch.h"

Memory* g_memory = NULL;
#ifdef DEBUG
Allocation* g_allocations = NULL;
u64         g_totalAllocs = 0;
i32         g_allocCount  = 0;
#endif // DEBUG