#pragma once

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char      i8;
typedef signed short     i16;
typedef signed int       i32;
typedef signed long long i64;

typedef float  f32;
typedef double f64;

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#ifdef DEBUG
#include <stdio.h>
#include <stdarg.h>
#include "Utils/Logger.h"
#include "Utils/DebugMemory.h"
#endif // DEBUG

#ifdef DEBUG
#define ALLOC(T, N) _DebugMalloc(sizeof(T) * (N), __FILE__, __LINE__)
#define FREE(ptr) _DebugFree(ptr, __FILE__, __LINE__)
#define PRINT_LEAKS() _DebugPrintLeaks()
#define CLEANUP() _DebugCleanup()
#else
#define ALLOC(T, N) malloc(sizeof(T) * (N))
#define FREE(ptr) free(ptr)
#define PRINT_LEAKS()
#define CLEANUP()
#endif // DEBUG

#define ZERO_MEM(ptr, N) memset(ptr, 0, sizeof(*ptr) * N)

#ifdef DEBUG
#ifdef _MSC_VER
#define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#define DEBUG_BREAK() __builtin_trap()
#else
#define DEBUG_BREAK() (*(volatile int*) 0 = 0)
#endif // _MSC_VER

#define DEBUG_OP(x) x

#define LOG_TRACE(...) _Log("[TRACE]", __FILE__, __LINE__, TEXT_COLOR_GREEN, __VA_ARGS__)
#define LOG_WARN(...) _Log("[WARN]", __FILE__, __LINE__, TEXT_COLOR_YELLOW, __VA_ARGS__)
#define LOG_ERROR(...) _Log("[ERROR]", __FILE__, __LINE__, TEXT_COLOR_RED, __VA_ARGS__)

#define ASSERT(x) \
    if (!(x)) DEBUG_BREAK();

#define ASSERT_MSG(x, ...)      \
    if (!(x))                   \
    {                           \
        LOG_ERROR(__VA_ARGS__); \
        DEBUG_BREAK();          \
    }

#else
#define DEBUG_BREAK()

#define DEBUG_OP(x)

#define LOG_TRACE(...)
#define LOG_WARN(...)
#define LOG_ERROR(...)

#define ASSERT(x)
#define ASSERT_MSG(x, ...)
#endif // DEBUG

#define BIT(x) (1 << x)
#define KB(x) (x * 1024)
#define MB(x) (KB(x) * 1024)

#include "Utils/ArenaAllocator.h"

#define PERSISTENT g_memory->pPersistentStorage
#define TRANSIENT g_memory->pTransientStorage

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#define RELEASE(x) x->lpVtbl->Release(x)
#define SAFE_RELEASE(x) \
    if (x) x->lpVtbl->Release(x)