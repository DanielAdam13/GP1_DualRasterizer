#pragma once
#include <SDL_surface.h>
#include <string>
// DirectX Headers
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>

#include "ColorRGB.h"
#include "Vector2.h"

using namespace dae;

class Texture final
{
public:
	~Texture();
	static Texture* LoadFromFile(ID3D11Device* device, const std::string& filePath);

	ID3D11ShaderResourceView* GetSRV() const;

	// --- SOFTWARE ---
	ColorRGB Sample(const Vector2& uv) const;

private:
	Texture(SDL_Surface* pSurface);

	ID3D11Texture2D* m_pResourceTexture{};
	ID3D11ShaderResourceView* m_pSRV{};

	// --- SOFTWARE ---
	SDL_Surface* m_pSurface;
	uint32_t* m_pSurfacePixels;

};
