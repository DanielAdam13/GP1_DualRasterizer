#include "TransparencyEffect.h"
#include <iostream>
#include <sstream> // string stream for wstringstream
#include "Texture.h"

TransparencyEffect::TransparencyEffect(ID3D11Device* pDevice)
	:Effect::Effect(pDevice, L"resources/PartCoverage.fx")
{
}
