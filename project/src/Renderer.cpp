//External includes
#include "SDL.h"
#include "SDL_surface.h"

// Standard includes
#include <iostream>

//Project includes
#include "Renderer.h"

#include "Utils.h"
#include "Effect.h"

#define SAFE_RELEASE(p) \
if (p) {p->Release(); p = nullptr; }

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow),
	m_RotationFrozen{ true },
	m_CurrentRasterizerMode{ RasterizerMode::Hardware },
	m_UniformClearColorActive{ false },
	m_CurrentCullMode{ CullMode::Back },
	m_ShowFireMesh{ m_CurrentRasterizerMode == RasterizerMode::Hardware },
	m_CurrentSamplerType{ SamplerType::Point },
	m_CurrentLightingMode{ LightingMode::Combined },
	m_ShowNormalMap{ true },
	m_CurrentPixelColorState{ PixelColorState::FinalColor },
	m_ShowBoundingBox{ false }
{
	// Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	// --- SOFTWARE ---
	// Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = std::make_unique<float[]>(m_Width * m_Height);
	std::fill_n(m_pDepthBufferPixels.get(), m_Width * m_Height, std::numeric_limits<float>::max()); // Depth buffer elements are initalized with float max

	// Initialize DirectX pipeline
	const HRESULT result = InitializeDirectX();
	if (result == S_OK)
	{
		m_IsDXInitialized = true;

		CreateSamplerStates(m_pDevice);
		CreateRasterizerStates(m_pDevice);

		std::cout << "DirectX is initialized and ready!\n";
	}
	else
	{
		std::cout << "DirectX initialization failed!\n";
	}


	m_Camera.Initialize(45.f, { 0.f, 0.f, 0.f }, 0.1f, 100.f);

	// Initial Mesh costructor doesn't care about Software or Hardware states
	m_OpaqueMeshes.reserve(2);
	m_OpaqueMeshes.emplace_back(std::make_unique<Mesh<OpaqueEffect>>(
		m_pDevice,
		"resources/vehicle.obj",
		PrimitiveTopology::TriangleList,
		"resources/vehicle_diffuse.png",
		"resources/vehicle_normal.png",
		"resources/vehicle_specular.png",
		"resources/vehicle_gloss.png"
	));
	m_OpaqueMeshes[0]->Translate({ 0.f, 0.f, 50.f });

	m_TransparentMeshes.reserve(1);
	m_TransparentMeshes.emplace_back(std::make_unique<Mesh<TransparencyEffect>>(
		m_pDevice,
		"resources/fireFX.obj",
		PrimitiveTopology::TriangleList,
		"resources/fireFX_diffuse.png"));
	m_TransparentMeshes[0]->Translate({ 0.f, 0.f, 50.f });

	std::wcout << L"\n[Key Bindings - SHARED] \n [F1]  Toggle Rasterizer Mode(HARDWARE / SOFTWARE) \n [F2]  Toggle Vehicle Rotation(ON / OFF) \n [F9]  Cycle CullMode(BACK / FRONT / NONE) \n";
	std::wcout << L" [F10] Toggle Uniform ClearColor(ON / OFF) \n [F11] Toggle Print FPS(ON / OFF) \n\n[Key Bindings - HARDWARE] \n [F3] Toggle FireFX(ON / OFF) \n";
	std::wcout << L" [F4] Cycle Sampler State(POINT / LINEAR / ANISOTROPIC) \n\n[Key Bindings - SOFTWARE] \n [F5] Cycle Shading Mode(COMBINED / OBSERVED_AREA / DIFFUSE / SPECULAR) \n";
	std::wcout << L" [F6] Toggle NormalMap(ON / OFF)\n [F7] Toggle DepthBuffer Visualization(ON / OFF) \n [F8] Toggle BoundingBox Visualization(ON / OFF)\n\n";
}

