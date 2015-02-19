#pragma once

#include "Common/DeviceResources.h"
#include "ShaderStructures.h"
#include "Common/StepTimer.h"
#include "FBXManager.h"

namespace Dive
{
	class Sample3DRenderer
	{
	public:
		Sample3DRenderer(std::shared_ptr<DX::DeviceResources> const& deviceResources);
		void	CreateDeviceDependantResources();
		void	CreateWindowSizeDependantResources();
		void	ReleaseDeviceDependantResources();
		void	Update(DX::StepTimer const& timer);
		void	Render();

	private:
		void	Rotate(float radians);

	private:
		std::shared_ptr<DX::DeviceResources>	m_deviceResources;
		FBXManager*								m_fbxManager;

		Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBuffer;

		ModelViewProjectionConstantBuffer	m_constantBufferData;
		uint32	m_indexCount;

		bool	m_loadingComplete;
		float	m_degreesPerSeconds;
	};
}