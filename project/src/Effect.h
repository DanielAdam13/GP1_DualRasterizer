#pragma once
// DirectX Headers
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>

class Texture;
#include <string>

class Effect
{
public:
	Effect() = default;
	Effect(Effect& other) = delete;
	Effect(const Effect& other) = delete;
	Effect(Effect&& rffect) = delete;
	virtual ~Effect() noexcept;

	virtual ID3DX11Effect* GetEffect() const;
	virtual ID3DX11EffectTechnique* GetTechnique() const;
	virtual ID3DX11EffectMatrixVariable* GetWorldViewProjMatrix() const;

	virtual void SetDiffuseMap(Texture* pDiffuseTexture);

	enum class EffectType
	{
		Opaque,
		Transparent
	};
	
	// OpaqueEffect No-op Functions
	virtual ID3DX11EffectMatrixVariable* GetWorldMatrix() const { return nullptr; };
	virtual ID3DX11EffectVectorVariable* GetCameraPos() const { return nullptr; };

	virtual void SetNormalMap(Texture* pNormalTexture) {};
	virtual void SetSpecularMap(Texture* pSpecularTexture) {};
	virtual void SetGlossMap(Texture* pGlossTexture) {};

	// TransparencyEffect No-op Functions
	virtual void ApplyPipelineStates(ID3D11DeviceContext* pDevContext) {};

	

protected:
	Effect(ID3D11Device* pDevice, const std::wstring& assetPath);
	ID3DX11Effect* m_pEffect;
	ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring& assetPath);

	ID3DX11EffectTechnique* m_pTechnique;

	ID3DX11EffectMatrixVariable* m_pWorldViewProjMatrixVariable;

	ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVairable;

private:
	
};