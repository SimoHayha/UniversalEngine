#include "pch.h"
#include "ShaderStructures.h"
#include "Common/directxhelper.h"
#include "FBXSceneCache.h"

using namespace DirectX;
using namespace Dive;

namespace
{
	int const	TRIANGLE_VERTEX_COUNT = 3;
	int const	VERTEX_STRIDE = 4;
	int const	NORMAL_STRIDE = 3;
	int const	UV_STRIDE = 2;
}

VBOMesh::VBOMesh(std::shared_ptr<DX::DeviceResources> const& deviceResources) :
m_deviceResources(deviceResources),
m_hasNormal(false),
m_hasUV(false),
m_allByControlPoint(false),
m_indexCount(0)
{
}

VBOMesh::~VBOMesh()
{
	for (auto i = 0; i < m_subMeshes.GetCount(); ++i)
		delete m_subMeshes[i];
	m_subMeshes.Clear();
}

bool VBOMesh::Initialize(FbxMesh const* mesh)
{
	if (!mesh->GetNode())
		return false;

	int const	polygonCount = mesh->GetPolygonCount();

	FbxLayerElementArrayTemplate<int>*	materialIndices = nullptr;
	FbxGeometryElement::EMappingMode	materialMappingMode = FbxGeometryElement::eNone;
	if (mesh->GetElementMaterial())
	{
		materialIndices = &mesh->GetElementMaterial()->GetIndexArray();
		materialMappingMode = mesh->GetElementMaterial()->GetMappingMode();
		if (materialIndices && materialMappingMode == FbxGeometryElement::eByPolygon)
		{
			FBX_ASSERT(materialIndices->GetCount() == polygonCount);
			if (materialIndices->GetCount() == polygonCount)
			{
				// Count the faces of each material
				for (auto polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex)
				{
					auto const	materialIndex = materialIndices->GetAt(polygonIndex);
					if (m_subMeshes.GetCount() < materialIndex + 1)
						m_subMeshes.Resize(materialIndex + 1);
					if (!m_subMeshes[materialIndex])
						m_subMeshes[materialIndex] = new SubMesh();
					m_subMeshes[materialIndex]->TriangleCount += 1;
				}

				// Make sure we have no "holes" (NULL) in the m_subMeshes table. This can happen
				// if, in the loop above, we resized the m_subMeshes by more than one slot.
				for (auto i = 0; i < m_subMeshes.GetCount(); ++i)
				{
					if (!m_subMeshes[i])
						m_subMeshes[i] = new SubMesh();
				}

				// Record the offset (how many vertex)
				auto const		materialCount = m_subMeshes.GetCount();
				auto			offset = 0;
				for (auto index = 0; index < materialCount; ++index)
				{
					m_subMeshes[index]->IndexOffset = offset;
					offset += m_subMeshes[index]->TriangleCount * 3;
					// This will be used as counter in the following procedure, reset to zero
					m_subMeshes[index]->TriangleCount = 0;
				}
				FBX_ASSERT(offset == polygonCount * 3)
			}
		}
	}

	// All faces will use the same material.
	if (m_subMeshes.GetCount() == 0)
	{
		m_subMeshes.Resize(1);
		m_subMeshes[0] = new SubMesh();
	}

	// Congregate all the data of a mesh to be cached in VBOs.
	// If normal or UV is by polygon vertex, record all vertex attributes by polygon vertex.
	m_hasNormal = mesh->GetElementNormalCount() > 0;
	m_hasUV = mesh->GetElementUVCount() > 0;
	FbxGeometryElement::EMappingMode	normalMappingMode = FbxGeometryElement::eNone;
	FbxGeometryElement::EMappingMode	UVMappingMode = FbxGeometryElement::eNone;
	if (m_hasNormal)
	{
		normalMappingMode = mesh->GetElementNormal(0)->GetMappingMode();
		if (normalMappingMode == FbxGeometryElement::eNone)
			m_hasNormal = false;
		if (m_hasNormal && normalMappingMode != FbxGeometryElement::eByControlPoint)
			m_allByControlPoint = false;
	}

	if (m_hasUV)
	{
		UVMappingMode = mesh->GetElementUV(0)->GetMappingMode();
		if (UVMappingMode == FbxGeometryElement::eNone)
			m_hasUV = false;
		if (m_hasUV && UVMappingMode != FbxGeometryElement::eByControlPoint)
			m_allByControlPoint = false;
	}

	// Allocate the array memory, by control point or by polygon vertex.
	auto	polygonVertexCount = mesh->GetControlPointsCount();
	if (!m_allByControlPoint)
		polygonVertexCount = polygonCount * TRIANGLE_VERTEX_COUNT;
	float*			vertices = new float[polygonVertexCount * VERTEX_STRIDE];
	unsigned int*	indices = new unsigned int[polygonCount * TRIANGLE_VERTEX_COUNT];
	float*			normals = nullptr;
	if (m_hasNormal)
		normals = new float[polygonVertexCount * NORMAL_STRIDE];
	float*			UVs = nullptr;
	FbxStringList	UVNames;
	mesh->GetUVSetNames(UVNames);
	char const*		UVName = nullptr;
	if (m_hasUV && UVNames.GetCount())
	{
		UVs = new float[polygonVertexCount * UV_STRIDE];
		UVName = UVNames[0];
	}

	// Populate the array with vertex attributes, if by control point.
	FbxVector4 const*	controlPoints = mesh->GetControlPoints();
	FbxVector4			currentVertex;
	FbxVector4			currentNormal;
	FbxVector2			currentUV;
	if (m_allByControlPoint)
	{
		FbxGeometryElementNormal const*	normalElement = nullptr;
		FbxGeometryElementUV const*		UVElement = nullptr;
		if (m_hasNormal)
			normalElement = mesh->GetElementNormal(0);
		if (m_hasUV)
			UVElement = mesh->GetElementUV(0);
		for (auto index = 0; index < polygonVertexCount; ++index)
		{
			// Save the vertex position.
			currentVertex = controlPoints[index];
			vertices[index * VERTEX_STRIDE] = static_cast<float>(currentVertex[0]);
			vertices[index * VERTEX_STRIDE + 1] = static_cast<float>(currentVertex[1]);
			vertices[index * VERTEX_STRIDE + 2] = static_cast<float>(currentVertex[2]);
			vertices[index * VERTEX_STRIDE + 3] = 1.0f;

			// Save the normal.
			if (m_hasNormal)
			{
				auto	normalIndex = index;
				if (normalElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
					normalIndex = normalElement->GetIndexArray().GetAt(index);
				currentNormal = normalElement->GetDirectArray().GetAt(normalIndex);
				normals[index * NORMAL_STRIDE] = static_cast<float>(currentNormal[0]);
				normals[index * NORMAL_STRIDE + 1] = static_cast<float>(currentNormal[1]);
				normals[index * NORMAL_STRIDE + 2] = static_cast<float>(currentNormal[2]);
			}

			// Save the UV.
			if (m_hasUV)
			{
				auto	UVIndex = index;
				if (UVElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
					UVIndex = UVElement->GetIndexArray().GetAt(index);
				currentUV = UVElement->GetDirectArray().GetAt(UVIndex);
				UVs[index * UV_STRIDE] = static_cast<float>(currentUV[0]);
				UVs[index * UV_STRIDE + 1] = static_cast<float>(currentUV[1]);
			}
		}
	}

	auto	vertexCount = 0;
	for (auto polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex)
	{
		// The material for the current face.
		auto	materialIndex = 0;
		if (materialIndices && materialMappingMode == FbxGeometryElement::eByPolygon)
			materialIndex = materialIndices->GetAt(polygonIndex);

		// Where should I save the vertex attribute index, according to the material
		auto const	indexOffset = m_subMeshes[materialIndex]->IndexOffset +
								  m_subMeshes[materialIndex]->TriangleCount * 3;
		for (auto verticeIndex = 0; verticeIndex < TRIANGLE_VERTEX_COUNT; ++verticeIndex)
		{
			auto const	controlPointIndex = mesh->GetPolygonVertex(polygonIndex, verticeIndex);

			if (m_allByControlPoint)
				indices[indexOffset + verticeIndex] = static_cast<unsigned int>(controlPointIndex);
			// Populate the array with the vertex attribute, if by polygon vertex
			else
			{
				indices[indexOffset + verticeIndex] = static_cast<unsigned int>(vertexCount);

				currentVertex = controlPoints[controlPointIndex];
				vertices[vertexCount * VERTEX_STRIDE] = static_cast<float>(currentVertex[0]);
				vertices[vertexCount * VERTEX_STRIDE + 1] = static_cast<float>(currentVertex[1]);
				vertices[vertexCount * VERTEX_STRIDE + 2] = static_cast<float>(currentVertex[2]);
				vertices[vertexCount * VERTEX_STRIDE + 3] = 0.0f;

				if (m_hasNormal)
				{
					mesh->GetPolygonVertexNormal(polygonIndex, verticeIndex, currentNormal);
					normals[vertexCount * NORMAL_STRIDE] = static_cast<float>(currentNormal[0]);
					normals[vertexCount * NORMAL_STRIDE + 1] = static_cast<float>(currentNormal[1]);
					normals[vertexCount * NORMAL_STRIDE + 2] = static_cast<float>(currentNormal[2]);
				}

				if (m_hasUV)
				{
					bool	unmappedUV;
					mesh->GetPolygonVertexUV(polygonIndex, verticeIndex, UVName, currentUV, unmappedUV);
					UVs[vertexCount * UV_STRIDE] = static_cast<float>(currentUV[0]);
					UVs[vertexCount * UV_STRIDE + 1] = static_cast<float>(currentUV[1]);
				}
			}
			++vertexCount;
		}
		m_subMeshes[materialIndex]->TriangleCount += 1;
	}

	// Create usable directx object
	VertexPositionColorNormalUV*	dxObject = new VertexPositionColorNormalUV[polygonVertexCount]();
	for (auto vertexCount = 0; vertexCount < polygonVertexCount; vertexCount += VERTEX_STRIDE)
	{
		dxObject[vertexCount].Pos.x = vertices[vertexCount];
		dxObject[vertexCount + 1].Pos.y = vertices[vertexCount + 1];
		dxObject[vertexCount + 2].Pos.z = vertices[vertexCount + 2];

		if (m_hasNormal)
		{
			dxObject[vertexCount].Normal.x = normals[vertexCount];
			dxObject[vertexCount + 1].Normal.y = normals[vertexCount + 1];
			dxObject[vertexCount + 2].Normal.z = normals[vertexCount + 2];
		}

		if (m_hasUV)
		{
			dxObject[vertexCount].UV.x = UVs[vertexCount];
			dxObject[vertexCount + 1].UV.y = UVs[vertexCount + 1];
		}

		dxObject[vertexCount].Color.x = 1.0f;
		dxObject[vertexCount].Color.y = 1.0f;
		dxObject[vertexCount].Color.z = 1.0f;
	}

	D3D11_SUBRESOURCE_DATA	vertexBufferData = { 0 };
	vertexBufferData.pSysMem = dxObject;
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC	vertexBufferDesc(sizeof(dxObject), D3D11_BIND_VERTEX_BUFFER);
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
		&vertexBufferDesc,
		&vertexBufferData,
		&m_vertexBuffer
		)
		);

	m_indexCount = polygonCount * TRIANGLE_VERTEX_COUNT;

	D3D11_SUBRESOURCE_DATA	indexBufferData = { 0 };
	indexBufferData.pSysMem = indices;
	indexBufferData.SysMemPitch = 0;
	indexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC	indexBufferDesc(sizeof(unsigned int), D3D11_BIND_INDEX_BUFFER);
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
		&indexBufferDesc,
		&indexBufferData,
		&m_indexBuffer
		)
		);

	delete[] vertices;
	delete[] normals;
	delete[] UVs;

	return true;
}

