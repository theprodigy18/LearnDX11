#include "pch.h"
#include "EntryPoint.h"

#include "Platform/Window.h"
#include "Graphics/Graphics.h"

#include "Resources/Shaders.h"
#include "Resources/Mesh.h"

#pragma region GLOBAL_MEMORY
static bool InitializeGlobalMemory(u64 size);
static void CleanupGlobalMemory();
#pragma endregion

#pragma region CORE
static bool      InitializeCore();
static void      CleanupCore();
static WndHandle s_wndHandle = NULL;
static GfxHandle s_gfxHandle = NULL;
static bool      s_isRunning = true;
#pragma endregion CORE

#pragma region RESOURCES
static bool                 InitializeResources();
static void                 CleanupResources();
static D3D11_VIEWPORT*      s_viewportTable      = NULL;
static const u32            VIEWPORT_TABLE_COUNT = 1;
static ID3D11VertexShader** s_pVSTable           = NULL;
static const u32            VS_TABLE_COUNT       = 1;
static ID3D11PixelShader**  s_pPSTable           = NULL;
static const u32            PS_TABLE_COUNT       = 1;
static ID3D11Buffer*        s_pTriangleVB        = NULL;
static ID3D11InputLayout*   s_pBasicVSLayout     = NULL;
#define BASIC_VS_INDEX 0
#define BASIC_PS_INDEX 0
#define TRIANGLE_VB_STRIDE 24
#pragma endregion

#pragma region ENTRYPOINT
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
        CleanupGlobalMemory();
        return 1;
    }

    if (!InitializeResources())
    {
        ASSERT_MSG(false, "Failed to initialize resources.");
        CleanupCore();
        CleanupGlobalMemory();
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

        s_gfxHandle->pContext->lpVtbl->VSSetShader(s_gfxHandle->pContext, s_pVSTable[BASIC_VS_INDEX], NULL, 0);
        s_gfxHandle->pContext->lpVtbl->PSSetShader(s_gfxHandle->pContext, s_pPSTable[BASIC_PS_INDEX], NULL, 0);

        s_gfxHandle->pContext->lpVtbl->IASetInputLayout(s_gfxHandle->pContext, s_pBasicVSLayout);
        u32 stride = TRIANGLE_VB_STRIDE;
        u32 offset = 0;
        s_gfxHandle->pContext->lpVtbl->IASetVertexBuffers(s_gfxHandle->pContext, 0, 1, &s_pTriangleVB, &stride, &offset);

        s_gfxHandle->pContext->lpVtbl->IASetPrimitiveTopology(s_gfxHandle->pContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        s_gfxHandle->pContext->lpVtbl->Draw(s_gfxHandle->pContext, 3, 0);

        s_gfxHandle->pSwapChain->lpVtbl->Present(s_gfxHandle->pSwapChain, 1, 0);

        DROP_ClearArena(TRANSIENT);
    }

    ShowWindow(s_wndHandle->hwnd, SW_HIDE);

    CleanupResources();
    CleanupCore();
    CleanupGlobalMemory();

    PRINT_LEAKS();
    CLEANUP();
    return 0;
}
#pragma endregion

