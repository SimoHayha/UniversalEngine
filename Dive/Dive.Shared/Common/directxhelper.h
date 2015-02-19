#pragma once

#include <ppltasks.h>

namespace DX
{
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
			throw Platform::Exception::CreateException(hr);
	}

	inline Concurrency::task<std::vector<byte>> ReadDataAsync(std::wstring const& filename)
	{
		using namespace Windows::Storage;
		using namespace Concurrency;

		auto	folder = Windows::ApplicationModel::Package::Current->InstalledLocation;

		return create_task(folder->GetFileAsync(Platform::StringReference(filename.c_str()))).then([](StorageFile^ file)
		{
			return FileIO::ReadBufferAsync(file);
		}).then([](Streams::IBuffer^ fileBuffer) -> std::vector < byte >
		{
			std::vector<byte>	returnBuffer;
			returnBuffer.resize(fileBuffer->Length);
			Streams::DataReader::FromBuffer(fileBuffer)->ReadBytes(Platform::ArrayReference<byte>(returnBuffer.data(), fileBuffer->Length));
			return returnBuffer;
		});
	}

	inline float ConvertDipsToPixels(float dips, float dpi)
	{
		static const float	dipsPerInch = 96.0f;
		return floorf(dips * dpi / dipsPerInch + 0.5f);
	}

#if defined(_DEBUG)
	inline bool SdkLayersAvailable()
	{
		HRESULT	hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_NULL,
			0,
			D3D11_CREATE_DEVICE_DEBUG,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			nullptr,
			nullptr,
			nullptr
			);

		return SUCCEEDED(hr);
	}
#endif
}