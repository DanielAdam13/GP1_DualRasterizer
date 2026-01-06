#pragma once
// DirectX Headers
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>

#include "Math.h" // Includes dae structs + DataStructs
#include <vector>

#include "Effect.h"
#include "ShadingEffect.h"
#include "TransparencyEffect.h"

#include "Texture.h"
#include <string>
#include <memory>
#include "Utils.h"

#define SAFE_RELEASE(p) \
if (p) {p->Release(); p = nullptr; }

using namespace dae;

enum class PrimitiveTopology
{
	TriangleList,
	TriangleStrip
};

enum class SamplerType
{
	Point = 0,
	Linear = 1,
	Anisotropic = 2
};

enum class RasterizerState
{
	Hardware = 0,
	Software = 1
};

template <typename EffectType>
class Mesh final
{
public:
	static_assert(
		std::is_base_of_v<Effect, EffectType>,
		"Mesh<EffectType>: EffectType must derive from Effect");

	Mesh(ID3D11Device* pDevice, const std::string& mainBodyMeshOBJ, PrimitiveTopology _primitive,
		const std::string& diffuseTexturePath, const std::string& normalTexturePath = "", const std::string& specularTexturePath = "", const std::string& glossTexturePath = "")
		: m_pEffect{ std::make_unique<EffectType>(pDevice) },
		m_Vertices{},
		m_Indices{},
		m_CurrentTopology{ _primitive },
		m_Position{ 0.f, 0.f, 0.f },
		m_RotY{ 0.f },
		m_Scale{ 1.f, 1.f, 1.f },
		m_WorldMatrix{},
		m_TranslationMatrix{ Matrix::CreateTranslation(m_Position) },
		m_RotationMatrix{ Matrix::CreateRotationY(m_RotY) },
		m_ScaleMatrix{ Matrix::CreateScale(m_Scale) },
		m_pDiffuseTetxure{ std::unique_ptr<Texture>(Texture::LoadFromFile(pDevice, diffuseTexturePath)) },
		m_pNormalTexture{ std::unique_ptr<Texture>(Texture::LoadFromFile(pDevice, normalTexturePath)) },
		m_pSpecularTexture{ std::unique_ptr<Texture>(Texture::LoadFromFile(pDevice, specularTexturePath)) },
		m_pGlossTexture{ std::unique_ptr<Texture>(Texture::LoadFromFile(pDevice, glossTexturePath)) },
		m_CurrentSampler{ m_pPointSampler }
		{
			Utils::ParseOBJ(mainBodyMeshOBJ, m_Vertices, m_Indices);
			m_WorldMatrix = m_ScaleMatrix * m_RotationMatrix * m_TranslationMatrix;
			CreateLayouts(pDevice);
			CreateSamplerStates(pDevice);
		};

	Mesh(ID3D11Device* pDevice, const std::vector<VertexIn>& vertices, const std::vector<uint32_t>& indices, PrimitiveTopology _primitive,
		const std::string& diffuseTexturePath, const std::string& normalTexturePath = "", const std::string& specularTexturePath = "", const std::string& glossTexturePath = "")
		: m_pEffect{ std::make_unique<EffectType>(pDevice) },
		m_Vertices{ vertices },
		m_Indices{ indices },
		m_CurrentTopology{ _primitive },
		m_Position{ 0.f, 0.f, 0.f },
		m_RotY{ 0.f },
		m_Scale{ 1.f, 1.f, 1.f },
		m_WorldMatrix{},
		m_TranslationMatrix{ Matrix::CreateTranslation(m_Position) },
		m_RotationMatrix{ Matrix::CreateRotationY(m_RotY) },
		m_ScaleMatrix{ Matrix::CreateScale(m_Scale) },
		m_pDiffuseTetxure{ std::unique_ptr<Texture>(Texture::LoadFromFile(pDevice, diffuseTexturePath)) },
		m_pNormalTexture{ std::unique_ptr<Texture>(Texture::LoadFromFile(pDevice, normalTexturePath)) },
		m_pSpecularTexture{ std::unique_ptr<Texture>(Texture::LoadFromFile(pDevice, specularTexturePath)) },
		m_pGlossTexture{ std::unique_ptr<Texture>(Texture::LoadFromFile(pDevice, glossTexturePath)) },
		m_CurrentSampler{ m_pPointSampler }
	{
		m_WorldMatrix = m_ScaleMatrix * m_RotationMatrix * m_TranslationMatrix;
		CreateLayouts(pDevice);
		CreateSamplerStates(pDevice);
	};
	~Mesh()
	{
		SAFE_RELEASE(m_pVertexBuffer);
		SAFE_RELEASE(m_pIndexBuffer);
		SAFE_RELEASE(m_pInputLayout);

		SAFE_RELEASE(m_pPointSampler);
		SAFE_RELEASE(m_pLinearSampler);
		SAFE_RELEASE(m_pAnisotropicSampler);
	}

