#pragma once
// SDL Headers
#include "SDL.h"
#include "SDL_syswm.h"
#include "SDL_surface.h"
#include "SDL_image.h"
// DirectX Headers
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>
// Framework Headers
#include "Timer.h"

#include <vector>
#include "Mesh.h" // Includes Mesh + dae structs + DataStructs
#include "Camera.h"

namespace dae
{
	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(const Timer* pTimer);
		void Render();

	private:
		SDL_Window* m_pWindow{};

		int m_Width{};
		int m_Height{};

		Camera m_Camera{};
		std::vector<std::unique_ptr<Mesh<ShadingEffect>>> m_OpaqueMeshes{};
		std::vector<std::unique_ptr<Mesh<TransparencyEffect>>> m_TransparentMeshes{};
		SamplerType m_CurrentSamplerType;

		// --- SOFTWARE ---
		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		std::vector<VertexOut> m_TransformedMeshVertices{};

		void RenderSoftwareMesh(const MeshBase& mesh, const Matrix& viewProjMatrix);

		void VertexTransformationFunction(const std::vector<VertexIn>& vertices_in, std::vector<VertexOut>& vertices_out, 
			const Matrix& WVPMatrix, const Matrix& meshWorldMatrix) const;

		bool PassTriangleOptimizations(const std::array<VertexOut, 3> screenTri);

		ColorRGB PixelShading(const VertexIn& pixel, const MeshBase& mesh, const ColorRGB& pixelColor) const;

		void FillRectangle(int x0, int y0, int x1, int y1, const ColorRGB& color);

		// --- HARDWARE ---
		bool m_IsDXInitialized{ false };

		std::unique_ptr<ShadingEffect> m_pOpaqueEffect;
		std::unique_ptr<TransparencyEffect> m_pTransparencyEffect;

		ID3D11SamplerState* m_pPointSampler{};
		ID3D11SamplerState* m_pLinearSampler{};
		ID3D11SamplerState* m_pAnisotropicSampler{};
		ID3D11SamplerState* m_CurrentSampler{};

		void CreateSamplerStates(ID3D11Device* pDevice);

		//DIRECTX
		HRESULT InitializeDirectX();
		ID3D11Device* m_pDevice{};
		ID3D11DeviceContext* m_pDeviceContext{};

		IDXGISwapChain* m_pSwapChain{};

		ID3D11Texture2D* m_pDepthStencilBuffer{};
		ID3D11DepthStencilView* m_pDepthStencilView{};

		ID3D11Texture2D* m_pRenderTargetBuffer{};
		ID3D11RenderTargetView* m_pRenderTargetView{};

		void ProcessInput();

		// CONDITIONS
		bool m_RotationFrozen;

		RasterizerState m_CurrentRasterizerState;

		bool m_ShowFireMesh;

		bool m_UniformClearColorActive;

		// --- SOFTWARE ---
		enum class LightingMode
		{
			ObservedArea,
			Diffuse,
			Specular,
			Combined
		};
		LightingMode m_CurrentLightingMode;

		bool m_ShowNormalMap;

		enum class PixelColorState
		{
			FinalColor,
			DepthBuffer
		};
		PixelColorState m_CurrentPixelColorState;

		bool m_ShowBoundingBox;
	};
}
