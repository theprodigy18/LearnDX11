#include "pch.h"
#include "Graphics/Graphics.h"

bool DROP_CreateGraphics(const GfxInitProps* pProps, GfxHandle* pHandle)
{
    ASSERT_MSG(pProps && pProps->wndHandle, "Graphics properties are null.");
    ASSERT_MSG(pHandle, "Graphics handle pointer are null.");

    *pHandle = NULL;

    WndHandle wndHandle = pProps->wndHandle;

    DXGI_SWAP_CHAIN_DESC scDesc = {
        .BufferCount                        = 2,
        .BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferDesc.Format                  = DXGI_FORMAT_B8G8R8A8_UNORM,
        .BufferDesc.Width                   = wndHandle->width,
        .BufferDesc.Height                  = wndHandle->height,
        .BufferDesc.RefreshRate.Numerator   = 60,
        .BufferDesc.RefreshRate.Denominator = 1,
        .SampleDesc.Count                   = 1,
        .SampleDesc.Quality                 = 0,
        .OutputWindow                       = wndHandle->hwnd,
        .Windowed                           = TRUE,
        .Flags                              = 0,
        .SwapEffect                         = DXGI_SWAP_EFFECT_FLIP_DISCARD};

    D3D_FEATURE_LEVEL featureLevel;

    UINT flags = 0;
#ifdef DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // DEBUG

    ID3D11Device*        pDevice    = NULL;
    ID3D11DeviceContext* pContext   = NULL;
    IDXGISwapChain*      pSwapChain = NULL;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags,
        NULL, 0, D3D11_SDK_VERSION, &scDesc,
        &pSwapChain, &pDevice,
        &featureLevel, &pContext);

    if (FAILED(hr) || !pDevice || !pContext || !pSwapChain)
    {
        ASSERT_MSG(false, "Failed to create device and swapchain.");
        return false;
    }