void Dive::VBOMesh::UpdateVertexPosition(FbxMesh const* mesh, FbxVector4 const* vertices) const
{
	float*	newVertices = nullptr;
	auto	vertexCount = 0;
	if (m_allByControlPoint)
	{
		vertexCount = mesh->GetControlPointsCount();
		newVertices = new float[vertexCount * VERTEX_STRIDE];
		for (auto index = 0; index < vertexCount; ++index)
		{
			newVertices[index * VERTEX_STRIDE] = static_cast<float>(vertices[index][0]);
			newVertices[index * VERTEX_STRIDE + 1] = static_cast<float>(vertices[index][1]);
			newVertices[index * VERTEX_STRIDE + 2] = static_cast<float>(vertices[index][2]);
			newVertices[index * VERTEX_STRIDE + 3] = 1.0f;
		}
	}
	else
	{
		auto const	polygonCount = mesh->GetPolygonCount();
		vertexCount = polygonCount * TRIANGLE_VERTEX_COUNT;
		newVertices = new float[vertexCount * VERTEX_STRIDE];

		auto	vertexCount = 0;
		for (auto polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex)
		{
			for (auto verticeIndex = 0; verticeIndex < TRIANGLE_VERTEX_COUNT; ++verticeIndex)
			{
				auto const	controlPointIndex = mesh->GetPolygonVertex(polygonIndex, verticeIndex);
				newVertices[vertexCount * VERTEX_STRIDE] = static_cast<float>(vertices[controlPointIndex][0]);
				newVertices[vertexCount * VERTEX_STRIDE + 1] = static_cast<float>(vertices[controlPointIndex][1]);
				newVertices[vertexCount * VERTEX_STRIDE + 2] = static_cast<float>(vertices[controlPointIndex][2]);
				newVertices[vertexCount * VERTEX_STRIDE + 3] = 1.0f;
				++vertexCount;
			}
		}
	}

	if (newVertices)
	{
		D3D11_MAPPED_SUBRESOURCE	mappedResource;
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDeviceContext()->Map(
			m_vertexBuffer.Get(),
			0,
			D3D11_MAP_WRITE_NO_OVERWRITE,
			0,
			&mappedResource
			)
			);
		VertexPositionColorNormalUV*	dataPtr = (VertexPositionColorNormalUV*)mappedResource.pData;
		vertexCount *= TRIANGLE_VERTEX_COUNT;
		for (auto i = 0; i < vertexCount; i += VERTEX_STRIDE)
		{
			dataPtr[i].Pos.x = newVertices[i];
			dataPtr[i + 1].Pos.y = newVertices[i + 1];
			dataPtr[i + 2].Pos.z = newVertices[i + 2];
		}

		delete[] newVertices;

		m_deviceResources->GetD3DDeviceContext()->Unmap(m_vertexBuffer.Get(), 0);
	}
}

