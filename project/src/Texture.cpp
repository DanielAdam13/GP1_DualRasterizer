#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>
#include <fstream>
#include <iostream>

using namespace dae;

#define SAFE_RELEASE(p) \
if (p) {p->Release(); p = nullptr; }

Texture::Texture(SDL_Surface * pSurface) :
	m_pSurface{ pSurface },
	m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
{
}

Texture::~Texture()
{
	SAFE_RELEASE(m_pSRV);
	SAFE_RELEASE(m_pResourceTexture);

	if (m_pSurface)
	{
		SDL_FreeSurface(m_pSurface);
		m_pSurface = nullptr;
		m_pSurfacePixels = nullptr;
	}
}

Texture* Texture::LoadFromFile(ID3D11Device* device, const std::string& filePath)
{
	SDL_Surface* surface{ IMG_Load(filePath.c_str()) };

	if (!surface)
	{
		std::cout << "Cannot load texture surface from invalid path \n";
		return nullptr;
	}

	Texture* newTexture{ new Texture(surface) };

	// ----- Create Texture2D -----
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = surface->w;
	desc.Height = surface->h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = surface->pixels;
	initData.SysMemPitch = static_cast<UINT>(surface->pitch);
	initData.SysMemSlicePitch = static_cast<UINT>(surface->h * surface->pitch);

	HRESULT result{ device->CreateTexture2D(&desc, &initData, &newTexture->m_pResourceTexture) };
	if (FAILED(result))
	{
		std::wcout << "Failed to create Texture2D\n";
		// Free resources correctly
		SDL_FreeSurface(surface);
		delete newTexture;
		return nullptr;
	}

	// ----- Create SRV -----
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	result = device->CreateShaderResourceView(newTexture->m_pResourceTexture, &srvDesc, &newTexture->m_pSRV);
	if (FAILED(result))
	{
		std::wcout << "Failed to create SRV\n";
		// Free resources correctly
		SDL_FreeSurface(surface);
		delete newTexture;
		return nullptr;
	}

	//SDL_FreeSurface(surface); // Not needed anymore

	return newTexture;
}

ID3D11ShaderResourceView* Texture::GetSRV() const
{
	return m_pSRV;
}

ColorRGB Texture::Sample(const Vector2& uv) const
{
	int x{ static_cast<int>(uv.x * m_pSurface->w) };
	int y{ static_cast<int>(uv.y * m_pSurface->h) };

	int pixelIndex{ x + y * m_pSurface->w };
	uint32_t pixel{ m_pSurfacePixels[pixelIndex] };

	uint8_t r, g, b;
	SDL_GetRGB(pixel, m_pSurface->format, &r, &g, &b);

	return ColorRGB(r / 255.f, g / 255.f, b / 255.f);
}