Renderer::~Renderer()
{
	SDL_DestroyWindow(m_pWindow);
	m_pWindow = nullptr;

	// --- HARDWARE ---
	// 1. Unbind Render Target view and Depth Stencil view from Device Context
	if (m_pDeviceContext)
	{
		m_pDeviceContext->ClearState();
		m_pDeviceContext->Flush();
	}

	// 2. Release Views
	SAFE_RELEASE(m_pDepthStencilView);
	SAFE_RELEASE(m_pRenderTargetView);

	// 3. Release Buffers
	SAFE_RELEASE(m_pDepthStencilBuffer);
	SAFE_RELEASE(m_pRenderTargetBuffer);

	// 4. Release Swap Chain
	SAFE_RELEASE(m_pSwapChain);

	// 5. Handle Device Context BEFORE Device
	SAFE_RELEASE(m_pDeviceContext);

	// 6. Release Device
	SAFE_RELEASE(m_pDevice);
}

void Renderer::Update(const Timer* pTimer)
{
	// SHARED
	const float aspectRatio{ static_cast<float>(m_Width) / static_cast<float>(m_Height) };
	ProcessInput();

	if (!m_RotationFrozen)
	{
		const float rotationSpeed{ PI_DIV_4 * pTimer->GetElapsed() };
		for (auto& pOpaqMesh : m_OpaqueMeshes)
		{
			pOpaqMesh->RotateY(rotationSpeed);
		}

		for (auto& pTrMesh : m_TransparentMeshes)
		{
			pTrMesh->RotateY(rotationSpeed);
		}
	}

	m_Camera.Update(pTimer, aspectRatio, m_CurrentRasterizerMode);

	switch (m_CurrentCullMode)
	{
	case CullMode::None:
		m_pCurrentRasterizer = m_pRasterizerNone;
		break;
	case CullMode::Back:
		m_pCurrentRasterizer = m_pRasterizerBack;
		break;
	case CullMode::Front:
		m_pCurrentRasterizer = m_pRasterizerFront;
		break;
	}
	

	if (m_CurrentRasterizerMode == RasterizerMode::Hardware)
	{
		switch (m_CurrentSamplerType)
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
	}
}

void Renderer::Render()
{
	if (!m_IsDXInitialized)
		return;

	Matrix viewProjMatrix{ m_Camera.viewMatrix * m_Camera.projectionMatrix };

	// ------- START OF FRAME --------
	if (m_CurrentRasterizerMode == RasterizerMode::Hardware)
	{
		constexpr float hardwareColor[4] = { 0.39f, 0.59f, 0.93f, 1.f };
		constexpr float uniformHardwareClearColor[4] = { 0.1f, 0.1f, 0.1f, 1.f };
		const float* currentHardwareBackgroundColor{ m_UniformClearColorActive ? uniformHardwareClearColor : hardwareColor };

		// Clear Views at the start of each Frame
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, currentHardwareBackgroundColor);
		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
	}
	else
	{
		constexpr UINT8 softwareColor[3] = { UINT8(0.39f * 255), UINT8(0.39f * 255), UINT8(0.39f * 255) };
		constexpr UINT8 uniformSoftwareClearColor[3] = { UINT8(0.1f * 255), UINT8(0.1f * 255), UINT8(0.1f * 255) };
		const UINT8* currentSoftwareBackgroundColor{ m_UniformClearColorActive ? uniformSoftwareClearColor : softwareColor };

		// START SDL
		SDL_LockSurface(m_pBackBuffer);

		std::fill_n(m_pDepthBufferPixels.get(), m_Width * m_Height, std::numeric_limits<float>::max());

		// CLEAR THE BUFFER
		SDL_FillRect(
			m_pBackBuffer,
			nullptr, // null fills the whole surface
			SDL_MapRGB(m_pBackBuffer->format,
				currentSoftwareBackgroundColor[0],
				currentSoftwareBackgroundColor[1],
				currentSoftwareBackgroundColor[2]) // Background color
		);
	}

	// ----------- RENDER FRAME -------------	
	// Draw Opaque Meshes first
	for (auto& pOpaqMesh : m_OpaqueMeshes)
	{
		pOpaqMesh->Render(m_CurrentRasterizerMode, viewProjMatrix, m_Camera.origin, m_pDeviceContext, m_CurrentSampler, m_CurrentCullMode);

		// After World Matrix calculations in Mesh::Render
		if (m_CurrentRasterizerMode == RasterizerMode::Software)
		{
			RenderSoftwareMesh(*pOpaqMesh, viewProjMatrix);
		}
	}
	// Draw Transparent Meshes AFTER
	if (m_CurrentRasterizerMode == RasterizerMode::Hardware && m_ShowFireMesh)
	{
		for (auto& pTrMesh : m_TransparentMeshes)
		{
			pTrMesh->Render(RasterizerMode::Hardware, viewProjMatrix, m_Camera.origin, m_pDeviceContext, m_CurrentSampler, m_CurrentCullMode);

			/*if (m_CurrentRasterizerMode == RasterizerMode::Software)
			{
				RenderSoftwareMesh(*pTrMesh, viewProjMatrix);
			}*/
		}
	}

	// -------- END OF FRAME --------
	if (m_CurrentRasterizerMode == RasterizerMode::Hardware)
	{
		// Present BackBuffer (SWAP)
		m_pSwapChain->Present(0, 0);
	}
	else
	{
		// Update SDL Surface
		SDL_UnlockSurface(m_pBackBuffer);
		SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
		SDL_UpdateWindowSurface(m_pWindow);
	}
}

