#pragma once
#include "Effect.h"

class ShadingEffect final : public Effect
{
public:
	explicit ShadingEffect(ID3D11Device* pDevice);
	ShadingEffect(ShadingEffect& other) = delete;
	ShadingEffect(const ShadingEffect& other) = delete;
	ShadingEffect(ShadingEffect&& effect) = delete;
	virtual ~ShadingEffect() noexcept;
	
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