#include "pch.h"
#include "FBXSceneCache.h"
#include "FBXSceneContext.h"

#include <string>

using namespace DirectX;
using namespace Dive;

FBXSceneContext::FBXSceneContext(char const* filename, FbxManager* fbxManager, std::shared_ptr<DX::DeviceResources> const& deviceResources) :
m_filename(filename),
m_manager(fbxManager),
m_scene(nullptr),
m_importer(nullptr),
m_currentAnimLayer(nullptr),
m_deviceResources(deviceResources)
{
}

bool FBXSceneContext::Initialize()
{
	m_scene = FbxScene::Create(m_manager, "");
	if (!m_scene)
	{
		_RPT0(2, "Error: Unable to create FBX scene!\n");
		return false;
	}

	int	fileFormat = -1;
	m_importer = FbxImporter::Create(m_manager, "");
	if (!m_importer)
	{
		_RPT0(2, "Error: Unable to create FBX importer!\n");
		return false;
	}
	if (!m_manager->GetIOPluginRegistry()->DetectReaderFileFormat(m_filename, fileFormat))
		fileFormat = m_manager->GetIOPluginRegistry()->FindReaderIDByDescription("FBX binary (*.fbx)");

	if (m_importer->Initialize(m_filename, fileFormat))
	{
		_RPT1(0, "Importing file %s\n", m_filename);

		if (m_importer->Import(m_scene))
		{
			FbxAxisSystem	sceneAxisSystem = m_scene->GetGlobalSettings().GetAxisSystem();
			FbxAxisSystem	ourAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
			if (sceneAxisSystem != ourAxisSystem)
				ourAxisSystem.ConvertScene(m_scene);

			FbxSystemUnit	sceneSystemUnit = m_scene->GetGlobalSettings().GetSystemUnit();
			if (sceneSystemUnit.GetScaleFactor() != 1.0)
				FbxSystemUnit::cm.ConvertScene(m_scene);

			m_scene->FillAnimStackNameArray(m_animStackNameArray);

			FillCameraArray();

			FbxGeometryConverter	geomConverter(m_manager);
			geomConverter.Triangulate(m_scene, true);

			geomConverter.SplitMeshesPerMaterial(m_scene, true);

			LoadCacheRecursive();
		}
	}

	return true;
}

void FBXSceneContext::Deinitialize()
{

}

void FBXSceneContext::FillCameraArray()
{
	m_cameraArray.Clear();

	FillCameraArrayRecursive(m_scene->GetRootNode());
}

void FBXSceneContext::FillCameraArrayRecursive(FbxNode* node)
{
	if (node)
	{
		if (node->GetNodeAttribute())
		{
			if (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eCamera)
				m_cameraArray.Add(node);
		}

		auto const	count = node->GetChildCount();
		for (auto i = 0; i < count; ++i)
			FillCameraArrayRecursive(node->GetChild(i));
	}
}

void FBXSceneContext::LoadCacheRecursive()
{
	auto const	textureCount = m_scene->GetTextureCount();
	for (auto textureIndex = 0; textureIndex < textureCount; ++textureIndex)
	{
		FbxTexture*		texture = m_scene->GetTexture(textureIndex);
		FbxFileTexture*	fileTexture = FbxCast<FbxFileTexture>(texture);
		if (fileTexture && !fileTexture->GetUserDataPtr())
		{
			FbxString const	filename = fileTexture->GetFileName();

			if (filename.Right(3).Upper() != "TGA")
			{
				_RPT1(0, "Only TGA textures are supported now: %s\n", filename.Buffer());
				continue;
			}

			DirectX::ScratchImage*	img = new DirectX::ScratchImage();
			DirectX::TexMetadata	info;
			HRESULT	status = LoadFromTGAFile(std::to_wstring(*filename).c_str(), &info, *img);

			FbxString const	absFbxFilename = FbxPathUtils::Resolve(m_filename);
			FbxString const	absFolderName = FbxPathUtils::GetFolderName(absFbxFilename);
			if (FAILED(status))
			{
				FbxString const	resolvedFilename = FbxPathUtils::Bind(absFolderName, fileTexture->GetRelativeFileName());
				status = LoadFromTGAFile(std::to_wstring(*resolvedFilename).c_str(), &info, *img);
			}

			if (FAILED(status))
			{
				FbxString const	textureFilename = FbxPathUtils::GetFileName(filename);
				FbxString const	resolvedFilename = FbxPathUtils::Bind(absFolderName, textureFilename);
				status = LoadFromTGAFile(std::to_wstring(*resolvedFilename).c_str(), &info, *img);
			}

			if (FAILED(status))
			{
				_RPT1(0, "Failed to load texture file: %s\n", filename.Buffer());
			}

			if (SUCCEEDED(status))
				fileTexture->SetUserDataPtr(img);
		}
	}

	LoadCacheRecursive(m_scene->GetRootNode());
}

void FBXSceneContext::LoadCacheRecursive(FbxNode* node)
{
	auto const	materialCount = node->GetMaterialCount();
	for (auto materialIndex = 0; materialIndex < materialCount; ++materialIndex)
	{
		FbxSurfaceMaterial*	material = node->GetMaterial(materialIndex);
		if (material && !material->GetUserDataPtr())
		{
			FbxAutoPtr<MaterialCache>	materialCache(new MaterialCache());
			if (materialCache->Initialize(material))
				material->SetUserDataPtr(materialCache.Release());
		}
	}

	FbxNodeAttribute*	nodeAttribute = node->GetNodeAttribute();
	if (nodeAttribute)
	{
		if (nodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			FbxMesh*	mesh = node->GetMesh();
			if (mesh && !mesh->GetUserDataPtr())
			{
				FbxAutoPtr<VBOMesh>	meshCache(new VBOMesh(m_deviceResources));
				if (meshCache->Initialize(mesh))
					mesh->SetUserDataPtr(meshCache.Release());
			}
		}
		else if (nodeAttribute->GetAttributeType() == FbxNodeAttribute::eLight)
		{
			// todo light cache
		}
	}

	auto const	childCount = node->GetChildCount();
	for (auto childIndex = 0; childIndex < childCount; ++childIndex)
		LoadCacheRecursive(node->GetChild(childIndex));
}