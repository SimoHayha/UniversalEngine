#pragma once

#include "Common/StepTimer.h"
#include "Common/DeviceResources.h"
#include "Sample3DRenderer.h"
#include "SampleFPSTextRenderer.h"

namespace Dive
{
	class DiveMain : public DX::IDeviceNotify
	{
	public:
		DiveMain(std::shared_ptr<DX::DeviceResources> const& deviceResources);
		~DiveMain();

		void	CreateWindowSizeDependentResources();
		void	Update();
		bool	Render();

		virtual void	OnDeviceLost() override;
		virtual void	OnDeviceRestored() override;

	private:
		std::shared_ptr<DX::DeviceResources>	m_deviceResources;

		std::unique_ptr<Sample3DRenderer>		m_sampleRenderer;
		std::unique_ptr<SampleFPSTextRenderer>	m_fpsTextRenderer;

		DX::StepTimer	m_timer;
	};
}