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
static bool                 InitializeViewports();
static bool                 InitializeShadersAndMeshes();
static void                 CleanupShadersAndMeshes();
static bool                 InitializeRenderTargets();
static void                 CleanupRenderTargets();
static D3D11_VIEWPORT*      s_viewportTable       = NULL;
static ID3D11VertexShader** s_pVSTable            = NULL;
static ID3D11PixelShader**  s_pPSTable            = NULL;
static ID3D11Buffer*        s_pTriangleVB         = NULL;
static ID3D11InputLayout*   s_pBasicVSLayout      = NULL;
static GfxRenderTarget*     s_renderTargetsTable  = NULL;
static u32*                 s_renderTargetDivider = NULL;
#define VIEWPORT_TABLE_COUNT 3
#define VS_TABLE_COUNT 2
#define PS_TABLE_COUNT 4
#define RENDER_TARGET_TABLE_COUNT 6
#define VIEWPORT_FULLSIZE_INDEX 0
#define VIEWPORT_HALFSIZE_INDEX 1
#define VIEWPORT_QUARTERSIZE_INDEX 2
#define HDR_RENDER_TARGET_INDEX 0
#define BRIGHTPASS_RENDER_TARGET_INDEX 1
#define BLOOM0_LARGE_RENDER_TARGET_INDEX 2
#define BLOOM1_LARGE_RENDER_TARGET_INDEX 3
#define BLOOM0_MEDIUM_RENDER_TARGET_INDEX 4
#define BLOOM1_MEDIUM_RENDER_TARGET_INDEX 5
#define BASIC_VS_INDEX 0
#define BASIC_PS_INDEX 0
#define COPY_VS_INDEX 1
#define COPY_PS_INDEX 1
#define BRIGHTPASS_PS_INDEX 2
#define BLOOM_PS_INDEX 3
#define TRIANGLE_VB_STRIDE 24

typedef struct
{
    f32 weights[4];
    f32 texelSize[2];
    f32 weightCenter;
    i32 horizontal;
} BloomParams;

typedef struct
{
    f32 intensity;
    f32 padding[3];
} IntensityParams;
#pragma endregion

