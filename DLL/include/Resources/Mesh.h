#pragma once

#include "Graphics/Graphics.h"

bool DROP_CreateVertexBuffer(const GfxHandle handle, const void* vertices, u32 verticesSize, ID3D11Buffer** ppVertexBuffer);
bool DROP_CreateInputLayout(
    const GfxHandle handle, const D3D11_INPUT_ELEMENT_DESC* layouts, u32 layoutCount,
    ID3D10Blob* pByteCode, ID3D11InputLayout** ppInputLayout);