void dae::Renderer::VertexTransformationFunction(const std::vector<VertexIn>& vertices_in, std::vector<VertexOut>& vertices_out, 
	const Matrix& WVPMatrix, const Matrix& worldMatrix) const
{
	if (vertices_out.capacity() < vertices_in.size())
	{
		vertices_out.reserve(vertices_in.size());
	}

	//const float cameraNear{ m_Camera.nearPlane };
	//const float cameraFar{ m_Camera.farPlane };

	for (size_t i{}; i < vertices_in.size(); ++i)
	{
		// Model -> Clip
		const Vector4 clipPos{ WVPMatrix.TransformPoint(Vector4(vertices_in[i].position, 1.f)) };

		const float invW{ 1.f / clipPos.w };

		// Perspective Divide
		const Vector3 projectedPos{ Vector3(clipPos) * invW };

		// Perspective -> Screen
		const Vector4 screenPos{ (projectedPos.x + 1.f) * 0.5f * m_Width,
			(1.f - projectedPos.y) * 0.5f * m_Height,
			projectedPos.z,
			invW
		};

		const auto perspectiveUV{ vertices_in[i].UVCoordinate * invW };

		// Model -> World
		const auto worldNormal{ worldMatrix.TransformVector(vertices_in[i].normal) };
		const auto worldTangent{ worldMatrix.TransformVector(vertices_in[i].tangent) };
		
		const Vector3 worldPos{ worldMatrix.TransformPoint(vertices_in[i].position) };
		const auto viewDir{ Vector3{ m_Camera.origin - worldPos }.Normalized() };

		vertices_out.emplace_back(VertexOut{ screenPos, perspectiveUV, worldNormal, worldTangent, viewDir });
	}
}

bool dae::Renderer::PassTriangleOptimizations(const std::array<VertexOut, 3> screenTri)
{
	// --- INSIDE SCREEN CHECK ---
	if (std::any_of(screenTri.begin(), screenTri.end(),
		[&](const VertexOut& v) {
			return v.position.x < 0.f || v.position.x >= m_Width ||
				v.position.y < 0.f || v.position.y >= m_Height;
		}))
	{
		return false; // Skip triangle
	}

	// --- FRUSTRUM CULLING ---
	if (std::any_of(screenTri.begin(), screenTri.end(),
		[&](const VertexOut& v) {
			return v.position.z < 0.f || v.position.z > 1.f;
		}))
	{
		return false; // Skip triangle
	}
	return true;
}