#pragma region ENTRYPOINT
static void RenderBloomPass(
    const GfxRenderTarget* pCurrent, const GfxRenderTarget* pFormer, i32 horizontal,
    ID3D11ShaderResourceView* pNullSRV, ID3D11Buffer* pBloomCBuffer, const f32* clearColor, ID3D11SamplerState* pLinearSample);

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

    if (!InitializeViewports())
    {
        ASSERT_MSG(false, "Failed to initialize resources.");
        CleanupCore();
        CleanupGlobalMemory();
        return 1;
    }

    if (!InitializeShadersAndMeshes())
    {
        ASSERT_MSG(false, "Failed to initialize resources.");
        CleanupCore();
        CleanupGlobalMemory();
        return 1;
    }

    if (!InitializeRenderTargets())
    {
        ASSERT_MSG(false, "Failed to initialize resources.");
        CleanupShadersAndMeshes();
        CleanupCore();
        CleanupGlobalMemory();
        return 1;
    }

    D3D11_SAMPLER_DESC samplerDesc = {
        .Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        .AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP,
        .AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP,
        .AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP,
        .MipLODBias     = 0.0f,
        .MaxAnisotropy  = 1,
        .ComparisonFunc = D3D11_COMPARISON_ALWAYS,
        .MinLOD         = 0,
        .MaxLOD         = D3D11_FLOAT32_MAX};

    ID3D11SamplerState* plinearSampler = NULL;

    HRESULT hr = s_gfxHandle->pDevice->lpVtbl->CreateSamplerState(
        s_gfxHandle->pDevice, &samplerDesc, &plinearSampler);
    if (FAILED(hr) || !plinearSampler)
    {
        ASSERT_MSG(false, "Failed to create sampler state.");
        CleanupRenderTargets();
        CleanupShadersAndMeshes();
        CleanupCore();
        CleanupGlobalMemory();
        return 1;
    }

    D3D11_BUFFER_DESC bloomBufferDesc = {
        .ByteWidth           = sizeof(BloomParams),
        .Usage               = D3D11_USAGE_DYNAMIC,
        .BindFlags           = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags           = 0,
        .StructureByteStride = 0};

    ID3D11Buffer* pBloomCBuffer = NULL;

    hr = s_gfxHandle->pDevice->lpVtbl->CreateBuffer(
        s_gfxHandle->pDevice, &bloomBufferDesc, NULL, &pBloomCBuffer);

    if (FAILED(hr) || !pBloomCBuffer)
    {
        ASSERT_MSG(false, "Failed to create bloom constant buffer.");
        RELEASE(plinearSampler);
        CleanupRenderTargets();
        CleanupShadersAndMeshes();
        CleanupCore();
        CleanupGlobalMemory();
        return 1;
    }

    D3D11_BUFFER_DESC intensityBufferDesc = {
        .ByteWidth           = sizeof(IntensityParams),
        .Usage               = D3D11_USAGE_DYNAMIC,
        .BindFlags           = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags           = 0,
        .StructureByteStride = 0};

    ID3D11Buffer* pIntensityCBuffer = NULL;

    hr = s_gfxHandle->pDevice->lpVtbl->CreateBuffer(
        s_gfxHandle->pDevice, &intensityBufferDesc, NULL, &pIntensityCBuffer);
    if (FAILED(hr) || !pIntensityCBuffer)
    {
        ASSERT_MSG(false, "Failed to create intensity constant buffer");
        RELEASE(pBloomCBuffer);
        RELEASE(plinearSampler);
        CleanupRenderTargets();
        CleanupShadersAndMeshes();
        CleanupCore();
        CleanupGlobalMemory();
    }

    ID3D11ShaderResourceView* pNullSRV = NULL;

    ShowWindow(s_wndHandle->hwnd, SW_SHOW);

    while (s_isRunning)
    {
        DROP_PollEvents();

        f32 clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        s_gfxHandle->pContext->lpVtbl->RSSetViewports(s_gfxHandle->pContext, 1, &s_viewportTable[VIEWPORT_FULLSIZE_INDEX]);

        // Draw normal meshes on HDR render target.
        s_gfxHandle->pContext->lpVtbl->OMSetRenderTargets(
            s_gfxHandle->pContext, 1, &s_renderTargetsTable[HDR_RENDER_TARGET_INDEX].pRTV, NULL);
        s_gfxHandle->pContext->lpVtbl->ClearRenderTargetView(
            s_gfxHandle->pContext, s_renderTargetsTable[HDR_RENDER_TARGET_INDEX].pRTV, clearColor);

        s_gfxHandle->pContext->lpVtbl->VSSetShader(s_gfxHandle->pContext, s_pVSTable[BASIC_VS_INDEX], NULL, 0);
        s_gfxHandle->pContext->lpVtbl->PSSetShader(s_gfxHandle->pContext, s_pPSTable[BASIC_PS_INDEX], NULL, 0);

        D3D11_MAPPED_SUBRESOURCE mappedResource;

        s_gfxHandle->pContext->lpVtbl->Map(
            s_gfxHandle->pContext, (ID3D11Resource*) pIntensityCBuffer, 0,
            D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (SUCCEEDED(hr))
        {
            IntensityParams* pParams = (IntensityParams*) mappedResource.pData;
            pParams->intensity       = 5.0f;
            s_gfxHandle->pContext->lpVtbl->Unmap(s_gfxHandle->pContext, (ID3D11Resource*) pIntensityCBuffer, 0);
        }
        s_gfxHandle->pContext->lpVtbl->PSSetConstantBuffers(s_gfxHandle->pContext, 0, 1, &pIntensityCBuffer);

        s_gfxHandle->pContext->lpVtbl->IASetInputLayout(s_gfxHandle->pContext, s_pBasicVSLayout);
        u32 stride = TRIANGLE_VB_STRIDE;
        u32 offset = 0;
        s_gfxHandle->pContext->lpVtbl->IASetVertexBuffers(s_gfxHandle->pContext, 0, 1, &s_pTriangleVB, &stride, &offset);

        s_gfxHandle->pContext->lpVtbl->IASetPrimitiveTopology(s_gfxHandle->pContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        s_gfxHandle->pContext->lpVtbl->Draw(s_gfxHandle->pContext, 3, 0);

        // Bright pass.
        s_gfxHandle->pContext->lpVtbl->OMSetRenderTargets(
            s_gfxHandle->pContext, 1, &s_renderTargetsTable[BRIGHTPASS_RENDER_TARGET_INDEX].pRTV, NULL);
        s_gfxHandle->pContext->lpVtbl->ClearRenderTargetView(
            s_gfxHandle->pContext, s_renderTargetsTable[BRIGHTPASS_RENDER_TARGET_INDEX].pRTV, clearColor);

        s_gfxHandle->pContext->lpVtbl->VSSetShader(
            s_gfxHandle->pContext, s_pVSTable[COPY_VS_INDEX], NULL, 0);
        s_gfxHandle->pContext->lpVtbl->PSSetShader(
            s_gfxHandle->pContext, s_pPSTable[BRIGHTPASS_PS_INDEX], NULL, 0);

        s_gfxHandle->pContext->lpVtbl->PSSetShaderResources(
            s_gfxHandle->pContext, 0, 1, &s_renderTargetsTable[HDR_RENDER_TARGET_INDEX].pSRV);
        s_gfxHandle->pContext->lpVtbl->PSSetSamplers(s_gfxHandle->pContext, 0, 1, &plinearSampler);

        s_gfxHandle->pContext->lpVtbl->IASetInputLayout(s_gfxHandle->pContext, NULL);

        s_gfxHandle->pContext->lpVtbl->Draw(s_gfxHandle->pContext, 3, 0);

        s_gfxHandle->pContext->lpVtbl->PSSetShaderResources(s_gfxHandle->pContext, 0, 1, &pNullSRV);

        // Bloom pass.
        s_gfxHandle->pContext->lpVtbl->RSSetViewports(s_gfxHandle->pContext, 1, &s_viewportTable[VIEWPORT_HALFSIZE_INDEX]);
        RenderBloomPass(
            &s_renderTargetsTable[BLOOM0_LARGE_RENDER_TARGET_INDEX],
            &s_renderTargetsTable[HDR_RENDER_TARGET_INDEX],
            1, pNullSRV, pBloomCBuffer, clearColor, plinearSampler);
        RenderBloomPass(
            &s_renderTargetsTable[BLOOM1_LARGE_RENDER_TARGET_INDEX],
            &s_renderTargetsTable[BLOOM0_LARGE_RENDER_TARGET_INDEX],
            0, pNullSRV, pBloomCBuffer, clearColor, plinearSampler);

        s_gfxHandle->pContext->lpVtbl->RSSetViewports(s_gfxHandle->pContext, 1, &s_viewportTable[VIEWPORT_QUARTERSIZE_INDEX]);
        RenderBloomPass(
            &s_renderTargetsTable[BLOOM0_MEDIUM_RENDER_TARGET_INDEX],
            &s_renderTargetsTable[BRIGHTPASS_RENDER_TARGET_INDEX],
            1, pNullSRV, pBloomCBuffer, clearColor, plinearSampler);
        RenderBloomPass(
            &s_renderTargetsTable[BLOOM1_MEDIUM_RENDER_TARGET_INDEX],
            &s_renderTargetsTable[BLOOM0_MEDIUM_RENDER_TARGET_INDEX],
            0, pNullSRV, pBloomCBuffer, clearColor, plinearSampler);

        s_gfxHandle->pContext->lpVtbl->RSSetViewports(s_gfxHandle->pContext, 1, &s_viewportTable[VIEWPORT_FULLSIZE_INDEX]);
        // Copy hdr texture to back buffer.
        s_gfxHandle->pContext->lpVtbl->OMSetRenderTargets(s_gfxHandle->pContext, 1, &s_gfxHandle->pBackBufferRTV, NULL);
        s_gfxHandle->pContext->lpVtbl->ClearRenderTargetView(
            s_gfxHandle->pContext, s_gfxHandle->pBackBufferRTV, clearColor);

        s_gfxHandle->pContext->lpVtbl->PSSetShader(s_gfxHandle->pContext, s_pPSTable[COPY_PS_INDEX], NULL, 0);

        s_gfxHandle->pContext->lpVtbl->PSSetShaderResources(
            s_gfxHandle->pContext, 0, 1, &s_renderTargetsTable[HDR_RENDER_TARGET_INDEX].pSRV);
        s_gfxHandle->pContext->lpVtbl->PSSetShaderResources(
            s_gfxHandle->pContext, 1, 1, &s_renderTargetsTable[BLOOM1_LARGE_RENDER_TARGET_INDEX].pSRV);
        s_gfxHandle->pContext->lpVtbl->PSSetShaderResources(
            s_gfxHandle->pContext, 2, 1, &s_renderTargetsTable[BLOOM1_MEDIUM_RENDER_TARGET_INDEX].pSRV);

        s_gfxHandle->pContext->lpVtbl->Draw(s_gfxHandle->pContext, 3, 0);

        s_gfxHandle->pContext->lpVtbl->PSSetShaderResources(s_gfxHandle->pContext, 0, 1, &pNullSRV);
        s_gfxHandle->pContext->lpVtbl->PSSetShaderResources(s_gfxHandle->pContext, 1, 1, &pNullSRV);
        s_gfxHandle->pContext->lpVtbl->PSSetShaderResources(s_gfxHandle->pContext, 2, 1, &pNullSRV);

        s_gfxHandle->pSwapChain->lpVtbl->Present(s_gfxHandle->pSwapChain, 1, 0);

        DROP_ClearArena(TRANSIENT);
    }

    ShowWindow(s_wndHandle->hwnd, SW_HIDE);

    RELEASE(pIntensityCBuffer);
    RELEASE(pBloomCBuffer);
    RELEASE(plinearSampler);
    CleanupRenderTargets();
    CleanupShadersAndMeshes();
    CleanupCore();
    CleanupGlobalMemory();

    PRINT_LEAKS();
    CLEANUP();
    return 0;
}

static void RenderBloomPass(
    const GfxRenderTarget* pCurrent, const GfxRenderTarget* pFormer, i32 horizontal,
    ID3D11ShaderResourceView* pNullSRV, ID3D11Buffer* pBloomCBuffer, const f32* clearColor, ID3D11SamplerState* pLinearSample)
{
    s_gfxHandle->pContext->lpVtbl->OMSetRenderTargets(
        s_gfxHandle->pContext, 1, &pCurrent->pRTV, NULL);
    s_gfxHandle->pContext->lpVtbl->ClearRenderTargetView(
        s_gfxHandle->pContext, pCurrent->pRTV, clearColor);

    s_gfxHandle->pContext->lpVtbl->PSSetShader(
        s_gfxHandle->pContext, s_pPSTable[BLOOM_PS_INDEX], NULL, 0);

    s_gfxHandle->pContext->lpVtbl->PSSetShaderResources(
        s_gfxHandle->pContext, 0, 1, &pFormer->pSRV);
    s_gfxHandle->pContext->lpVtbl->PSSetSamplers(s_gfxHandle->pContext, 0, 1, &pLinearSample);

    D3D11_MAPPED_SUBRESOURCE mappedResource;

    HRESULT hr = s_gfxHandle->pContext->lpVtbl->Map(
        s_gfxHandle->pContext, (ID3D11Resource*) pBloomCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
        BloomParams* pParams  = (BloomParams*) mappedResource.pData;
        pParams->texelSize[0] = 1.0f / (f32) pCurrent->width;
        pParams->texelSize[1] = 1.0f / (f32) pCurrent->height;
        pParams->horizontal   = horizontal;
        pParams->weightCenter = 0.4026199470f;
        pParams->weights[0]   = 0.2442013420f;
        pParams->weights[1]   = 0.2442013420f;
        pParams->weights[2]   = 0.0544886845f;
        pParams->weights[3]   = 0.0544886845;
        s_gfxHandle->pContext->lpVtbl->Unmap(s_gfxHandle->pContext, (ID3D11Resource*) pBloomCBuffer, 0);
    }
    s_gfxHandle->pContext->lpVtbl->PSSetConstantBuffers(s_gfxHandle->pContext, 0, 1, &pBloomCBuffer);

    s_gfxHandle->pContext->lpVtbl->Draw(s_gfxHandle->pContext, 3, 0);

    s_gfxHandle->pContext->lpVtbl->PSSetShaderResources(s_gfxHandle->pContext, 0, 1, &pNullSRV);
}
#pragma endregion

#pragma region RESOURCES
static bool InitializeViewports()
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

    s_viewportTable[1].Width    = s_wndHandle->width / 2.0f;
    s_viewportTable[1].Height   = s_wndHandle->height / 2.0f;
    s_viewportTable[1].TopLeftX = 0;
    s_viewportTable[1].TopLeftY = 0;
    s_viewportTable[1].MinDepth = 0.0f;
    s_viewportTable[1].MaxDepth = 1.0f;

    s_viewportTable[2].Width    = s_wndHandle->width / 4.0f;
    s_viewportTable[2].Height   = s_wndHandle->height / 4.0f;
    s_viewportTable[2].TopLeftX = 0;
    s_viewportTable[2].TopLeftY = 0;
    s_viewportTable[2].MinDepth = 0.0f;
    s_viewportTable[2].MaxDepth = 1.0f;

    return true;
}
static bool InitializeShadersAndMeshes()
{
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
        L"assets/shaders/basic_vs.cso",
        L"assets/shaders/copy_vs.cso"};
    ID3DBlob* pByteCodeList[] = {
        NULL,
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
        L"assets/shaders/basic_ps.cso",
        L"assets/shaders/copy_ps.cso",
        L"assets/shaders/brightpass_ps.cso",
        L"assets/shaders/bloom_ps.cso"};
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
        {.pos = {0.0f, 0.5f}, .color = {2.0f, 0.5f, 0.2f, 1.0f}},   // Brighter red
        {.pos = {0.5f, -0.5f}, .color = {0.2f, 2.0f, 0.2f, 1.0f}},  // Brighter green
        {.pos = {-0.5f, -0.5f}, .color = {0.2f, 0.2f, 2.0f, 1.0f}}, // Brighter blue
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
static bool InitializeRenderTargets()
{
    s_renderTargetsTable = (GfxRenderTarget*) DROP_Allocate(
        PERSISTENT, sizeof(GfxRenderTarget) * RENDER_TARGET_TABLE_COUNT);
    if (!s_renderTargetsTable)
    {
        LOG_ERROR("Failed to allocate memory for render targets table.");
        return false;
    }

    s_renderTargetDivider = (u32*) (DROP_Allocate(PERSISTENT, sizeof(u32) * RENDER_TARGET_TABLE_COUNT));
    if (!s_renderTargetDivider)
    {
        LOG_ERROR("Failed to allocate memory for render target divider.");
        return false;
    }

    s_renderTargetDivider[0] = 1;
    s_renderTargetDivider[1] = 1;
    s_renderTargetDivider[2] = 2;
    s_renderTargetDivider[3] = 2;
    s_renderTargetDivider[4] = 4;
    s_renderTargetDivider[5] = 4;

    for (u32 i = 0; i < RENDER_TARGET_TABLE_COUNT; ++i)
    {
        if (!DROP_CreateHDRRenderTarget(
                s_gfxHandle, s_wndHandle->width / s_renderTargetDivider[i],
                s_wndHandle->height / s_renderTargetDivider[i], &s_renderTargetsTable[i]))
        {
            LOG_ERROR("Failed to create render targets at index: %d", i);
            for (u32 j = 0; j < i; ++j)
                DROP_DestroyRenderTarget(&s_renderTargetsTable[j]);
            return false;
        }
    }

    return true;
}
static void CleanupRenderTargets()
{
    for (u32 i = 0; i < RENDER_TARGET_TABLE_COUNT; ++i)
        DROP_DestroyRenderTarget(&s_renderTargetsTable[i]);
}
static void CleanupShadersAndMeshes()
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

    for (u32 i = 0; i < RENDER_TARGET_TABLE_COUNT; ++i)
    {
        RELEASE(s_renderTargetsTable[i].pRTV);
        RELEASE(s_renderTargetsTable[i].pSRV);
        RELEASE(s_renderTargetsTable[i].pTexture);

        if (!DROP_CreateHDRRenderTarget(
                s_gfxHandle, width / s_renderTargetDivider[i], height / s_renderTargetDivider[i],
                &s_renderTargetsTable[i]))
        {
            LOG_ERROR("Failed to resize render target at index: %d", i);
            PostQuitMessage(0);
            s_isRunning = false;
            return true;
        }
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