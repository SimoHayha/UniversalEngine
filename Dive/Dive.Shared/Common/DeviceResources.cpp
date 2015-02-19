#include "pch.h"
#include "DeviceResources.h"
#include "fbxsdk.h"
#include "DirectXHelper.h"

using namespace D2D1;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;
using namespace Platform;

// Constants used to calculate screen rotations
namespace ScreenRotation
{
	// 0-degree Z-rotation
	static const XMFLOAT4X4 Rotation0(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	// 90-degree Z-rotation
	static const XMFLOAT4X4 Rotation90(
		0.0f, 1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	// 180-degree Z-rotation
	static const XMFLOAT4X4 Rotation180(
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	// 270-degree Z-rotation
	static const XMFLOAT4X4 Rotation270(
		0.0f, -1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);
};

DX::DeviceResources::DeviceResources() :
m_screenViewport(),
m_d3dFeatureLevel(D3D_FEATURE_LEVEL_11_1),
m_d3dRenderTargetSize(),
m_outputSize(),
m_logicalSize(),
m_nativeOrientation(DisplayOrientations::None),
m_currentOrientation(DisplayOrientations::None),
m_dpi(-1.0f),
m_deviceNotify(nullptr)
{
	CreateDeviceIndependentResources();
	CreateDeviceResources();
}

void DX::DeviceResources::CreateDeviceIndependentResources()
{
	D2D1_FACTORY_OPTIONS	options;
	ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));

#if defined(_DEBUG)
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

	DX::ThrowIfFailed(
		D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			__uuidof(ID2D1Factory2),
			&options,
			&m_d2dFactory
			)
		);

	DX::ThrowIfFailed(
		DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory2),
			&m_dwriteFactory
			)
		);

	DX::ThrowIfFailed(
		CoCreateInstance(
			CLSID_WICImagingFactory2,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_wicFactory)
			)
		);
}

void DX::DeviceResources::CreateDeviceResources()
{
	UINT	creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
	if (DX::SdkLayersAvailable())
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL	featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};

	ComPtr<ID3D11Device>		device;
	ComPtr<ID3D11DeviceContext>	context;

	HRESULT	hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		0,
		creationFlags,
		featureLevels,
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,
		&device,
		&m_d3dFeatureLevel,
		&context
		);

	if (FAILED(hr))
	{
		// WARP
		DX::ThrowIfFailed(
			D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE_WARP,
				0,
				creationFlags,
				featureLevels,
				ARRAYSIZE(featureLevels),
				D3D11_SDK_VERSION,
				&device,
				&m_d3dFeatureLevel,
				&context
				)
			);
	}

	DX::ThrowIfFailed(
		device.As(&m_d3dDevice)
		);

	DX::ThrowIfFailed(
		context.As(&m_d3dContext)
		);

	ComPtr<IDXGIDevice3>	dxgiDevice;
	DX::ThrowIfFailed(
		m_d3dDevice.As(&dxgiDevice)
		);

	DX::ThrowIfFailed(
		m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice)
		);

	DX::ThrowIfFailed(
		m_d2dDevice->CreateDeviceContext(
			D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
			&m_d2dContext
			)
		);
}

