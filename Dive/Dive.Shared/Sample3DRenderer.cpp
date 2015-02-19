#include "pch.h"
#include "Sample3DRenderer.h"
#include "fbxsdk.h"
#include "Common/directxhelper.h"

using namespace Dive;

using namespace DirectX;
using namespace Windows::Foundation;

Sample3DRenderer::Sample3DRenderer(std::shared_ptr<DX::DeviceResources> const& deviceResources) :
m_loadingComplete(false),
m_degreesPerSeconds(45.0f),
m_indexCount(0),
m_deviceResources(deviceResources)
{
	CreateDeviceDependantResources();
	CreateWindowSizeDependantResources();
	m_fbxManager = new FBXManager();
	m_fbxManager->Initialize();
}

void Sample3DRenderer::CreateWindowSizeDependantResources()
{
	Size	outputSize = m_deviceResources->GetOutputSize();
	float	aspectRatio = outputSize.Width / outputSize.Height;
	float	fovAngleY = 70.0f * XM_PI / 180.0f;

	if (aspectRatio < 1.0f)
		fovAngleY *= 2.0f;

	XMMATRIX	perspectiveMatrix = XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
		);

	XMFLOAT4X4	orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX	orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(
		&m_constantBufferData.Projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
		);

	static const XMVECTORF32 eye = { 0.0f, 0.7f, 1.5f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	XMStoreFloat4x4(&m_constantBufferData.View, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));
}

void Sample3DRenderer::Update(DX::StepTimer const& timer)
{
	float	radiansPerSecond = XMConvertToRadians(m_degreesPerSeconds);
	double	totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
	float	radians = static_cast<float>(fmod(totalRotation, XM_2PI));

	Rotate(radians);
}

void Sample3DRenderer::Render()
{
	if (!m_loadingComplete)
		return;

	auto	context = m_deviceResources->GetD3DDeviceContext();

	context->UpdateSubresource(
		m_constantBuffer.Get(),
		0,
		nullptr,
		&m_constantBufferData,
		0,
		0
		);

	UINT	stride = sizeof(VertexPositionColor);
	UINT	offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
		);

	context->IASetIndexBuffer(
		m_indexBuffer.Get(),
		DXGI_FORMAT_R16_UINT,
		0
		);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->IASetInputLayout(m_inputLayout.Get());

	context->VSSetShader(
		m_vertexShader.Get(),
		nullptr,
		0
		);

	context->VSSetConstantBuffers(
		0,
		1,
		m_constantBuffer.GetAddressOf()
		);

	context->PSSetShader(
		m_pixelShader.Get(),
		nullptr,
		0
		);

	context->DrawIndexed(
		m_indexCount,
		0,
		0
		);
}

void Sample3DRenderer::CreateDeviceDependantResources()
{
	auto	loadVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso");
	auto	loadPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso");

	auto	createVSTask = loadVSTask.then([this](std::vector<byte> const& fileData)
	{
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_vertexShader
				)
			);

		static const D3D11_INPUT_ELEMENT_DESC	vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
				)
			);
	});

	auto	createPSTask = loadPSTask.then([this](std::vector<byte> const& fileData)
	{
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			&m_pixelShader
			)
			);

		CD3D11_BUFFER_DESC	constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
			&constantBufferDesc,
			nullptr,
			&m_constantBuffer
			)
			);
	});

	auto	createCubeTask = (createPSTask && createVSTask).then([this]()
	{
		static const VertexPositionColor	cubeVertices[] =
		{
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(-0.5f, 0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f) },
			{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(0.5f, 0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
		};

		D3D11_SUBRESOURCE_DATA	vertexBufferData = { 0 };
		vertexBufferData.pSysMem = cubeVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC	vertexBufferDesc(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
			&vertexBufferDesc,
			&vertexBufferData,
			&m_vertexBuffer
			)
			);

		static const unsigned short cubeIndices[] =
		{
			0, 2, 1, // -x
			1, 2, 3,

			4, 5, 6, // +x
			5, 7, 6,

			0, 1, 5, // -y
			0, 5, 4,

			2, 6, 7, // +y
			2, 7, 3,

			0, 4, 6, // -z
			0, 6, 2,

			1, 3, 7, // +z
			1, 7, 5,
		};

		m_indexCount = ARRAYSIZE(cubeIndices);

		D3D11_SUBRESOURCE_DATA	indexBufferData = { 0 };
		indexBufferData.pSysMem = cubeIndices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC	indexBufferDesc(sizeof(cubeIndices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
			&indexBufferDesc,
			&indexBufferData,
			&m_indexBuffer
			)
			);
	});

	auto	createHumanoidTask = (createPSTask && createVSTask && createCubeTask).then([this]()
	{


		//FbxScene*	scene = m_fbxManager->LoadScene("humanoid.fbx");
		//FbxNode*	node = (FbxNode*)scene;

		//FbxNodeAttribute::EType	attributeType;

		//if (!node->GetNodeAttribute())
		//{
		//	//throw ref new FailureException();
		//}
		//else
		//{
		//	attributeType = node->GetNodeAttribute()->GetAttributeType();
		//	if (attributeType == FbxNodeAttribute::eMesh)
		//	{
		//		FbxMesh*	mesh = (FbxMesh*)node;
		//		auto		controls = mesh->GetControlPoints();
		//		auto		vertexCount = mesh->GetControlPointsCount();
		//		auto		polyCount = mesh->GetPolygonCount();

		//		VertexPositionColor*	vertices = new VertexPositionColor[vertexCount];
		//		unsigned short*			indices = new unsigned short[polyCount * 3];

		//		FbxVector4	vertexVect;
		//		for (auto i = 0; i < vertexCount; ++i)
		//		{
		//			vertexVect = controls[i];
		//			vertices[i].Pos.x = static_cast<float>(vertexVect[0]);
		//			vertices[i].Pos.y = static_cast<float>(vertexVect[1]);
		//			vertices[i].Pos.z = static_cast<float>(vertexVect[2]);
		//		}
		//	}
		//}
	});

	createHumanoidTask.then([this]()
	{
		m_loadingComplete = true;
	});
}

void Sample3DRenderer::ReleaseDeviceDependantResources()
{
	m_loadingComplete = false;
	m_vertexShader.Reset();
	m_inputLayout.Reset();
	m_pixelShader.Reset();
	m_constantBuffer.Reset();
	m_vertexBuffer.Reset();
	m_indexBuffer.Reset();
}

void Dive::Sample3DRenderer::Rotate(float radians)
{
	XMStoreFloat4x4(&m_constantBufferData.Model, XMMatrixTranspose(XMMatrixRotationY(radians)));
}