	void Render(RasterizerState currentRasterizerState, const Matrix& viewProjMatrix, Vector3& cameraPos, ID3D11DeviceContext* pDeviceContext, SamplerType samplerType)
	{
		// SHARED
		m_WorldMatrix = m_ScaleMatrix * m_RotationMatrix * m_TranslationMatrix;
		Matrix worldViewProjectionMatrix{ m_WorldMatrix * viewProjMatrix };

		// HARDWARE
		if (currentRasterizerState == RasterizerState::Hardware)
		{
			m_pEffect->GetWorldViewProjMatrix()->SetMatrix(reinterpret_cast<float*>(&worldViewProjectionMatrix));

			// Bind Texture's SRV to GPU's resource view
			m_pEffect->SetDiffuseMap(m_pDiffuseTetxure.get());

			if (m_pNormalTexture)
				m_pEffect->SetNormalMap(m_pNormalTexture.get());

			if (m_pSpecularTexture)
				m_pEffect->SetSpecularMap(m_pSpecularTexture.get());

			if (m_pGlossTexture)
				m_pEffect->SetGlossMap(m_pGlossTexture.get());

			// Set Primitive Topology
			if (m_CurrentTopology == PrimitiveTopology::TriangleList)
			{
				pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			}
			else
			{
				pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			}

			// Set Input Layout
			pDeviceContext->IASetInputLayout(m_pInputLayout);

			// Set Vertex Buffer
			constexpr UINT stride{ sizeof(VertexIn) };
			constexpr UINT offset{ 0 };
			pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

			// Set Index Buffer
			pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

			switch (samplerType)
			{
			case SamplerType::Point:
				m_CurrentSampler = m_pPointSampler;
				break;
			case SamplerType::Linear:
				m_CurrentSampler = m_pLinearSampler;
				break;
			case SamplerType::Anisotropic:
				m_CurrentSampler = m_pAnisotropicSampler;
				break;
			}

			// ----- Apply Technique Pass -----
			D3DX11_TECHNIQUE_DESC techDesc{};
			m_pEffect->GetTechnique()->GetDesc(&techDesc);

			ID3DX11EffectPass* pass = m_pEffect->GetTechnique()->GetPassByIndex(0); // Technique has only one pass
			pass->Apply(0, pDeviceContext);

			// ----- Bind Variables AFTER Technique pass ------
			pDeviceContext->PSSetSamplers(0, 1, &m_CurrentSampler);

			//m_pEffect->ApplyPipelineStates(pDeviceContext);

			const auto effectWorldMatrix{ m_pEffect->GetWorldMatrix() };
			const auto effectCameraPosVector{ m_pEffect->GetCameraPos() };

			if (effectWorldMatrix)
				effectWorldMatrix->SetMatrix(reinterpret_cast<float*>(&m_WorldMatrix));

			if (effectCameraPosVector)
				effectCameraPosVector->SetFloatVector(reinterpret_cast<float*>(&cameraPos));

			// ----- DRAW -----
			pDeviceContext->DrawIndexed(m_NumIndices, 0, 0);
		}
	};

	void Translate(const Vector3& offset)
	{
		m_Position += offset;
		m_TranslationMatrix = Matrix::CreateTranslation(m_Position);
	};
	void RotateY(float yaw)
	{
		m_RotY += yaw;
		m_RotationMatrix = Matrix::CreateRotationY(m_RotY);
	};
	void Scale(const Vector3& scale)
	{
		m_ScaleMatrix = Matrix::CreateScale(scale);
	};

