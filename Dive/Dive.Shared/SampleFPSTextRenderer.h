#pragma once

#include <string>
#include "Common/DeviceResources.h"
#include "Common/StepTimer.h"

namespace Dive
{
	class SampleFPSTextRenderer
	{
	public:
		SampleFPSTextRenderer(std::shared_ptr<DX::DeviceResources> const& deviceResources);
		void	CreateDeviceDependentResources();
		void	ReleaseDeviceDependentResources();
		void	Update(DX::StepTimer const& timer);
		void	Render();

	private:
		std::shared_ptr<DX::DeviceResources>	m_deviceResources;

		std::wstring									m_text;
		DWRITE_TEXT_METRICS								m_textMetrics;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>	m_whiteBrush;
		Microsoft::WRL::ComPtr<ID2D1DrawingStateBlock>	m_stateBlock;
		Microsoft::WRL::ComPtr<IDWriteTextLayout>		m_textLayout;
		Microsoft::WRL::ComPtr<IDWriteTextFormat>		m_textFormat;
	};
}