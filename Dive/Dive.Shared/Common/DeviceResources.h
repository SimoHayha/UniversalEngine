#pragma once

namespace DX
{
	interface IDeviceNotify
	{
		virtual void	OnDeviceLost() = 0;
		virtual void	OnDeviceRestored() = 0;
	};

	class DeviceResources
	{
	public:
		DeviceResources();

		void	SetWindow(Windows::UI::Core::CoreWindow^ window);
		void	SetLogicalSize(Windows::Foundation::Size logicalSize);
		void	SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation);
		void	SetDpi(float dpi);
		void	ValidateDevice();
		void	HandleDeviceLost();
		void	RegisterDeviceNotify(IDeviceNotify* deviceNotify);
		void	Trim();
		void	Present();

		// Device Accessors
		Windows::Foundation::Size	GetOutputSize() const;
		Windows::Foundation::Size	GetLogicalSize() const;

		// D3D Accessors
		ID3D11Device2*			GetD3DDevice() const;
		ID3D11DeviceContext2*	GetD3DDeviceContext() const;
		IDXGISwapChain1*		GetSwapChain() const;
		D3D_FEATURE_LEVEL		GetDeviceFeatureLevel() const;
		ID3D11RenderTargetView*	GetBackBufferRenderTargetView() const;
		ID3D11DepthStencilView*	GetDepthStencilView() const;
		D3D11_VIEWPORT			GetScreenViewport() const;
		DirectX::XMFLOAT4X4		GetOrientationTransform3D() const;

		// D2D Accessors
		ID2D1Factory2*			GetD2DFactory() const;
		ID2D1Device1*			GetD2DDevice() const;
		ID2D1DeviceContext1*	GetD2DDeviceContext() const;
		ID2D1Bitmap1*			GetD2DTargetBitmap() const;
		IDWriteFactory2*		GetDWriteFactory() const;
		IWICImagingFactory2*	GetWicImagingFactory() const;
		D2D1::Matrix3x2F		GetOrientationTransform2D() const;

	private:
		void				CreateDeviceIndependentResources();
		void				CreateDeviceResources();
		void				CreateWindowSizeDependentResources();
		DXGI_MODE_ROTATION	ComputeDisplayRotation();

		// Direct3D objects
		Microsoft::WRL::ComPtr<ID3D11Device2>			m_d3dDevice;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext2>	m_d3dContext;
		Microsoft::WRL::ComPtr<IDXGISwapChain1>			m_swapChain;

		// Direct3D rendering objects
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	m_d3dRenderTargetView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	m_d3dDepthStencilView;
		D3D11_VIEWPORT									m_screenViewport;

		// Direct2D drawing components
		Microsoft::WRL::ComPtr<ID2D1Factory2>		m_d2dFactory;
		Microsoft::WRL::ComPtr<ID2D1Device1>		m_d2dDevice;
		Microsoft::WRL::ComPtr<ID2D1DeviceContext1>	m_d2dContext;
		Microsoft::WRL::ComPtr<ID2D1Bitmap1>		m_d2dTargetBitmap;

		// DirectWrite drawing components
		Microsoft::WRL::ComPtr<IDWriteFactory2>		m_dwriteFactory;
		Microsoft::WRL::ComPtr<IWICImagingFactory2>	m_wicFactory;

		// Cached reference to the window
		Platform::Agile<Windows::UI::Core::CoreWindow>	m_window;

		// Cached device properties
		D3D_FEATURE_LEVEL								m_d3dFeatureLevel;
		Windows::Foundation::Size						m_d3dRenderTargetSize;
		Windows::Foundation::Size						m_outputSize;
		Windows::Foundation::Size						m_logicalSize;
		Windows::Graphics::Display::DisplayOrientations	m_nativeOrientation;
		Windows::Graphics::Display::DisplayOrientations	m_currentOrientation;
		float											m_dpi;

		// Transforms used for display orientation
		D2D1::Matrix3x2F	m_orientationTransform2D;
		DirectX::XMFLOAT4X4	m_orientationTransform3D;

		IDeviceNotify*	m_deviceNotify;
	};
}