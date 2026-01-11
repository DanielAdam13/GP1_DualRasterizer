#pragma once
#include "Effect.h"

class TransparencyEffect final : public Effect
{
public:
	explicit TransparencyEffect(ID3D11Device* pDevice);

	TransparencyEffect(const TransparencyEffect& other) = delete;
	TransparencyEffect(TransparencyEffect&& effect) = delete;
	TransparencyEffect& operator=(const TransparencyEffect&) = delete;
	TransparencyEffect& operator=(TransparencyEffect&&) noexcept = delete;

	virtual ~TransparencyEffect() noexcept = default;

	//virtual void ApplyPipelineStates(ID3D11DeviceContext* pDevContext) override;

private:
};