int Dive::VBOMesh::GetSubMeshCount() const
{
	return m_subMeshes.GetCount();
}

MaterialCache::MaterialCache() :
m_shinness(0)
{
}

MaterialCache::~MaterialCache()
{
}

bool MaterialCache::Initialize(FbxSurfaceMaterial const* material)
{
	FbxDouble3 const	emissive = GetMaterialProperty(material, FbxSurfaceMaterial::sEmissive, FbxSurfaceMaterial::sEmissiveFactor, m_emissive.m_texture);
	m_emissive.m_color[0] = static_cast<float>(emissive[0]);
	m_emissive.m_color[1] = static_cast<float>(emissive[1]);
	m_emissive.m_color[2] = static_cast<float>(emissive[2]);

	FbxDouble3 const	ambient = GetMaterialProperty(material, FbxSurfaceMaterial::sAmbient, FbxSurfaceMaterial::sAmbientFactor, m_emissive.m_texture);
	m_ambient.m_color[0] = static_cast<float>(emissive[0]);
	m_ambient.m_color[1] = static_cast<float>(emissive[1]);
	m_ambient.m_color[2] = static_cast<float>(emissive[2]);

	FbxDouble3 const	diffuse = GetMaterialProperty(material, FbxSurfaceMaterial::sDiffuse, FbxSurfaceMaterial::sDiffuseFactor, m_emissive.m_texture);
	m_diffuse.m_color[0] = static_cast<float>(emissive[0]);
	m_diffuse.m_color[1] = static_cast<float>(emissive[1]);
	m_diffuse.m_color[2] = static_cast<float>(emissive[2]);

	FbxDouble3 const	specular = GetMaterialProperty(material, FbxSurfaceMaterial::sSpecular, FbxSurfaceMaterial::sSpecularFactor, m_emissive.m_texture);
	m_specular.m_color[0] = static_cast<float>(emissive[0]);
	m_specular.m_color[1] = static_cast<float>(emissive[1]);
	m_specular.m_color[2] = static_cast<float>(emissive[2]);

	FbxProperty	shininessProperty = material->FindProperty(FbxSurfaceMaterial::sShininess);
	if (shininessProperty.IsValid())
	{
		double shininess = shininessProperty.Get<FbxDouble>();
		m_shinness = static_cast<float>(shininess);
	}

	return true;
}

