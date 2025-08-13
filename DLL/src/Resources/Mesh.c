#include "pch.h"
#include "Resources/Mesh.h"

bool DROP_CreateVertexBuffer(const GfxHandle handle, const void* vertices, u32 verticesSize, ID3D11Buffer** ppVertexBuffer)
{
    ASSERT_MSG(handle, "Graphics handle is null");
    ASSERT_MSG(vertices, "Vertices are null");
    ASSERT_MSG(ppVertexBuffer, "Vertex buffer pointer is null.");

    *ppVertexBuffer = NULL;

    D3D11_BUFFER_DESC bufferDesc = {
        .ByteWidth           = verticesSize,
        .Usage               = D3D11_USAGE_DEFAULT,
        .BindFlags           = D3D11_BIND_VERTEX_BUFFER,
        .CPUAccessFlags      = 0,
        .MiscFlags           = 0,
        .StructureByteStride = 0};

    D3D11_SUBRESOURCE_DATA bufferSubResource = {
        .pSysMem          = vertices,
        .SysMemPitch      = 0,
        .SysMemSlicePitch = 0};

    ID3D11Buffer* pVertexBuffer = NULL;

    HRESULT hr = handle->pDevice->lpVtbl->CreateBuffer(handle->pDevice, &bufferDesc, &bufferSubResource, &pVertexBuffer);
    if (FAILED(hr) || !pVertexBuffer)
    {
        ASSERT_MSG(false, "Failed to create vertex buffer.");
        return false;
    }

    *ppVertexBuffer = pVertexBuffer;

    return true;
}
bool DROP_CreateInputLayout(
    const GfxHandle handle, const D3D11_INPUT_ELEMENT_DESC* layouts, u32 layoutCount,
    ID3D10Blob* pByteCode, ID3D11InputLayout** ppInputLayout)
{
    ASSERT_MSG(handle, "Graphics handle is null.");
    ASSERT_MSG(layouts, "layouts are null.");
    ASSERT_MSG(pByteCode, "VS bytecode is null.");
    ASSERT_MSG(ppInputLayout, "Input layout pointer is null.");

    *ppInputLayout = NULL;

    ID3D11InputLayout* pInputLayout = NULL;

    HRESULT hr = handle->pDevice->lpVtbl->CreateInputLayout(
        handle->pDevice, layouts, layoutCount, pByteCode->lpVtbl->GetBufferPointer(pByteCode),
        pByteCode->lpVtbl->GetBufferSize(pByteCode), &pInputLayout);
    if (FAILED(hr) || !pInputLayout)
    {
        ASSERT_MSG(false, "Failed to create input layout.");
        return false;
    }

    *ppInputLayout = pInputLayout;

    return true;
}