void DX::DeviceResources::CreateWindowSizeDependentResources()
{
#if (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
	m_swapChain = nullptr;
#endif
	ID3D11RenderTargetView*	nullViews[] = { nullptr };
	m_d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	m_d3dRenderTargetView = nullptr;
	m_d3dDepthStencilView = nullptr;
	m_d3dContext->Flush();

	m_outputSize.Width = DX::ConvertDipsToPixels(m_logicalSize.Width, m_dpi);
	m_outputSize.Height = DX::ConvertDipsToPixels(m_logicalSize.Height, m_dpi);

	m_outputSize.Width = max(m_outputSize.Width, 1);
	m_outputSize.Height = max(m_outputSize.Height, 1);

	DXGI_MODE_ROTATION	displayRotation = ComputeDisplayRotation();

	bool	swapDimension = displayRotation == DXGI_MODE_ROTATION_ROTATE90 || displayRotation == DXGI_MODE_ROTATION_ROTATE270;
	m_d3dRenderTargetSize.Width = swapDimension ? m_outputSize.Height : m_outputSize.Width;
	m_d3dRenderTargetSize.Height = swapDimension ? m_outputSize.Width : m_outputSize.Height;

	if (m_swapChain != nullptr)
	{
		HRESULT	hr = m_swapChain->ResizeBuffers(
			2,
			lround(m_d3dRenderTargetSize.Width),
			lround(m_d3dRenderTargetSize.Height),
			DXGI_FORMAT_B8G8R8A8_UNORM,
			0
			);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			HandleDeviceLost();
			return;
		}
		else
			DX::ThrowIfFailed(hr);
	}
	else
	{
		DXGI_SWAP_CHAIN_DESC1	swapChainDesc = { 0 };

		swapChainDesc.Width = lround(m_d3dRenderTargetSize.Width);
		swapChainDesc.Height = lround(m_d3dRenderTargetSize.Height);
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapChainDesc.Flags = 0;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		ComPtr<IDXGIDevice3>	dxgiDevice;
		DX::ThrowIfFailed(
			m_d3dDevice.As(&dxgiDevice)
			);

		ComPtr<IDXGIAdapter>	dxgiAdapter;
		DX::ThrowIfFailed(
			dxgiDevice->GetAdapter(&dxgiAdapter)
			);

		ComPtr<IDXGIFactory2>	dxgiFactory;
		DX::ThrowIfFailed(
			dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))
			);

		DX::ThrowIfFailed(
			dxgiFactory->CreateSwapChainForCoreWindow(
				m_d3dDevice.Get(),
				reinterpret_cast<IUnknown*>(m_window.Get()),
				&swapChainDesc,
				nullptr,
				&m_swapChain
				)
			);

		DX::ThrowIfFailed(
			dxgiDevice->SetMaximumFrameLatency(1)
			);
	}

	switch (displayRotation)
	{
	case DXGI_MODE_ROTATION_IDENTITY:
		m_orientationTransform2D = Matrix3x2F::Identity();
		m_orientationTransform3D = ScreenRotation::Rotation0;
		break;
	case DXGI_MODE_ROTATION_ROTATE90:
		m_orientationTransform2D =
			Matrix3x2F::Rotation(90.0f) *
			Matrix3x2F::Translation(m_logicalSize.Height, 0.0f);
		m_orientationTransform3D = ScreenRotation::Rotation270;
		break;
	case DXGI_MODE_ROTATION_ROTATE180:
		m_orientationTransform2D =
			Matrix3x2F::Rotation(180.0f) *
			Matrix3x2F::Translation(m_logicalSize.Width, m_logicalSize.Height);
		m_orientationTransform3D = ScreenRotation::Rotation180;
		break;
	case DXGI_MODE_ROTATION_ROTATE270:
		m_orientationTransform2D =
			Matrix3x2F::Rotation(270.0f) *
			Matrix3x2F::Translation(0.0f, m_logicalSize.Width);
		m_orientationTransform3D = ScreenRotation::Rotation90;
		break;
	default:
		throw ref new FailureException();
		break;
	}

	DX::ThrowIfFailed(
		m_swapChain->SetRotation(displayRotation)
		);

	ComPtr<ID3D11Texture2D>	backBuffer;
	DX::ThrowIfFailed(
		m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
		);

	DX::ThrowIfFailed(
		m_d3dDevice->CreateRenderTargetView(
			backBuffer.Get(),
			nullptr,
			&m_d3dRenderTargetView
			)
		);

	CD3D11_TEXTURE2D_DESC	depthStencilDesc(
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		lround(m_d3dRenderTargetSize.Width),
		lround(m_d3dRenderTargetSize.Height),
		1,
		1,
		D3D11_BIND_DEPTH_STENCIL
		);

	ComPtr<ID3D11Texture2D>	depthStencil;
	DX::ThrowIfFailed(
		m_d3dDevice->CreateTexture2D(
			&depthStencilDesc,
			nullptr,
			&depthStencil
			)
		);

	CD3D11_DEPTH_STENCIL_VIEW_DESC	depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	DX::ThrowIfFailed(
		m_d3dDevice->CreateDepthStencilView(
			depthStencil.Get(),
			&depthStencilViewDesc,
			&m_d3dDepthStencilView
			)
		);

	m_screenViewport = CD3D11_VIEWPORT(
		0.0f,
		0.0f,
		m_d3dRenderTargetSize.Width,
		m_d3dRenderTargetSize.Height
		);

	m_d3dContext->RSSetViewports(1, &m_screenViewport);

	D2D1_BITMAP_PROPERTIES1	bitmapProperties =
		D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			m_dpi,
			m_dpi
		);

	ComPtr<IDXGISurface2>	dxgiBackBuffer;
	DX::ThrowIfFailed(
		m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer))
		);

	DX::ThrowIfFailed(
		m_d2dContext->CreateBitmapFromDxgiSurface(
			dxgiBackBuffer.Get(),
			&bitmapProperties,
			&m_d2dTargetBitmap
			)
		);

	m_d2dContext->SetTarget(m_d2dTargetBitmap.Get());

	m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
}