void MaterialCache::SetCurrentMaterials() const
{
	// todo bind shader material and texture
}

void MaterialCache::SetDefaultMaterial()
{
	// todo bind shader default material and empty texture
}


bool MaterialCache::HasTexture() const
{
	return m_diffuse.m_texture != nullptr;
}

FbxDouble3 MaterialCache::GetMaterialProperty(FbxSurfaceMaterial const* material, char const* propertyName, char const* factorPropertyName, DirectX::ScratchImage*& pTexture)
{
	FbxDouble3			result(0.0, 0.0, 0.0);
	FbxProperty const	property = material->FindProperty(propertyName);
	FbxProperty const	factorProperty = material->FindProperty(factorPropertyName);
	if (property.IsValid() && factorProperty.IsValid())
	{
		result = property.Get<FbxDouble3>();
		double	factor = factorProperty.Get<FbxDouble>();
		if (factor != 1.0)
		{
			result[0] *= factor;
			result[1] *= factor;
			result[2] *= factor;
		}
	}

	if (property.IsValid())
	{
		auto const	textureCount = property.GetSrcObjectCount < FbxFileTexture>();
		if (textureCount)
		{
			FbxFileTexture const*	texture = property.GetSrcObject<FbxFileTexture>();
			if (texture && texture->GetUserDataPtr())
				pTexture = static_cast<DirectX::ScratchImage*>(texture->GetUserDataPtr());
		}
	}

	return result;
}

