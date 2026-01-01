#pragma once
#include <SDL_surface.h>
#include <string>
// DirectX Headers
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>

class Texture final
{
public:
	~Texture();
	static Texture* LoadFromFile(ID3D11Device* device, const std::string& filePath);

	ID3D11ShaderResourceView* GetSRV() const;

private:
	Texture();

	ID3D11Texture2D* m_pResourceTexture{};
	ID3D11ShaderResourceView* m_pSRV{};

};