#pragma region RESOURCES
static bool InitializeResources()
{
    // Setup viewport.
    s_viewportTable = (D3D11_VIEWPORT*) DROP_Allocate(PERSISTENT, (sizeof(D3D11_VIEWPORT) * VIEWPORT_TABLE_COUNT));
    if (!s_viewportTable)
    {
        LOG_ERROR("Failed to allocate memory for viewport table.");
        return false;
    }

    s_viewportTable[0].Width    = s_wndHandle->width;
    s_viewportTable[0].Height   = s_wndHandle->height;
    s_viewportTable[0].TopLeftX = 0;
    s_viewportTable[0].TopLeftY = 0;
    s_viewportTable[0].MinDepth = 0.0f;
    s_viewportTable[0].MaxDepth = 1.0f;

    // Create shader.
    s_pVSTable = (ID3D11VertexShader**) DROP_Allocate(PERSISTENT, sizeof(ID3D11VertexShader*) * VS_TABLE_COUNT);
    if (!s_pVSTable)
    {
        LOG_ERROR("Failed to allocate memory for vertex shader table.");
        return false;
    }
    s_pPSTable = (ID3D11PixelShader**) DROP_Allocate(PERSISTENT, sizeof(ID3D11PixelShader*) * PS_TABLE_COUNT);
    if (!s_pPSTable)
    {
        LOG_ERROR("Failed to allocate memory for pixel shader table.");
        return false;
    }

    HRESULT hr = 0;

    const wchar_t* vertexShaderList[] = {
        L"assets/shaders/basic_vs.cso"};
    ID3DBlob* pByteCodeList[] = {
        NULL};

    for (u32 i = 0; i < VS_TABLE_COUNT; ++i)
    {
        hr = D3DReadFileToBlob(vertexShaderList[i], &pByteCodeList[i]);
        if (FAILED(hr) || !pByteCodeList[i])
        {
            LOG_ERROR("Failed to load vertex shader: %s.", vertexShaderList[i]);
            for (u32 j = 0; j < i; ++j)
                RELEASE(pByteCodeList[j]);
            return false;
        }

        hr = s_gfxHandle->pDevice->lpVtbl->CreateVertexShader(
            s_gfxHandle->pDevice, pByteCodeList[i]->lpVtbl->GetBufferPointer(pByteCodeList[i]),
            pByteCodeList[i]->lpVtbl->GetBufferSize(pByteCodeList[i]), NULL, &s_pVSTable[i]);
        if (FAILED(hr) || !s_pVSTable[i])
        {
            LOG_ERROR("Failed to create vertex shader at index %d", i);
            for (u32 j = 0; j <= i; ++j)
            {
                RELEASE(pByteCodeList[j]);
                if (j < i)
                    RELEASE(s_pVSTable[j]);
            }
            return false;
        }
    }

    const wchar_t* pixelShaderList[] = {
        L"assets/shaders/basic_ps.cso"};
    for (u32 i = 0; i < PS_TABLE_COUNT; ++i)
    {
        ID3DBlob* pByteCode = NULL;

        hr = D3DReadFileToBlob(pixelShaderList[i], &pByteCode);
        if (FAILED(hr) || !pByteCode)
        {
            LOG_ERROR("Failed to load pixel shader: %s.", pixelShaderList[i]);
            for (u32 j = 0; j < VS_TABLE_COUNT; ++j)
            {
                RELEASE(pByteCodeList[j]);
                RELEASE(s_pVSTable[j]);
            }
            return false;
        }

        hr = s_gfxHandle->pDevice->lpVtbl->CreatePixelShader(
            s_gfxHandle->pDevice, pByteCode->lpVtbl->GetBufferPointer(pByteCode),
            pByteCode->lpVtbl->GetBufferSize(pByteCode), NULL, &s_pPSTable[i]);
        pByteCode->lpVtbl->Release(pByteCode);
        if (FAILED(hr) || !s_pPSTable[i])
        {
            LOG_ERROR("Failed to create pixel shader at index %d", i);
            for (u32 j = 0; j < VS_TABLE_COUNT; ++j)
            {
                RELEASE(pByteCodeList[j]);
                RELEASE(s_pVSTable[j]);
            }
            for (u32 j = 0; j < i; ++j)
                RELEASE(s_pPSTable[j]);
            return false;
        }
    }

    typedef struct
    {
        f32 pos[2];
        f32 color[4];
    } Vertex;

    Vertex vertices[] = {
        {.pos = {0.0f, 0.5f}, .color = {1.0f, 0.0f, 0.0f, 1.0f}},
        {.pos = {0.5f, -0.5f}, .color = {0.0f, 1.0f, 0.0f, 1.0f}},
        {.pos = {-0.5f, -0.5f}, .color = {0.0f, 0.0f, 1.0f, 1.0f}},
    };

    if (!DROP_CreateVertexBuffer(
            s_gfxHandle, vertices, sizeof(vertices), &s_pTriangleVB) ||
        !s_pTriangleVB)
    {
        LOG_ERROR("Failed to create triangle vertex buffer.");
        for (u32 i = 0; i < VS_TABLE_COUNT; ++i)
        {
            RELEASE(pByteCodeList[i]);
            RELEASE(s_pVSTable[i]);
        }
        for (u32 i = 0; i < PS_TABLE_COUNT; ++i)
            RELEASE(s_pPSTable[i]);
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC basicVSLayouts[] = {
        {.SemanticName         = "POSITION",
         .SemanticIndex        = 0,
         .Format               = DXGI_FORMAT_R32G32_FLOAT,
         .InputSlot            = 0,
         .AlignedByteOffset    = 0,
         .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
         .InstanceDataStepRate = 0},
        {.SemanticName         = "COLOR",
         .SemanticIndex        = 0,
         .Format               = DXGI_FORMAT_R32G32B32A32_FLOAT,
         .InputSlot            = 0,
         .AlignedByteOffset    = 8,
         .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
         .InstanceDataStepRate = 0}};

    if (!DROP_CreateInputLayout(
            s_gfxHandle, basicVSLayouts, ARRAYSIZE(basicVSLayouts),
            pByteCodeList[BASIC_VS_INDEX], &s_pBasicVSLayout) ||
        !s_pBasicVSLayout)
    {
        LOG_ERROR("Failed to create input layout.");
        s_pTriangleVB->lpVtbl->Release(s_pTriangleVB);
        for (u32 i = 0; i < VS_TABLE_COUNT; ++i)
        {
            RELEASE(pByteCodeList[i]);
            RELEASE(s_pVSTable[i]);
        }
        for (u32 i = 0; i < PS_TABLE_COUNT; ++i)
            RELEASE(s_pPSTable[i]);
        return false;
    }

    for (u32 i = 0; i < VS_TABLE_COUNT; ++i)
        RELEASE(pByteCodeList[i]);

    return true;
}
static void CleanupResources()
{
    SAFE_RELEASE(s_pBasicVSLayout);
    SAFE_RELEASE(s_pTriangleVB);

    for (u32 i = 0; i < VS_TABLE_COUNT; ++i)
    {
        SAFE_RELEASE(s_pVSTable[i]);
    }
    for (u32 i = 0; i < PS_TABLE_COUNT; ++i)
    {
        SAFE_RELEASE(s_pPSTable[i]);
    }
}
#pragma endregion

#pragma region CORE
static bool OnClose()
{
    PostQuitMessage(0);
    s_isRunning = false;
    return true;
}
static bool OnResize(RECT* pRect)
{
    u32 width  = pRect->right - pRect->left;
    u32 height = pRect->bottom - pRect->top;

    s_wndHandle->width  = width;
    s_wndHandle->height = height;

    if (!DROP_ResizeGraphics(s_gfxHandle, width, height))
    {
        ASSERT_MSG(false, "Failed to resize graphics.");
        PostQuitMessage(0);
        s_isRunning = false;
        return true;
    }

    for (u32 i = 0; i < VIEWPORT_TABLE_COUNT; ++i)
    {
        s_viewportTable[i].Width  = width;
        s_viewportTable[i].Height = height;
    }

    return false;
}
static bool InitializeCore()
{

    WndInitProps wndProps = {
        .title    = L"Learning DX11",
        .width    = 1280,
        .height   = 720,
        .callback = {
            .OnClose  = OnClose,
            .OnResize = OnResize}};

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