#ifdef DEBUG
    ID3D11InfoQueue* pInfoQueue = NULL;

    hr = pDevice->lpVtbl->QueryInterface(pDevice, &IID_ID3D11InfoQueue, (void**) &pInfoQueue);
    if (SUCCEEDED(hr) && pInfoQueue)
    {
        pInfoQueue->lpVtbl->SetBreakOnSeverity(pInfoQueue, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->lpVtbl->SetBreakOnSeverity(pInfoQueue, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->lpVtbl->SetBreakOnSeverity(pInfoQueue, D3D11_MESSAGE_SEVERITY_WARNING, TRUE);

        RELEASE(pInfoQueue);
    }
#endif // DEBUG

    ID3D11Buffer* pBackBuffer = NULL;

    hr = pSwapChain->lpVtbl->GetBuffer(pSwapChain, 0, &IID_ID3D11Texture2D, (void**) &pBackBuffer);
    if (FAILED(hr) || !pBackBuffer)
    {
        ASSERT_MSG(false, "Failed to get back buffer from swapchain.");
        RELEASE(pSwapChain);
        RELEASE(pContext);
        RELEASE(pDevice);
        return false;
    }

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {
        .Format             = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        .ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D,
        .Texture2D.MipSlice = 0};

    ID3D11RenderTargetView* pRTV = NULL;

    hr = pDevice->lpVtbl->CreateRenderTargetView(pDevice, (ID3D11Resource*) pBackBuffer, &rtvDesc, &pRTV);
    RELEASE(pBackBuffer);
    if (FAILED(hr) || !pRTV)
    {
        ASSERT_MSG(false, "Failed to create render target view.");
        RELEASE(pSwapChain);
        RELEASE(pContext);
        RELEASE(pDevice);
        return false;
    }

    GfxHandle handle = (GfxHandle) DROP_Allocate(PERSISTENT, sizeof(_GfxHandle));
    if (!handle)
    {
        ASSERT_MSG(false, "Failed to allocate memory for handle.");
        RELEASE(pRTV);
        RELEASE(pSwapChain);
        RELEASE(pContext);
        RELEASE(pDevice);
        return false;
    }
    handle->pDevice        = pDevice;
    handle->pContext       = pContext;
    handle->pSwapChain     = pSwapChain;
    handle->pBackBufferRTV = pRTV;

    *pHandle = handle;

    return true;
}

void DROP_DestroyGraphics(GfxHandle* pHandle)
{
    ASSERT_MSG(pHandle && *pHandle, "Graphics handle is null");
    GfxHandle handle = *pHandle;

    if (handle)
    {
        SAFE_RELEASE(handle->pBackBufferRTV);
        SAFE_RELEASE(handle->pSwapChain);
        SAFE_RELEASE(handle->pContext);
        SAFE_RELEASE(handle->pDevice);

        handle->pDevice        = NULL;
        handle->pContext       = NULL;
        handle->pSwapChain     = NULL;
        handle->pBackBufferRTV = NULL;
    }

    *pHandle = NULL;
}

bool DROP_ResizeGraphics(GfxHandle handle, u32 width, u32 height)
{
    ASSERT_MSG(handle, "Graphics handle is null.");

    SAFE_RELEASE(handle->pBackBufferRTV);
    handle->pBackBufferRTV = NULL;

    HRESULT hr = handle->pSwapChain->lpVtbl->ResizeBuffers(
        handle->pSwapChain, 2, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    if (FAILED(hr))
    {
        ASSERT_MSG(false, "Failed to resize buffers.");
        return false;
    }

    ID3D11Buffer* pBackBuffer = NULL;

    hr = handle->pSwapChain->lpVtbl->GetBuffer(handle->pSwapChain, 0, &IID_ID3D11Texture2D, (void**) &pBackBuffer);
    if (FAILED(hr) || !pBackBuffer)
    {
        ASSERT_MSG(false, "Failed to get back buffer.");
        return false;
    }

    ID3D11RenderTargetView* pRTV = NULL;

    hr = handle->pDevice->lpVtbl->CreateRenderTargetView(
        handle->pDevice, (ID3D11Resource*) pBackBuffer, NULL, &pRTV);
    RELEASE(pBackBuffer);
    if (FAILED(hr) || !pRTV)
    {
        ASSERT_MSG(false, "Failed to create new render target view.");
        return false;
    }

    handle->pBackBufferRTV = pRTV;
    return true;
}

bool DROP_CreateHDRRenderTarget(const GfxHandle handle, u32 width, u32 height, GfxRenderTarget* pRenderTarget)
{
    ASSERT_MSG(handle, "Graphics handle is null.");
    ASSERT_MSG(pRenderTarget, "Render target view is null.");

    D3D11_TEXTURE2D_DESC texDesc = {
        .Width              = width,
        .Height             = height,
        .SampleDesc.Count   = 1,
        .SampleDesc.Quality = 0,
        .ArraySize          = 1,
        .BindFlags          = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        .CPUAccessFlags     = 0,
        .MiscFlags          = 0,
        .Format             = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .MipLevels          = 1,
        .Usage              = D3D11_USAGE_DEFAULT};

    ID3D11Texture2D* pTexture = NULL;

    HRESULT hr = handle->pDevice->lpVtbl->CreateTexture2D(handle->pDevice, &texDesc, NULL, &pTexture);
    if (FAILED(hr) || !pTexture)
    {
        ASSERT_MSG(false, "Failed to create texture 2D.");
        return false;
    }

    ID3D11RenderTargetView* pRTV = NULL;

    hr = handle->pDevice->lpVtbl->CreateRenderTargetView(
        handle->pDevice, (ID3D11Resource*) pTexture, NULL, &pRTV);
    if (FAILED(hr) || !pRTV)
    {
        ASSERT_MSG(false, "Failed to create texture 2D.");
        RELEASE(pTexture);
        return false;
    }

    ID3D11ShaderResourceView* pSRV = NULL;

    hr = handle->pDevice->lpVtbl->CreateShaderResourceView(
        handle->pDevice, (ID3D11Resource*) pTexture, NULL, &pSRV);
    if (FAILED(hr) || !pSRV)
    {
        ASSERT_MSG(false, "Failed to create texture 2D.");
        RELEASE(pRTV);
        RELEASE(pTexture);
        return false;
    }

    pRenderTarget->pRTV     = pRTV;
    pRenderTarget->pSRV     = pSRV;
    pRenderTarget->pTexture = pTexture;
    pRenderTarget->width    = width;
    pRenderTarget->height   = height;

    return true;
}

void DROP_DestroyRenderTarget(GfxRenderTarget* pRenderTarget)
{
    ASSERT_MSG(pRenderTarget, "Render target is null.");

    if (pRenderTarget)
    {
        SAFE_RELEASE(pRenderTarget->pRTV);
        SAFE_RELEASE(pRenderTarget->pSRV);
        SAFE_RELEASE(pRenderTarget->pTexture);

        pRenderTarget->pRTV     = NULL;
        pRenderTarget->pSRV     = NULL;
        pRenderTarget->pTexture = NULL;
        pRenderTarget->width    = 0;
        pRenderTarget->height   = 0;
    }
}
