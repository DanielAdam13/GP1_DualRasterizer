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
#include "Mesh.h" // Includes Mesh + dae structs + DataStructs + important enum classes
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
		std::vector<std::unique_ptr<Mesh<OpaqueEffect>>> m_OpaqueMeshes{};
		std::vector<std::unique_ptr<Mesh<TransparencyEffect>>> m_TransparentMeshes{};

		// --- SOFTWARE ---
		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		std::unique_ptr<float[]> m_pDepthBufferPixels{};

		std::vector<VertexOut> m_TransformedMeshVertices{};

		template <typename MeshType>
		void RenderSoftwareMesh(const MeshType& mesh, const Matrix& viewProjMatrix)
		{
			Matrix worldViewProjectionMatrix{ mesh.GetWorldMatrix() * viewProjMatrix };

			m_TransformedMeshVertices.clear();

			VertexTransformationFunction(mesh.GetVertices(), m_TransformedMeshVertices, worldViewProjectionMatrix, mesh.GetWorldMatrix());

			const auto& meshIndices{ mesh.GetIndices() };

			for (size_t i{}; i < meshIndices.size(); i += 3)
			{
				std::array<VertexOut, 3> screenTri{ m_TransformedMeshVertices[meshIndices[i]],
					m_TransformedMeshVertices[meshIndices[i + 1]],
					m_TransformedMeshVertices[meshIndices[i + 2]] };

				if (!PassTriangleOptimizations(screenTri))
					continue; // Skip triangle

				// ---- Bounding Box -----
				std::pair<int, int> topLeft{ static_cast<int>(std::floor(
					std::min({screenTri[0].position.x, screenTri[1].position.x, screenTri[2].position.x}))),
					static_cast<int>(std::floor(
						std::min({screenTri[0].position.y, screenTri[1].position.y, screenTri[2].position.y}))) };
				topLeft.first = std::max(topLeft.first, 0);
				topLeft.second = std::max(topLeft.second, 0);

				std::pair<int, int> bottomRight{ static_cast<int>(std::ceil(
					std::max({screenTri[0].position.x, screenTri[1].position.x, screenTri[2].position.x}))),
					static_cast<int>(std::ceil(
						std::max({screenTri[0].position.y, screenTri[1].position.y, screenTri[2].position.y}))) };
				bottomRight.first = std::min(bottomRight.first, m_Width - 1);
				bottomRight.second = std::min(bottomRight.second, m_Height - 1);

				if (m_ShowBoundingBox)
				{
					FillRectangle(topLeft.first, topLeft.second, bottomRight.first, bottomRight.second, ColorRGB{ 1.f, 1.f, 1.f });
				}
				else
				{
					// PIXEL LOOP - Bounding Box
					for (int px{ topLeft.first }; px <= bottomRight.first; ++px)
					{
						for (int py{ int(topLeft.second) }; py <= bottomRight.second; ++py)
						{
							ColorRGB finalColor{};

							VertexIn pixel{ Vector3{ static_cast<float>(px) + 0.5f,
								static_cast<float>(py) + 0.5f, 1.f} }; // We check from the center of the pixel, hence +0.5f

							std::array<float, 3> triangleAreaRatios;

							// INSIDE - OUTSIDE TEST + Depth Interpolation
							bool pixelInTriangle{ IsPixelIn_Triangle(screenTri, pixel, triangleAreaRatios) };

							if (pixelInTriangle)
							{
								int currentPixelNr{ GetPixelNumber(px, py, m_Width) };

								// Depth Test
								if (pixel.position.z < m_pDepthBufferPixels[currentPixelNr])
								{
									if (m_CurrentCullMode != CullMode::Front)
									{
										// Depth Write
										m_pDepthBufferPixels[currentPixelNr] = pixel.position.z;
									}

									// UV, Normal, Tangent, ViewDirection Interpolation
									InterpolateVertex(triangleAreaRatios, screenTri, pixel);

									ColorRGB pixelColor{ mesh.GetDiffuseTexture()->Sample(pixel.UVCoordinate) };

									// ----- SHADING -----
									pixelColor = PixelShading(pixel, mesh, pixelColor);

									switch (m_CurrentPixelColorState)
									{
									case dae::Renderer::PixelColorState::FinalColor:
										finalColor = pixelColor;
										break;
									case dae::Renderer::PixelColorState::DepthBuffer:
										ColorRGB depthValue{ RemapValue(pixel.position.z, 0.997f) };
										finalColor = depthValue;
										break;
									}

									// ---- Render only if overwriting pixel ----
									//Update Color in Buffer
									finalColor.MaxToOne();

									m_pBackBufferPixels[currentPixelNr] = SDL_MapRGB(m_pBackBuffer->format,
										static_cast<uint8_t>(finalColor.r * 255),
										static_cast<uint8_t>(finalColor.g * 255),
										static_cast<uint8_t>(finalColor.b * 255));
								}
							}
						}
					}
				}
			}
		}

		void VertexTransformationFunction(const std::vector<VertexIn>& vertices_in, std::vector<VertexOut>& vertices_out, 
			const Matrix& WVPMatrix, const Matrix& meshWorldMatrix);

		bool PassTriangleOptimizations(const std::array<VertexOut, 3> screenTri) const;

		template <typename MeshType>
		ColorRGB PixelShading(const VertexIn& pixel, const MeshType& mesh, const ColorRGB& pixelColor) const
		{
			ColorRGB finalShadedColor{};
			const Vector3 lightDirection{ -Vector3{0.577f, -0.577f, 0.577f}.Normalized() }; // Inverted Light
			constexpr float lightIntensity{ 1.f };

			Vector3 finalNormal{ pixel.normal };

			// Normal Map Sampling
			if (mesh.GetNormalTexture() && m_ShowNormalMap)
			{
				const Vector3 binormal{ Vector3::Cross(pixel.normal, pixel.tangent) };

				// Tangent -> World
				const Matrix tangentSpaceMatrix{ pixel.tangent, binormal, pixel.normal, {} };

				// Get Normal from map
				const ColorRGB sampledNormalColor{ mesh.GetNormalTexture()->Sample(pixel.UVCoordinate) };

				// Convert Normal to Tangent space
				const Vector3 tangentSpaceNormal{
					sampledNormalColor.r * 2.f - 1.f,
					sampledNormalColor.g * 2.f - 1.f,
					sampledNormalColor.b * 2.f - 1.f
				};

				// Tangent Space to World Space USING the TBN matrix
				finalNormal = tangentSpaceMatrix.TransformVector(tangentSpaceNormal).Normalized();
			}

			// Lambert Diffuse
			const float cosTheta{ std::min(1.f, std::max(0.f, Vector3::Dot(lightDirection, finalNormal))) };
			const ColorRGB lambertDiffuse{ pixelColor };


			const ColorRGB lambertColor(GetLambertColor(lambertDiffuse, cosTheta) * lightIntensity);

			// Specular Color
			ColorRGB specularColor{ colors::Black };
			if (m_CurrentLightingMode == LightingMode::Specular || m_CurrentLightingMode == LightingMode::Combined)
			{
				if (mesh.GetSpecularTexture() && mesh.GetGlossTexture())
				{
					const ColorRGB sampledSpecular{ mesh.GetSpecularTexture()->Sample(pixel.UVCoordinate) };
					const ColorRGB sampledGlossiness{ mesh.GetGlossTexture()->Sample(pixel.UVCoordinate) };

					constexpr float shininess{ 25.f };
					const float phongExponent{ sampledGlossiness.r * shininess };

					specularColor = Phong(sampledSpecular, sampledSpecular.r, phongExponent,
						lightDirection, pixel.viewDirection.Normalized(), finalNormal) * lightIntensity;
				}
			}

			// Ambient Color
			constexpr float ambientStrength{ 0.025f };
			const ColorRGB ambientColor{ lambertDiffuse * ambientStrength };
			finalShadedColor += ambientColor;

			switch (m_CurrentLightingMode)
			{
			case dae::Renderer::LightingMode::ObservedArea:
				finalShadedColor = ColorRGB{ cosTheta, cosTheta, cosTheta };
				break;
			case dae::Renderer::LightingMode::Diffuse:
				finalShadedColor = lambertColor;
				break;
			case dae::Renderer::LightingMode::Specular:
				finalShadedColor = specularColor;
				break;
			case dae::Renderer::LightingMode::Combined:
				finalShadedColor = lambertColor + specularColor;
				break;
			}

			return finalShadedColor;
		}

		void FillRectangle(int x0, int y0, int x1, int y1, const ColorRGB& color) const;

		// --- HARDWARE ---
		bool m_IsDXInitialized{ false };

		std::unique_ptr<OpaqueEffect> m_pOpaqueEffect;
		std::unique_ptr<TransparencyEffect> m_pTransparencyEffect;

		ID3D11SamplerState* m_pPointSampler{};
		ID3D11SamplerState* m_pLinearSampler{};
		ID3D11SamplerState* m_pAnisotropicSampler{};
		ID3D11SamplerState* m_CurrentSampler{};

		ID3D11RasterizerState* m_pRasterizerBack{};
		ID3D11RasterizerState* m_pRasterizerFront{};
		ID3D11RasterizerState* m_pRasterizerNone{};
		ID3D11RasterizerState* m_pCurrentRasterizer{};

		void CreateSamplerStates(ID3D11Device* pDevice);
		void CreateRasterizerStates(ID3D11Device* pDevice);

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
		// --- SHARED ---
		bool m_RotationFrozen;

		RasterizerMode m_CurrentRasterizerMode;

		bool m_UniformClearColorActive;

		CullMode m_CurrentCullMode;
		
		// --- HARDWARE ---
		bool m_ShowFireMesh;
		SamplerType m_CurrentSamplerType;

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
