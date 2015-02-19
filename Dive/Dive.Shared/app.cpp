#include "pch.h"
#include "app.h"

using namespace Dive;

using namespace concurrency;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	auto	engineSource = ref new EngineSource();
	CoreApplication::Run(engineSource);
	return 0;
}

IFrameworkView^ EngineSource::CreateView()
{
	return ref new App();
}

App::App() :
m_windowClosed(false),
m_windowVisible(true)
{
}

void App::Initialize(CoreApplicationView^ applicationView)
{
	applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &App::OnActivated);
	CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs^>(this, &App::OnSuspending);
	CoreApplication::Resuming += ref new EventHandler<Object^>(this, &App::OnResuming);

	m_deviceResources = std::make_shared<DX::DeviceResources>();
}

void App::SetWindow(CoreWindow^ window)
{
	window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &App::OnVisibilityChanged);
	window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &App::OnWindowClosed);

	DisplayInformation^	currentDisplayInformation = DisplayInformation::GetForCurrentView();

	DisplayInformation::DisplayContentsInvalidated += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnDisplayContentsInvalidated);

#if !(WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
	window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &App::OnWindowSizeChanged);
	currentDisplayInformation->DpiChanged += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnDPIChanged);
	currentDisplayInformation->OrientationChanged += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnOrientationChanged);

	auto	pointerVisualizationSettings = PointerVisualizationSettings::GetForCurrentView();
	pointerVisualizationSettings->IsContactFeedbackEnabled = false;
	pointerVisualizationSettings->IsBarrelButtonFeedbackEnabled = false;
#endif

	m_deviceResources->SetWindow(window);
}

void App::Load(Platform::String^ entryPoint)
{
	if (!m_main)
		m_main = std::unique_ptr<DiveMain>(new DiveMain(m_deviceResources));
}

void App::Run()
{
	while (!m_windowClosed)
	{
		if (m_windowVisible)
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

			m_main->Update();

			if (m_main->Render())
				m_deviceResources->Present();
		}
		else
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}
}

void App::Uninitialize()
{
}

void App::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	CoreWindow::GetForCurrentThread()->Activate();
}

void App::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
	SuspendingDeferral^	deferral = args->SuspendingOperation->GetDeferral();

	create_task([this, deferral]()
	{
		m_deviceResources->Trim();

		deferral->Complete();
	});
}

void App::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
}

void App::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
}

void App::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	m_windowClosed = true;
}

#if !(WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
void App::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	m_deviceResources->SetLogicalSize(Size(sender->Bounds.Width, sender->Bounds.Height));
	m_main->CreateWindowSizeDependentResources();
}
#endif

void App::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	m_deviceResources->ValidateDevice();
}

#if !(WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
void App::OnDPIChanged(DisplayInformation^ sender, Object^ args)
{
	m_deviceResources->SetDpi(sender->LogicalDpi);
	m_main->CreateWindowSizeDependentResources();
}

void App::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	m_deviceResources->SetCurrentOrientation(sender->CurrentOrientation);
	m_main->CreateWindowSizeDependentResources();
}
#endif