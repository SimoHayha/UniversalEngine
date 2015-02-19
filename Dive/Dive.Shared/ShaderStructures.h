#pragma once

namespace Dive
{
	struct ModelViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4	Model;
		DirectX::XMFLOAT4X4	View;
		DirectX::XMFLOAT4X4	Projection;
	};

	struct VertexPositionColor
	{
		DirectX::XMFLOAT3	Pos;
		DirectX::XMFLOAT3	Color;
	};

	struct VertexPositionColorNormalUV
	{
		DirectX::XMFLOAT3	Pos;
		DirectX::XMFLOAT3	Color;
		DirectX::XMFLOAT3	Normal;
		DirectX::XMFLOAT2	UV;
	};
}