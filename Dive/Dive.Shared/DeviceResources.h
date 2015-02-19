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

		void	SetLogicalSize(Windows::Foundation::Size logicalSize);
		void	SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation);
		void	SetDpi(float dpi);
		void	RegisterDeviceNotify(IDeviceNotify* deviceNotify);
		void	Trim();
		void	Present();

	private:
		void	CreateWindowSizeDependentResources();

		// Cached reference to the window
		Platform::Agile<Windows::UI::Core::CoreWindow>	m_window;

		// Cached device properties
		Windows::Foundation::Size						m_logicalSize;
		Windows::Graphics::Display::DisplayOrientations	m_nativeOrientation;
		Windows::Graphics::Display::DisplayOrientations	m_currentOrientation;
		float											m_dpi;

		IDeviceNotify*	m_deviceNotify;
	};
}