void dae::Renderer::FillRectangle(int x0, int y0, int x1, int y1, const ColorRGB& color)
{
	auto drawPixel = [&](int x, int y) // Store Lambda function
		{
			int idx = GetPixelNumber(x, y, m_Width);
			m_pBackBufferPixels[idx] = SDL_MapRGB(m_pBackBuffer->format,
				uint8_t(color.r * 255),
				uint8_t(color.g * 255),
				uint8_t(color.b * 255));
		};

	for (int y = y0; y <= y1; ++y)
	{
		for (int x = x0; x <= x1; ++x)
		{
			int idx = GetPixelNumber(x, y, m_Width);
			m_pBackBufferPixels[idx] = SDL_MapRGB(m_pBackBuffer->format,
				uint8_t(color.r * 255),
				uint8_t(color.g * 255),
				uint8_t(color.b * 255));
		}
	}
}

void dae::Renderer::CreateSamplerStates(ID3D11Device* pDevice)
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

void dae::Renderer::CreateRasterizerStates(ID3D11Device* pDevice)
{
	D3D11_RASTERIZER_DESC desc{};
	desc.FillMode = D3D11_FILL_SOLID;
	desc.FrontCounterClockwise = false; // depends on your winding order
	desc.DepthClipEnable = true;

	// Back-face culling
	desc.CullMode = D3D11_CULL_BACK;
	pDevice->CreateRasterizerState(&desc, &m_pRasterizerBack);

	// Front-face culling
	desc.CullMode = D3D11_CULL_FRONT;
	pDevice->CreateRasterizerState(&desc, &m_pRasterizerFront);

	// No culling
	desc.CullMode = D3D11_CULL_NONE;
	pDevice->CreateRasterizerState(&desc, &m_pRasterizerNone);
}

HRESULT Renderer::InitializeDirectX()
{
	// 1. ----- Create Device and Device Context -----
	D3D_FEATURE_LEVEL featureLevel{ D3D_FEATURE_LEVEL_11_1 };
	uint32_t createDeviceFlag{ 0 };
#if defined(DEBUG) || defined(_DEBUG)
	createDeviceFlag |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// Create DXGI Factory
	IDXGIFactory1* pDxgiFactory{};
	HRESULT result{ CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**> (&pDxgiFactory)) };
	if (FAILED(result))
		return S_FALSE;

	IDXGIAdapter* adapter = nullptr;
	IDXGIAdapter* selectedAdapter = nullptr;

	for (UINT i{ 0 }; pDxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);
		std::wcout << L"Adapter: " << i << L": " << desc.Description << L"\n";

		if (desc.VendorId == 0x10DE) // NVIDIA vendor ID
		{
			selectedAdapter = adapter;
			break;
		}
	}

	if (selectedAdapter == nullptr)
	{
		std::wcout << L"No NVIDIA GPU found on machine\n";
	}

	result = D3D11CreateDevice(selectedAdapter, D3D_DRIVER_TYPE_UNKNOWN, 0, createDeviceFlag, &featureLevel,
		1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext);
	if (FAILED(result))
		return S_FALSE;

	// 2. ----- SWAPCHAIN ------
	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferDesc.Width = m_Width;
	swapChainDesc.BufferDesc.Height = m_Height;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 32 bit format
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // Buffers will be bound to Output Merger as a render target
	swapChainDesc.BufferCount = 1;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;

	// Get Handle from SDL
	SDL_SysWMinfo sysWMInfo{};
	SDL_GetVersion(&sysWMInfo.version);
	SDL_GetWindowWMInfo(m_pWindow, &sysWMInfo);
	swapChainDesc.OutputWindow = sysWMInfo.info.win.window;

	result = pDxgiFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
	if (FAILED(result))
		return S_FALSE;

	// 3. ----- Depth Stencil and Depth Stencil View -----
	// Depth Stencil
	D3D11_TEXTURE2D_DESC depthStencilDesc{};
	depthStencilDesc.Width = m_Width;
	depthStencilDesc.Height = m_Height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	// View
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
	depthStencilViewDesc.Format = depthStencilDesc.Format;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer);
	if (FAILED(result))
		return S_FALSE;

	result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);
	if (FAILED(result))
		return S_FALSE;


	// 4. ----- Render Target and Render Target View -----
	result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**> (&m_pRenderTargetBuffer));
	if (FAILED(result))
		return S_FALSE;

	result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);
	if (FAILED(result))
		return S_FALSE;

	// Output Merger Stage
	// 5. ----- Binding RenderTargertView and DepthStencilView ------
	m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);


	// 6. ----- ViewPort ------
	D3D11_VIEWPORT viewPort{};
	viewPort.Width = static_cast<float> (m_Width);
	viewPort.Height = static_cast<float> (m_Height);
	viewPort.TopLeftX = 0.f;
	viewPort.TopLeftY = 0.f;
	viewPort.MinDepth = 0.f;
	viewPort.MaxDepth = 1.f;
	m_pDeviceContext->RSSetViewports(1, &viewPort);

	return S_OK;
}

