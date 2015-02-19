#pragma once

#include "fbxsdk.h"
#include "Common/DeviceResources.h"

namespace Dive
{
	class VBOMesh
	{
	public:
		VBOMesh(std::shared_ptr<DX::DeviceResources> const& m_deviceResources);
		~VBOMesh();

		bool	Initialize(FbxMesh const* mesh);

		void	UpdateVertexPosition(FbxMesh const* mesh, FbxVector4 const* vertices) const;
		int		GetSubMeshCount() const;

	private:
		enum
		{
			VERTEX_VBO,
			NORMAL_VBO,
			UV_VBO,
			INDEX_VBO,
			VBO_COUNT
		};

		struct SubMesh
		{
			SubMesh() : IndexOffset(0), TriangleCount(0) { }

			int	IndexOffset;
			int	TriangleCount;
		};

		std::shared_ptr<DX::DeviceResources>	m_deviceResources;

		FbxArray<SubMesh*>	m_subMeshes;
		bool				m_hasNormal;
		bool				m_hasUV;
		bool				m_allByControlPoint;

		Microsoft::WRL::ComPtr<ID3D11Buffer>	m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>	m_indexBuffer;
		uint32									m_indexCount;
	};

	class MaterialCache
	{
	public:
		MaterialCache();
		~MaterialCache();

		bool	Initialize(FbxSurfaceMaterial const* material);
		void	SetCurrentMaterials() const;
		bool	HasTexture() const;

		static void	SetDefaultMaterial();

	private:
		struct ColorChannel
		{
			ColorChannel() : m_texture(nullptr)
			{
				m_color[0] = 0.0f;
				m_color[1] = 0.0f;
				m_color[2] = 0.0f;
				m_color[3] = 1.0f;
			}

			DirectX::ScratchImage*	m_texture;
			float					m_color[4];
		};
		ColorChannel	m_emissive;
		ColorChannel	m_ambient;
		ColorChannel	m_diffuse;
		ColorChannel	m_specular;
		float			m_shinness;

	private:
		FbxDouble3	GetMaterialProperty(FbxSurfaceMaterial const* material, char const* propertyName, char const* factorPropertyName, DirectX::ScratchImage*& texture);
	};
}