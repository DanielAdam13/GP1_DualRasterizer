#pragma once
#include "Effect.h"

class TransparencyEffect final : public Effect
{
public:
	explicit TransparencyEffect(ID3D11Device* pDevice);
	TransparencyEffect(TransparencyEffect& other) = delete;
	TransparencyEffect(const TransparencyEffect& other) = delete;
	TransparencyEffect(TransparencyEffect&& effect) = delete;
	virtual ~TransparencyEffect() noexcept = default;

	virtual void ApplyPipelineStates(ID3D11DeviceContext* pDevContext) override;

private:
	//ID3D11RasterizerState* m_pRasterizerStateNoCull{};
	//ID3D11BlendState* m_pBlendState{};
};