void dae::Renderer::ProcessInput()
{
	const uint8_t* pKeyboardState{ SDL_GetKeyboardState(nullptr) };

	// --- SHARED ---
	
	// RASTERIZER STATE
	static bool wasF1Pressed{ false };
	bool isF1Pressed = pKeyboardState[SDL_SCANCODE_F1];

	if (wasF1Pressed && !isF1Pressed)
	{
		m_CurrentRasterizerMode = static_cast<RasterizerMode>((static_cast<int>(m_CurrentRasterizerMode) + 1) % 2);

		if (m_CurrentRasterizerMode == RasterizerMode::Hardware)
			std::wcout << L"RASTERIZER MODE = HARDWARE\n";
		else
			std::wcout << L"RASTERIZER MODE = SOFTWARE\n";
	}
	wasF1Pressed = isF1Pressed;

	// ROTATION
	static bool wasF2Pressed{ false };
	bool isF2Pressed = pKeyboardState[SDL_SCANCODE_F2];

	if (wasF2Pressed && !isF2Pressed)
	{
		m_RotationFrozen = !m_RotationFrozen;

		if (m_RotationFrozen)
			std::wcout << L"Vehicle Rotation OFF\n";
		else
			std::wcout << L"Vehicle Rotation ON\n";
	}
	wasF2Pressed = isF2Pressed;

	// Uniform Clear Color
	static bool wasF10Pressed{ false };
	bool isF10Pressed = pKeyboardState[SDL_SCANCODE_F10];

	if (wasF10Pressed && !isF10Pressed)
	{
		m_UniformClearColorActive = !m_UniformClearColorActive;

		if (m_UniformClearColorActive)
			std::wcout << L"Uniform ClearColor ON\n";
		else
			std::wcout << L"Uniform ClearColor OFF\n";
	}
	wasF10Pressed = isF10Pressed;

	// Cull Mode
	static bool wasF9Pressed{ false };
	bool isF9Pressed = pKeyboardState[SDL_SCANCODE_F9];

	if (wasF9Pressed && !isF9Pressed)
	{
		m_CurrentCullMode = static_cast<CullMode>((static_cast<int>(m_CurrentCullMode) + 1) % 3);

		switch (m_CurrentCullMode)
		{
		case CullMode::None:
			std::wcout << L"CULL MODE: NONE\n";
			break;
		case CullMode::Back:
			std::wcout << L"CULL MODE: BACK\n";
			break;
		case CullMode::Front:
			std::wcout << L"CULL MODE: FRONT\n";
			break;
		}
	}
	wasF9Pressed = isF9Pressed;

	// ------ HARDWARE ------
	if (m_CurrentRasterizerMode == RasterizerMode::Hardware)
	{
		// Toggle FireFX Mesh
		static bool wasF3Pressed{ false };
		bool isF3Pressed = pKeyboardState[SDL_SCANCODE_F3];

		if (wasF3Pressed && !isF3Pressed)
		{
			m_ShowFireMesh = !m_ShowFireMesh;

			if (m_ShowFireMesh)
				std::wcout << L"FireFX ON\n";
			else
				std::wcout << L"FireFX OFF\n";
		}
		wasF3Pressed = isF3Pressed;

		// Sampler State
		static bool wasF4Pressed{ false };
		bool isF4Pressed = pKeyboardState[SDL_SCANCODE_F4];

		if (wasF4Pressed && !isF4Pressed)
		{
			m_CurrentSamplerType = static_cast<SamplerType>((static_cast<int>(m_CurrentSamplerType) + 1) % 3);

			switch (m_CurrentSamplerType)
			{
			case SamplerType::Point:
				std::wcout << L"Sampler Filter = POINT\n";
				break;
			case SamplerType::Linear:
				std::wcout << L"Sampler Filter = LINEAR\n";
				break;
			case SamplerType::Anisotropic:
				std::wcout << L"Sampler Filter = ANISOTROPIC\n";
				break;
			}
		}
		wasF4Pressed = isF4Pressed;
	}
	else // ------ SOFTWARE ONLY ------
	{
		// Switch Lighting Modes
		static bool wasF5Pressed{ false };
		bool isF5Pressed = pKeyboardState[SDL_SCANCODE_F5];

		if (wasF5Pressed && !isF5Pressed)
		{
			m_CurrentLightingMode = static_cast<LightingMode>(static_cast<int>(m_CurrentLightingMode) + 1);

			if (m_CurrentLightingMode > LightingMode::Combined)
			{
				m_CurrentLightingMode = LightingMode::ObservedArea;
			}

			switch (m_CurrentLightingMode)
			{
			case dae::Renderer::LightingMode::ObservedArea:
				std::wcout << L"Shading Mode = OBSERVED_AREA\n";
				break;
			case dae::Renderer::LightingMode::Diffuse:
				std::wcout << L"Shading Mode = DIFFUSE\n";
				break;
			case dae::Renderer::LightingMode::Specular:
				std::wcout << L"Shading Mode = SPECULAR\n";
				break;
			case dae::Renderer::LightingMode::Combined:
				std::wcout << L"Shading Mode = COMBINED\n";
				break;
			}
		}
		wasF5Pressed = isF5Pressed;

		// Toggle Normal Map
		static bool wasF6Pressed{ false };
		bool isF6Pressed = pKeyboardState[SDL_SCANCODE_F6];

		if (wasF6Pressed && !isF6Pressed)
		{
			m_ShowNormalMap = !m_ShowNormalMap;

			if (m_ShowNormalMap)
				std::wcout << L"NormalMap ON\n";
			else
				std::wcout << L"NormalMap OFF\n";
		}
		wasF6Pressed = isF6Pressed;

		// Depth Buffer Visualization
		static bool wasF7Pressed{ false };
		bool isF7Pressed = pKeyboardState[SDL_SCANCODE_F7];

		if (wasF7Pressed && !isF7Pressed)
		{
			m_CurrentPixelColorState = static_cast<PixelColorState>(static_cast<int>(m_CurrentPixelColorState) + 1);

			if (m_CurrentPixelColorState > PixelColorState::DepthBuffer)
			{
				m_CurrentPixelColorState = PixelColorState::FinalColor;
			}

			switch (m_CurrentPixelColorState)
			{
			case dae::Renderer::PixelColorState::FinalColor:
				std::wcout << L"DepthBuffer Visualization OFF\n";
				break;
			case dae::Renderer::PixelColorState::DepthBuffer:
				std::wcout << L"DepthBuffer Visualization ON\n";
				break;
			}
		}
		wasF7Pressed = isF7Pressed;

		// Toggle Bounding Box
		static bool wasF8Pressed{ false };
		bool isF8Pressed = pKeyboardState[SDL_SCANCODE_F8];

		if (wasF8Pressed && !isF8Pressed)
		{
			m_ShowBoundingBox = !m_ShowBoundingBox;

			if (m_ShowBoundingBox)
				std::wcout << L"BoundingBox Visualization ON\n";
			else 
				std::wcout << L"BoundingBox Visualization OFF\n";
		}
		wasF8Pressed = isF8Pressed;
	}
}
