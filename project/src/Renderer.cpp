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
	m_CurrentSamplerType{ SamplerType::Point },
	m_RotationFrozen{ false },
	m_CurrentRasterizerState{ RasterizerState::Hardware },
	m_ShowFireMesh{ m_CurrentRasterizerState == RasterizerState::Hardware },
	m_UniformClearColorActive{ false }
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Initialize DirectX pipeline
	const HRESULT result = InitializeDirectX();
	if (result == S_OK)
	{
		m_IsInitialized = true;
		std::cout << "DirectX is initialized and ready!\n";
	}
	else
	{
		std::cout << "DirectX initialization failed!\n";
	}

	m_Camera.Initialize(45.f, { 0.f, 0.f, 0.f }, 0.1f, 100.f);


	m_OpaqueMeshes.reserve(2);
	m_OpaqueMeshes.emplace_back(std::make_unique<Mesh<ShadingEffect>>(
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
}

Renderer::~Renderer()
{
	SDL_DestroyWindow(m_pWindow);
	m_pWindow = nullptr;

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
	const float aspectRatio{ static_cast<float>(m_Width) / static_cast<float>(m_Height) };
	ProcessInput();

	if (!m_RotationFrozen)
	{
		const float rotationSpeed{ PI_DIV_4 * pTimer->GetElapsed() };
		for (auto& pOpaqMesh : m_OpaqueMeshes)
		{
			//pOpaqMesh->RotateY(rotationSpeed);
		}

		for (auto& pTrMesh : m_TransparentMeshes)
		{
			//pTrMesh->RotateY(rotationSpeed);
		}
	}

	// ------- START OF FRAME --------
	if (m_CurrentRasterizerState == RasterizerState::Hardware)
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
		//SDL_LockSurface(m_pBackBuffer);

		//std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, std::numeric_limits<float>::max());

		//// CLEAR THE BUFFER
		//SDL_FillRect(
		//	m_pBackBuffer,
		//	nullptr, // null fills the whole surface
		//	SDL_MapRGB(m_pBackBuffer->format, 
		//		currentSoftwareBackgroundColor[0], 
		//		currentSoftwareBackgroundColor[1], 
		//		currentSoftwareBackgroundColor[2]) // Background color
		//);
	}

	m_Camera.Update(pTimer, aspectRatio);
	Matrix viewProjMatrix{ m_Camera.viewMatrix * m_Camera.projectionMatrix };

	// ----------- Render Frame -------------	
	// Draw Opaque Meshes first
	for (auto& pMesh : m_OpaqueMeshes)
	{
		pMesh->Render(m_CurrentRasterizerState, viewProjMatrix, m_Camera.origin, m_pDeviceContext, m_CurrentSamplerType);
	}
	// Draw Transparent Meshes AFTER
	if (m_CurrentRasterizerState == RasterizerState::Hardware && m_ShowFireMesh)
	{
		for (auto& pTrMesh : m_TransparentMeshes)
		{
			pTrMesh->Render(RasterizerState::Hardware, viewProjMatrix, m_Camera.origin, m_pDeviceContext, m_CurrentSamplerType);
		}
	}

	// -------- END OF FRAME --------
	if (m_CurrentRasterizerState == RasterizerState::Hardware)
	{
		// Present BackBuffer (SWAP)
		m_pSwapChain->Present(0, 0);
	}
	else
	{
		// Update SDL Surface
		/*SDL_UnlockSurface(m_pBackBuffer);
		SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
		SDL_UpdateWindowSurface(m_pWindow);*/
	}

}


void Renderer::Render() const
{
	if (!m_IsInitialized)
		return;
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

	// RASTERIZER STATE
	static bool wasF1Pressed{ false };
	bool isF1Pressed = pKeyboardState[SDL_SCANCODE_F1];

	if (wasF1Pressed && !isF1Pressed)
	{
		m_CurrentRasterizerState = static_cast<RasterizerState>((static_cast<int>(m_CurrentRasterizerState) + 1) % 2);
		std::wcout << "RASTERIZER STATE: " << std::to_wstring(static_cast<int>(m_CurrentRasterizerState)) << "\n";
	}
	wasF1Pressed = isF1Pressed;

	// ROTATION
	static bool wasF2Pressed{ false };
	bool isF2Pressed = pKeyboardState[SDL_SCANCODE_F2];

	if (wasF2Pressed && !isF2Pressed)
	{
		m_RotationFrozen = !m_RotationFrozen;
	}
	wasF2Pressed = isF2Pressed;

	// Uniform Clear Color
	static bool wasF10Pressed{ false };
	bool isF10Pressed = pKeyboardState[SDL_SCANCODE_F10];

	if (wasF10Pressed && !isF10Pressed)
	{
		m_UniformClearColorActive = !m_UniformClearColorActive;
	}
	wasF10Pressed = isF10Pressed;

	// ------ HARDWARE ONLY ------
	if (m_CurrentRasterizerState == RasterizerState::Hardware)
	{
		// Toggle FireFX Mesh
		static bool wasF3Pressed{ false };
		bool isF3Pressed = pKeyboardState[SDL_SCANCODE_F3];

		if (wasF3Pressed && !isF3Pressed)
		{
			m_ShowFireMesh = !m_ShowFireMesh;
		}
		wasF3Pressed = isF3Pressed;

		// Sampler State
		static bool wasF4Pressed{ false };
		bool isF4Pressed = pKeyboardState[SDL_SCANCODE_F4];

		if (wasF4Pressed && !isF4Pressed)
		{
			m_CurrentSamplerType = static_cast<SamplerType>((static_cast<int>(m_CurrentSamplerType) + 1) % 3);
			std::wcout << "Sampler State: " << std::to_wstring(static_cast<int>(m_CurrentSamplerType)) << "\n";
		}
		wasF4Pressed = isF4Pressed;
	}
	else // ------ SOFTWARE ONLY ------
	{
		
	}
	
}
