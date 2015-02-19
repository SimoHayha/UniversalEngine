#include "pch.h"
#include "DeviceResources.h"

DX::DeviceResources::DeviceResources()
{
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

void DX::DeviceResources::RegisterDeviceNotify(IDeviceNotify* deviceNotify)
{
	m_deviceNotify = deviceNotify;
}

// Call this method when the app suspends. It provides a hint to the driver that the app 
// is entering an idle state and that temporary buffers can be reclaimed for use by other apps.
void DX::DeviceResources::Trim()
{
}

void DX::DeviceResources::Present()
{

}

void DX::DeviceResources::CreateWindowSizeDependentResources()
{

}