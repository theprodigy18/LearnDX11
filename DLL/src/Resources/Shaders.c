#include "pch.h"
#include "Resources/Shaders.h"

bool DROP_CreateVertexShader(ID3D11Device* const pDevice, const wchar_t* fileName,
                             ID3D11VertexShader** ppVertexShader, ID3DBlob** ppByteCode)
{
    ASSERT_MSG(pDevice, "Graphics handle is null.");
    ASSERT_MSG(fileName, "Filename is null.");
    ASSERT_MSG(ppVertexShader, "Vertex shader is null.");

    *ppVertexShader = NULL;
    if (ppByteCode) *ppByteCode = NULL;

    ID3DBlob* pByteCode = NULL;

    HRESULT hr = D3DReadFileToBlob(fileName, &pByteCode);
    if (FAILED(hr) || !pByteCode)
    {
        ASSERT_MSG(false, "Failed to read vertex shader file.");
        return false;
    }

    ID3D11VertexShader* pVertexShader = NULL;

    hr = pDevice->lpVtbl->CreateVertexShader(
        pDevice, pByteCode->lpVtbl->GetBufferPointer(pByteCode),
        pByteCode->lpVtbl->GetBufferSize(pByteCode), NULL, &pVertexShader);
    if (FAILED(hr) || !pVertexShader)
    {
        ASSERT_MSG(false, "Failed to create vertex shader.");
        RELEASE(pByteCode);
        return false;
    }

    *ppVertexShader = pVertexShader;
    if (ppByteCode)
        *ppByteCode = pByteCode;
    else
        RELEASE(pByteCode);

    return true;
}

bool DROP_CreatePixelShader(ID3D11Device* const pDevice, const wchar_t* fileName, ID3D11PixelShader** ppPixelShader)
{
    ASSERT_MSG(pDevice, "Graphics handle is null.");
    ASSERT_MSG(fileName, "Filename is null.");
    ASSERT_MSG(ppPixelShader, "Vertex shader is null.");

    *ppPixelShader = NULL;

    ID3DBlob* pByteCode = NULL;

    HRESULT hr = D3DReadFileToBlob(fileName, &pByteCode);
    if (FAILED(hr) || !pByteCode)
    {
        ASSERT_MSG(false, "Failed to read pixel shader file.");
        return false;
    }

    ID3D11PixelShader* pPixelShader = NULL;

    hr = pDevice->lpVtbl->CreatePixelShader(
        pDevice, pByteCode->lpVtbl->GetBufferPointer(pByteCode),
        pByteCode->lpVtbl->GetBufferSize(pByteCode), NULL, &pPixelShader);
    RELEASE(pByteCode);
    if (FAILED(hr) || !pPixelShader)
    {
        ASSERT_MSG(false, "Failed to read pixel shader file.");
        return false;
    }

    *ppPixelShader = pPixelShader;

    return true;
}