	Matrix GetWorldMatrix() const
	{
		return m_WorldMatrix;
	};

private:
	// Mesh Members
	std::unique_ptr<EffectType> m_pEffect;
	std::vector<VertexIn> m_Vertices;
	std::vector<uint32_t> m_Indices;

	const PrimitiveTopology m_CurrentTopology;

	Vector3 m_Position;
	float m_RotY;
	Vector3 m_Scale;

	Matrix m_WorldMatrix;
	Matrix m_TranslationMatrix;
	Matrix m_RotationMatrix;
	Matrix m_ScaleMatrix;

	const std::unique_ptr<Texture> m_pDiffuseTetxure;
	const std::unique_ptr<Texture> m_pNormalTexture;
	const std::unique_ptr<Texture> m_pSpecularTexture;
	const std::unique_ptr<Texture> m_pGlossTexture;

	void CreateLayouts(ID3D11Device* pDevice)
	{
		// Vertex Layout
		static constexpr uint32_t numElements{ 4 };
		D3D11_INPUT_ELEMENT_DESC vertexDesc[numElements]{};

		vertexDesc[0].SemanticName = "POSITION";
		vertexDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		vertexDesc[0].AlignedByteOffset = offsetof(VertexIn, position); // Starts from offset position OR byte 0 if using float[3]
		vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		vertexDesc[1].SemanticName = "TEXCOORD";
		vertexDesc[1].SemanticIndex = 0;
		vertexDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		vertexDesc[1].AlignedByteOffset = offsetof(VertexIn, UVCoordinate); // Starts from offset UVCoordinate OR +3 +3 floats = 24 bytes if using float[2]
		vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		vertexDesc[2].SemanticName = "NORMAL";
		vertexDesc[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		vertexDesc[2].AlignedByteOffset = offsetof(VertexIn, normal);
		vertexDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		vertexDesc[3].SemanticName = "TANGENT";
		vertexDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		vertexDesc[3].AlignedByteOffset = offsetof(VertexIn, tangent);
		vertexDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		// Input Layout
		D3DX11_PASS_DESC passDesc{};
		m_pEffect->GetTechnique()->GetPassByIndex(0)->GetDesc(&passDesc);

		HRESULT result{ pDevice->CreateInputLayout(
			vertexDesc,
			numElements,
			passDesc.pIAInputSignature,
			passDesc.IAInputSignatureSize,
			&m_pInputLayout) };

		if (FAILED(result))
			return;

		// Vertex buffer
		D3D11_BUFFER_DESC bd{};
		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.ByteWidth = sizeof(VertexIn) * static_cast<uint32_t>(m_Vertices.size());
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData{};
		initData.pSysMem = m_Vertices.data();

		result = pDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer);
		if (FAILED(result))
			return;

		// Index Buffer
		m_NumIndices = static_cast<uint32_t>(m_Indices.size());

		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.ByteWidth = sizeof(uint32_t) * m_NumIndices;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = 0;
		initData.pSysMem = m_Indices.data();
		result = pDevice->CreateBuffer(&bd, &initData, &m_pIndexBuffer);

		if (FAILED(result))
			return;
	};
	void CreateSamplerStates(ID3D11Device* pDevice)
	{
		D3D11_SAMPLER_DESC desc{};
		desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

		// Point
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		m_pPointSampler = nullptr;
		pDevice->CreateSamplerState(&desc, &m_pPointSampler);

		// Linear
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		m_pLinearSampler = nullptr;
		pDevice->CreateSamplerState(&desc, &m_pLinearSampler);

		// Anisotropic
		desc.Filter = D3D11_FILTER_ANISOTROPIC;
		desc.MaxAnisotropy = 16;
		m_pAnisotropicSampler = nullptr;
		pDevice->CreateSamplerState(&desc, &m_pAnisotropicSampler);
	}

	// Direct X Resources
	ID3D11InputLayout* m_pInputLayout{};
	ID3D11Buffer* m_pVertexBuffer{};
	ID3D11Buffer* m_pIndexBuffer{};
	uint32_t m_NumIndices{};

	ID3D11SamplerState* m_pPointSampler{};
	ID3D11SamplerState* m_pLinearSampler{};
	ID3D11SamplerState* m_pAnisotropicSampler{};
	ID3D11SamplerState* m_CurrentSampler{};

};