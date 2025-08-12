#pragma once

#include "Graphics/Graphics.h"

bool DROP_CreateVertexShader(ID3D11Device* const pDevice, const wchar_t* fileName,
                             ID3D11VertexShader** ppVertexShader, ID3DBlob** ppByteCode);
bool DROP_CreatePixelShader(ID3D11Device* const pDevice, const wchar_t* fileName, ID3D11PixelShader** ppPixelShader);