#include "TransparencyEffect.h"
#include <iostream>
#include <sstream> // string stream for wstringstream
#include "Texture.h"

TransparencyEffect::TransparencyEffect(ID3D11Device* pDevice)
	:Effect::Effect(pDevice, L"resources/PartCoverage.fx")
{
	//D3D11_RASTERIZER_DESC rsDesc{};
	//rsDesc.CullMode = D3D11_CULL_NONE;        // for transparency / double-sided
	//rsDesc.FrontCounterClockwise = FALSE;

	//ID3D11RasterizerState* m_pRasterizerNoCull{};

	//HRESULT hr = pDevice->CreateRasterizerState(&rsDesc, &m_pRasterizerNoCull);
	//if (FAILED(hr))
	//{
	//	std::wcout << l"Failed to Create Rasterizer State\n";
	//}

	//D3D11_BLEND_DESC blendDesc{};
	//blendDesc.RenderTarget[0].BlendEnable = TRUE;
	//blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	//blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	//blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	//blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	//blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	//blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	//blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	//pDevice->CreateBlendState(&blendDesc, &m_pBlendState);
}

void TransparencyEffect::ApplyPipelineStates(ID3D11DeviceContext* pDevContext)
{
	/*pDevContext->RSSetState(m_pRasterizerStateNoCull);

	float blendFactor[4]{};
	pDevContext->OMSetBlendState(m_pBlendState, blendFactor, 0xFFFFFFFF);*/
}