DXGI_MODE_ROTATION DX::DeviceResources::ComputeDisplayRotation()
{
	DXGI_MODE_ROTATION	rotation = DXGI_MODE_ROTATION_UNSPECIFIED;

	switch (m_nativeOrientation)
	{
	case DisplayOrientations::Landscape:
		switch (m_currentOrientation)
		{
		case DisplayOrientations::Landscape:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;
		case DisplayOrientations::Portrait:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;
		case DisplayOrientations::LandscapeFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;
		case DisplayOrientations::PortraitFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;
		}
		break;
	case DisplayOrientations::Portrait:
		switch (m_currentOrientation)
		{
		case DisplayOrientations::Landscape:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;
		case DisplayOrientations::Portrait:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;
		case DisplayOrientations::LandscapeFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;
		case DisplayOrientations::PortraitFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;
		}
		break;
	}
	return rotation;
}

void DX::DeviceResources::SetLogicalSize(Windows::Foundation::Size logicalSize)
{
	if (m_logicalSize != logicalSize)
	{
		m_logicalSize = logicalSize;
		CreateWindowSizeDependentResources();
	}
}

void DX::DeviceResources::SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation)
{
	if (m_currentOrientation != currentOrientation)
	{
		m_currentOrientation = currentOrientation;
		CreateWindowSizeDependentResources();
	}
}

void DX::DeviceResources::SetDpi(float dpi)
{
	if (dpi != m_dpi)
	{
		m_dpi = dpi;

		m_logicalSize = Windows::Foundation::Size(m_window->Bounds.Width, m_window->Bounds.Height);

		CreateWindowSizeDependentResources();
	}
}

void DX::DeviceResources::ValidateDevice()
{
	ComPtr<IDXGIDevice3> dxgiDevice;
	DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

	ComPtr<IDXGIAdapter> deviceAdapter;
	DX::ThrowIfFailed(dxgiDevice->GetAdapter(&deviceAdapter));

	ComPtr<IDXGIFactory2> deviceFactory;
	DX::ThrowIfFailed(deviceAdapter->GetParent(IID_PPV_ARGS(&deviceFactory)));

	ComPtr<IDXGIAdapter1> previousDefaultAdapter;
	DX::ThrowIfFailed(deviceFactory->EnumAdapters1(0, &previousDefaultAdapter));

	DXGI_ADAPTER_DESC previousDesc;
	DX::ThrowIfFailed(previousDefaultAdapter->GetDesc(&previousDesc));

	ComPtr<IDXGIFactory2> currentFactory;
	DX::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&currentFactory)));

	ComPtr<IDXGIAdapter1> currentDefaultAdapter;
	DX::ThrowIfFailed(currentFactory->EnumAdapters1(0, &currentDefaultAdapter));

	DXGI_ADAPTER_DESC currentDesc;
	DX::ThrowIfFailed(currentDefaultAdapter->GetDesc(&currentDesc));

	if (previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart ||
		previousDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart ||
		FAILED(m_d3dDevice->GetDeviceRemovedReason()))
	{
		dxgiDevice = nullptr;
		deviceAdapter = nullptr;
		deviceFactory = nullptr;
		previousDefaultAdapter = nullptr;

		HandleDeviceLost();
	}
}

