#pragma once

#include "Platform/Window.h"

typedef struct _GfxHandle
{
    ID3D11Device*           pDevice;
    ID3D11DeviceContext*    pContext;
    IDXGISwapChain*         pSwapChain;
    ID3D11RenderTargetView* pBackBufferRTV;
} _GfxHandle;

typedef _GfxHandle* GfxHandle;

typedef struct _GfxInitProps
{
    WndHandle wndHandle;
} GfxInitProps;

bool DROP_CreateGraphics(const GfxInitProps* pProps, GfxHandle* pHandle);
void DROP_DestroyGraphics(GfxHandle* pHandle);
