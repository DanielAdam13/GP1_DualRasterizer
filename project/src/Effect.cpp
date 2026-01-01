#include "Effect.h"
#include <iostream>
#include <sstream> // string stream for wstringstream
#include "Texture.h"

Effect::Effect(ID3D11Device* pDevice, const std::wstring& assetPath)
	:m_pEffect{},
	m_pTechnique{},
	m_pWorldViewProjMatrixVariable{},
	m_pDiffuseMapVairable{}
{
	m_pEffect = LoadEffect(pDevice, assetPath);
	if (!m_pEffect)
	{
		std::wcout << L"Effect failed to load: " << assetPath << L"\n";
		return;
	}

	m_pTechnique = m_pEffect->GetTechniqueByName("DefaultTechnique");
	if (!m_pTechnique->IsValid())
	{
		std::wcout << L"Technique not valid!\n";
		m_pTechnique = nullptr;
	}

	m_pWorldViewProjMatrixVariable = m_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
	if (!m_pWorldViewProjMatrixVariable->IsValid())
	{
		std::wcout << L"m_pWorldViewProjMatrixVariable is not valid!\n";
		m_pWorldViewProjMatrixVariable = nullptr;
	}

	m_pDiffuseMapVairable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
	if (!m_pDiffuseMapVairable->IsValid())
	{
		std::wcout << L"m_pDiffuseMapVariable not valid!\n";
		m_pDiffuseMapVairable = nullptr;
	}
}

Effect::~Effect() noexcept
{
	if (m_pEffect)
	{
		m_pEffect->Release();
		m_pEffect = nullptr;
	}

	if (m_pTechnique)
		m_pTechnique = nullptr; // Technique is owned by Effect, thus no need to Release

	if (m_pWorldViewProjMatrixVariable)
		m_pWorldViewProjMatrixVariable = nullptr; // Matrix is owned by Effect --||--

	if (m_pDiffuseMapVairable)
		m_pDiffuseMapVairable = nullptr;
}

ID3DX11Effect* Effect::LoadEffect(ID3D11Device* pDevice, const std::wstring& assetPath)
{
	HRESULT result;
	ID3D10Blob* pErrorBlob{ nullptr };
	ID3DX11Effect* pEffect;

	DWORD shaderFlags{ 0 };

#if defined(DEBUG) || defined(_DEBUG)
	shaderFlags |= D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	result = D3DX11CompileEffectFromFile(assetPath.c_str(),
		nullptr,
		nullptr,
		shaderFlags,
		0,
		pDevice,
		&pEffect,
		&pErrorBlob);

	if (FAILED(result))
	{
		if (pErrorBlob != nullptr)
		{
			const char* pErrors{ static_cast<char*>(pErrorBlob->GetBufferPointer()) };
			std::wstringstream ss{};

			for (int i{ 0 }; i < pErrorBlob->GetBufferSize(); ++i)
			{
				ss << pErrors[i];
			}

			OutputDebugStringW(ss.str().c_str());
			pErrorBlob->Release();
			pErrorBlob = nullptr;

			std::wcout << ss.str() << L"\n";
		}
		else
		{
			std::wstringstream ss{};

			ss << "EffectLoader: Failed to Create Effect From File! \nPath: " << assetPath;
			std::wcout << ss.str() << L"\n";
			return nullptr;
		}
	}

	return pEffect;
}

ID3DX11Effect* Effect::GetEffect() const
{
	return m_pEffect;
}

ID3DX11EffectTechnique* Effect::GetTechnique() const
{
	return (m_pTechnique && m_pTechnique->IsValid()) ? m_pTechnique : nullptr;
}

ID3DX11EffectMatrixVariable* Effect::GetWorldViewProjMatrix() const
{
	return m_pWorldViewProjMatrixVariable;
}

void Effect::SetDiffuseMap(Texture* pDiffuseTexture) // Bind texture's SRV to Effect's SRV
{
	if (m_pDiffuseMapVairable)
	{
		m_pDiffuseMapVairable->SetResource(pDiffuseTexture->GetSRV());
	}
}
