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
    if (SUCCEEDED(hr))
    {
        pInfoQueue->lpVtbl->SetBreakOnSeverity(pInfoQueue, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->lpVtbl->SetBreakOnSeverity(pInfoQueue, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->lpVtbl->SetBreakOnSeverity(pInfoQueue, D3D11_MESSAGE_SEVERITY_WARNING, TRUE);

        pInfoQueue->lpVtbl->Release(pInfoQueue);
    }
#endif // DEBUG

    ID3D11Buffer* pBackBuffer = NULL;

    hr = pSwapChain->lpVtbl->GetBuffer(pSwapChain, 0, &IID_ID3D11Texture2D, (void**) &pBackBuffer);
    if (FAILED(hr) || !pBackBuffer)
    {
        ASSERT_MSG(false, "Failed to get back buffer from swapchain.");
        pSwapChain->lpVtbl->Release(pSwapChain);
        pContext->lpVtbl->Release(pContext);
        pDevice->lpVtbl->Release(pDevice);
        return false;
    }

    ID3D11RenderTargetView* pRTV = NULL;

    hr = pDevice->lpVtbl->CreateRenderTargetView(pDevice, (ID3D11Resource*) pBackBuffer, NULL, &pRTV);
    pBackBuffer->lpVtbl->Release(pBackBuffer);
    if (FAILED(hr) || !pRTV)
    {
        ASSERT_MSG(false, "Failed to create render target view.");
        pSwapChain->lpVtbl->Release(pSwapChain);
        pContext->lpVtbl->Release(pContext);
        pDevice->lpVtbl->Release(pDevice);
        return false;
    }

    GfxHandle handle = (GfxHandle) DROP_Allocate(PERSISTENT, sizeof(_GfxHandle));
    if (!handle)
    {
        ASSERT_MSG(false, "Failed to allocate memory for handle.");
        pSwapChain->lpVtbl->Release(pSwapChain);
        pContext->lpVtbl->Release(pContext);
        pDevice->lpVtbl->Release(pDevice);
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