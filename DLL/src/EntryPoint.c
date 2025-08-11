#include "pch.h"
#include "EntryPoint.h"

#include "Platform/Window.h"
#include "Graphics/Graphics.h"

#pragma region GLOBAL_MEMORY
static bool InitializeGlobalMemory(u64 size);
static void CleanupGlobalMemory();
#pragma endregion

#pragma region CORE
static bool            InitializeCore();
static void            CleanupCore();
static WndHandle       s_wndHandle     = NULL;
static GfxHandle       s_gfxHandle     = NULL;
static bool            s_isRunning     = true;
static D3D11_VIEWPORT* s_viewportTable = NULL;
#pragma endregion CORE

int EntryPoint()
{
    if (!InitializeGlobalMemory(KB(1)))
    {
        ASSERT_MSG(false, "Failed to initialize global memory.");
        return 1;
    }

    if (!InitializeCore())
    {
        ASSERT_MSG(false, "Failed to initialize core.");
        return 1;
    }

    ShowWindow(s_wndHandle->hwnd, SW_SHOW);

    while (s_isRunning)
    {
        DROP_PollEvents();

        s_gfxHandle->pContext->lpVtbl->RSSetViewports(s_gfxHandle->pContext, 1, &s_viewportTable[0]);
        s_gfxHandle->pContext->lpVtbl->OMSetRenderTargets(s_gfxHandle->pContext, 1, &s_gfxHandle->pBackBufferRTV, NULL);
        f32 clears[4] = {0.2f, 0.3f, 0.3f, 1.0f};
        s_gfxHandle->pContext->lpVtbl->ClearRenderTargetView(
            s_gfxHandle->pContext, s_gfxHandle->pBackBufferRTV, clears);

        s_gfxHandle->pSwapChain->lpVtbl->Present(s_gfxHandle->pSwapChain, 1, 0);
    }

    ShowWindow(s_wndHandle->hwnd, SW_HIDE);

    CleanupCore();

    CleanupGlobalMemory();

    PRINT_LEAKS();
    CLEANUP();
    return 0;
}

#pragma region CORE
static bool OnClose()
{
    PostQuitMessage(0);
    s_isRunning = false;
    return true;
}

static bool InitializeCore()
{

    WndInitProps wndProps = {
        .title    = L"Learning DX11",
        .width    = 1280,
        .height   = 720,
        .callback = {
            .OnClose = OnClose}};

    if (!DROP_CreateWindow(&wndProps, &s_wndHandle) || !s_wndHandle)
    {
        LOG_ERROR("Failed to create window.");
        return false;
    }

    GfxInitProps gfxProps = {
        .wndHandle = s_wndHandle};

    if (!DROP_CreateGraphics(&gfxProps, &s_gfxHandle) || !s_gfxHandle)
    {
        LOG_ERROR("Failed to create graphics.");
        DROP_DestroyWindow(&s_wndHandle);
        return false;
    }

    s_viewportTable = (D3D11_VIEWPORT*) DROP_Allocate(PERSISTENT, sizeof(D3D11_VIEWPORT) * 1);
    if (!s_viewportTable)
    {
        LOG_ERROR("Failed to allocate memory for viewport table.");
        DROP_DestroyGraphics(&s_gfxHandle);
        DROP_DestroyWindow(&s_wndHandle);
        return false;
    }

    s_viewportTable[0].TopLeftX = 0;
    s_viewportTable[0].TopLeftY = 0;
    s_viewportTable[0].Width    = 1280;
    s_viewportTable[0].Height   = 720;
    s_viewportTable[0].MaxDepth = 1.0f;
    s_viewportTable[0].MinDepth = 0.0f;

    return true;
}
static void CleanupCore()
{
    DROP_DestroyGraphics(&s_gfxHandle);
    DROP_DestroyWindow(&s_wndHandle);
}
#pragma endregion CORE

#pragma region GLOBAL_MEMORY
static bool InitializeGlobalMemory(u64 size)
{
    ArenaAllocator* pPersistent = (ArenaAllocator*) ALLOC(ArenaAllocator, 1);
    ArenaAllocator* pTransient  = (ArenaAllocator*) ALLOC(ArenaAllocator, 1);
    if (!pPersistent || !pTransient)
    {
        LOG_ERROR("Failed to allocate pointer to persistent and transient storage.");
        if (pPersistent) FREE(pPersistent);
        if (pTransient) FREE(pTransient);
        return false;
    }

    g_memory = (Memory*) ALLOC(Memory, 1);
    if (!g_memory)
    {
        LOG_ERROR("Failed to allocate global memory");
        FREE(pPersistent);
        FREE(pTransient);
        return false;
    }

    g_memory->pPersistentStorage = pPersistent;
    g_memory->pTransientStorage  = pTransient;

    if (!DROP_MakeArena(PERSISTENT, size) || !PERSISTENT)
    {
        LOG_ERROR("Failed to allocate persistent storage.");
        FREE(PERSISTENT);
        FREE(TRANSIENT);
        FREE(g_memory);
        return false;
    }

    if (!DROP_MakeArena(TRANSIENT, size) || !TRANSIENT)
    {
        LOG_ERROR("Failed to allocate transient storage.");
        FREE(PERSISTENT->memory);
        FREE(PERSISTENT);
        FREE(TRANSIENT);
        FREE(g_memory);
        return false;
    }

    return true;
}
static void CleanupGlobalMemory()
{
    FREE(TRANSIENT->memory);
    FREE(PERSISTENT->memory);

    FREE(PERSISTENT);
    FREE(TRANSIENT);

    FREE(g_memory);

    g_memory = NULL;
}
#pragma endregion