void DX::DeviceResources::HandleDeviceLost()
{
	m_swapChain = nullptr;

	if (m_deviceNotify)
		m_deviceNotify->OnDeviceLost();

	CreateDeviceResources();
	m_d2dContext->SetDpi(m_dpi, m_dpi);
	CreateWindowSizeDependentResources();

	if (m_deviceNotify)
		m_deviceNotify->OnDeviceRestored();
}

void DX::DeviceResources::RegisterDeviceNotify(IDeviceNotify* deviceNotify)
{
	m_deviceNotify = deviceNotify;
}

// Call this method when the app suspends. It provides a hint to the driver that the app 
// is entering an idle state and that temporary buffers can be reclaimed for use by other apps.
void DX::DeviceResources::Trim()
{
	ComPtr<IDXGIDevice3>	dxgiDevice;
	m_d3dDevice.As(&dxgiDevice);

	dxgiDevice->Trim();
}

void DX::DeviceResources::Present()
{
	HRESULT	hr = m_swapChain->Present(1, 0);

	m_d3dContext->DiscardView(m_d3dRenderTargetView.Get());

	m_d3dContext->DiscardView(m_d3dDepthStencilView.Get());

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		HandleDeviceLost();
	else
		DX::ThrowIfFailed(hr);
}

Windows::Foundation::Size DX::DeviceResources::GetOutputSize() const
{
	return m_outputSize;
}

Windows::Foundation::Size DX::DeviceResources::GetLogicalSize() const
{
	return m_logicalSize;
}

ID3D11Device2* DX::DeviceResources::GetD3DDevice() const
{
	return m_d3dDevice.Get();
}

ID3D11DeviceContext2* DX::DeviceResources::GetD3DDeviceContext() const
{
	return m_d3dContext.Get();
}

IDXGISwapChain1* DX::DeviceResources::GetSwapChain() const
{
	return m_swapChain.Get();
}

D3D_FEATURE_LEVEL DX::DeviceResources::GetDeviceFeatureLevel() const
{
	return m_d3dFeatureLevel;
}

ID3D11RenderTargetView* DX::DeviceResources::GetBackBufferRenderTargetView() const
{
	return m_d3dRenderTargetView.Get();
}

ID3D11DepthStencilView* DX::DeviceResources::GetDepthStencilView() const
{
	return m_d3dDepthStencilView.Get();
}

D3D11_VIEWPORT DX::DeviceResources::GetScreenViewport() const
{
	return m_screenViewport;
}

DirectX::XMFLOAT4X4 DX::DeviceResources::GetOrientationTransform3D() const
{
	return m_orientationTransform3D;
}

void DX::DeviceResources::SetWindow(Windows::UI::Core::CoreWindow^ window)
{
	DisplayInformation^	currentDisplayInformation = DisplayInformation::GetForCurrentView();

	m_window = window;
	m_logicalSize = Windows::Foundation::Size(window->Bounds.Width, window->Bounds.Height);
	m_nativeOrientation = currentDisplayInformation->NativeOrientation;
	m_currentOrientation = currentDisplayInformation->CurrentOrientation;
	m_dpi = currentDisplayInformation->LogicalDpi;
	m_d2dContext->SetDpi(m_dpi, m_dpi);

	CreateWindowSizeDependentResources();
}

ID2D1Factory2* DX::DeviceResources::GetD2DFactory() const
{
	return m_d2dFactory.Get();
}

ID2D1Device1* DX::DeviceResources::GetD2DDevice() const
{
	return m_d2dDevice.Get();
}

ID2D1DeviceContext1* DX::DeviceResources::GetD2DDeviceContext() const
{
	return m_d2dContext.Get();
}

ID2D1Bitmap1* DX::DeviceResources::GetD2DTargetBitmap() const
{
	return m_d2dTargetBitmap.Get();
}

IDWriteFactory2* DX::DeviceResources::GetDWriteFactory() const
{
	return m_dwriteFactory.Get();
}

IWICImagingFactory2* DX::DeviceResources::GetWicImagingFactory() const
{
	return m_wicFactory.Get();
}

D2D1::Matrix3x2F DX::DeviceResources::GetOrientationTransform2D() const
{
	return m_orientationTransform2D;
}
