#include "pch.h"
#include "DiveMain.h"
#include "Common/DirectXHelper.h"

using namespace Dive;

using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Concurrency;

DiveMain::DiveMain(std::shared_ptr<DX::DeviceResources> const& deviceResources) :
m_deviceResources(deviceResources)
{
	deviceResources->RegisterDeviceNotify(this);

	m_sampleRenderer = std::unique_ptr<Sample3DRenderer>(new Sample3DRenderer(m_deviceResources));

	m_fpsTextRenderer = std::unique_ptr<SampleFPSTextRenderer>(new SampleFPSTextRenderer(m_deviceResources));

	CreateWindowSizeDependentResources();
}

DiveMain::~DiveMain()
{
	m_deviceResources->RegisterDeviceNotify(nullptr);
}

void DiveMain::CreateWindowSizeDependentResources()
{
	m_sampleRenderer->CreateWindowSizeDependantResources();
}

void DiveMain::Update()
{
	m_timer.Tick([&]()
	{
		m_sampleRenderer->Update(m_timer);
		m_fpsTextRenderer->Update(m_timer);
	});
}

bool DiveMain::Render()
{
	if (m_timer.GetFrameCount() == 0)
		return false;

	auto	context = m_deviceResources->GetD3DDeviceContext();

	auto	viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	ID3D11RenderTargetView* const	targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_sampleRenderer->Render();
	m_fpsTextRenderer->Render();

	return true;
}

void DiveMain::OnDeviceLost()
{
	m_sampleRenderer->ReleaseDeviceDependantResources();
	m_fpsTextRenderer->ReleaseDeviceDependentResources();
}

void DiveMain::OnDeviceRestored()
{
	m_sampleRenderer->CreateDeviceDependantResources();
	m_fpsTextRenderer->CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}