#include "ShadingEffect.h"
#include <iostream>
#include <sstream> // string stream for wstringstream
#include "Texture.h"

ShadingEffect::ShadingEffect(ID3D11Device* pDevice)
	: Effect::Effect(pDevice, L"resources/PosCol3D.fx")
{
	m_pNormalMapVairable = m_pEffect->GetVariableByName("gNormalMap")->AsShaderResource();
	if (!m_pNormalMapVairable->IsValid())
	{
		std::wcout << L"m_pNormalMapVariable not valid!\n";
		m_pNormalMapVairable = nullptr;
	}

	m_pSpecularMapVairable = m_pEffect->GetVariableByName("gSpecularMap")->AsShaderResource();
	if (!m_pSpecularMapVairable->IsValid())
	{
		std::wcout << L"m_pSpecularMapVariable not valid!\n";
		m_pSpecularMapVairable = nullptr;
	}

	m_pGlossMapVairable = m_pEffect->GetVariableByName("gGlossinessMap")->AsShaderResource();
	if (!m_pGlossMapVairable->IsValid())
	{
		std::wcout << L"m_pGlossMapVariable not valid!\n";
		m_pGlossMapVairable = nullptr;
	}

	m_pWorldMatrixVariable = m_pEffect->GetVariableByName("gWorldMatrix")->AsMatrix();
	if (!m_pWorldMatrixVariable->IsValid())
	{
		std::wcout << L"m_pWorldMatrixVariable is not valid!\n";
		m_pWorldMatrixVariable = nullptr;
	}

	m_pCameraPosVariable = m_pEffect->GetVariableByName("gCameraPos")->AsVector();
	if (!m_pCameraPosVariable->IsValid())
	{
		std::wcout << L"m_pCameraPosVariable is not valid!\n";
		m_pCameraPosVariable = nullptr;
	}
}

ShadingEffect::~ShadingEffect()
{
	// Variables owned by ShadingEffect => no need to release
	if (m_pWorldMatrixVariable)
		m_pWorldMatrixVariable = nullptr;

	if (m_pCameraPosVariable)
		m_pCameraPosVariable = nullptr;

	if (m_pNormalMapVairable)
		m_pNormalMapVairable = nullptr;

	if (m_pSpecularMapVairable)
		m_pSpecularMapVairable = nullptr;

	if (m_pGlossMapVairable)
		m_pGlossMapVairable = nullptr;
}

ID3DX11EffectMatrixVariable* ShadingEffect::GetWorldMatrix() const
{
	return m_pWorldMatrixVariable;
}

ID3DX11EffectVectorVariable* ShadingEffect::GetCameraPos() const
{
	return m_pCameraPosVariable;
}

void ShadingEffect::SetNormalMap(Texture* pNormalTexture)
{
	if (m_pNormalMapVairable && pNormalTexture)
	{
		m_pNormalMapVairable->SetResource(pNormalTexture->GetSRV());
	}
}

void ShadingEffect::SetSpecularMap(Texture* pSpecularTexture)
{
	if (m_pSpecularMapVairable && pSpecularTexture)
	{
		m_pSpecularMapVairable->SetResource(pSpecularTexture->GetSRV());
	}
}

void ShadingEffect::SetGlossMap(Texture* pGlossTexture)
{
	if (m_pGlossMapVairable && pGlossTexture)
	{
		m_pGlossMapVairable->SetResource(pGlossTexture->GetSRV());
	}
}
