#pragma once
#include "Effect.h"

class OpaqueEffect final : public Effect
{
public:
	explicit OpaqueEffect(ID3D11Device* pDevice);
	OpaqueEffect(OpaqueEffect& other) = delete;
	OpaqueEffect(const OpaqueEffect& other) = delete;
	OpaqueEffect(OpaqueEffect&& effect) = delete;
	virtual ~OpaqueEffect() noexcept;
	
	virtual ID3DX11EffectMatrixVariable* GetWorldMatrix() const override;
	virtual ID3DX11EffectVectorVariable* GetCameraPos() const override;

	virtual void SetNormalMap(Texture* pNormalTexture) override;
	virtual void SetSpecularMap(Texture* pSpecularTexture) override;
	virtual void SetGlossMap(Texture* pGlossTexture) override;
	
private:

	ID3DX11EffectShaderResourceVariable* m_pNormalMapVairable{};
	ID3DX11EffectShaderResourceVariable* m_pSpecularMapVairable{};
	ID3DX11EffectShaderResourceVariable* m_pGlossMapVairable{};

	// Shading Variables
	ID3DX11EffectMatrixVariable* m_pWorldMatrixVariable{};
	ID3DX11EffectVectorVariable* m_pCameraPosVariable{};
};