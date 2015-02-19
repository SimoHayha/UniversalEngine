#include "pch.h"
#include "FBXManager.h"

#include <locale>

using namespace Dive;

FBXManager::FBXManager()
{
}

bool FBXManager::Initialize()
{
	m_manager = FbxManager::Create();
	if (!m_manager)
		return false;
	else
		_RPT1(0, "Autodesk FBX SDK version %s\n", m_manager->GetVersion());

	m_ioSettings = FbxIOSettings::Create(m_manager, IOSROOT);
	if (!m_ioSettings)
		return false;
	m_manager->SetIOSettings(m_ioSettings);

	return true;
}

void FBXManager::Deinitialize()
{
	for (auto& it : m_scenes)
		it->Destroy();
	m_manager->Destroy();
	m_ioSettings->Destroy();
	m_scenes.clear();
}

FbxScene* FBXManager::LoadScene(std::string const& filename)
{
	/*FbxImporter*	importer = FbxImporter::Create(m_manager, "");
	if (!importer)
	return nullptr;

	FbxScene*	scene = FbxScene::Create(m_manager, "");
	if (!scene)
	{
	_RPT0(2, "Unable to create FBX scene!\n");
	importer->Destroy();
	return nullptr;
	}

	int	fileformat = -1;
	if (!m_manager->GetIOPluginRegistry()->DetectReaderFileFormat(filename.c_str(), fileformat))
	fileformat = m_manager->GetIOPluginRegistry()->FindReaderIDByDescription("FBX binary (*.fbx)");

	if (importer->Initialize(filename.c_str(), fileformat))
	{
	_RPT1(0, "Importing file %s\nPlease wait!", filename.c_str());

	if (importer->Import(scene))
	{
	FbxAxisSystem	sceneAxisSystem = scene->GetGlobalSettings().GetAxisSystem();
	FbxAxisSystem	ourAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
	if (sceneAxisSystem != ourAxisSystem)
	ourAxisSystem.ConvertScene(scene);

	FbxSystemUnit	sceneSystemUnit = scene->GetGlobalSettings().GetSystemUnit();
	if (sceneSystemUnit.GetScaleFactor() != 1.0)
	FbxSystemUnit::cm.ConvertScene(scene);

	scene->FillAnimStackNameArray();
	}
	}
	else
	_RPT2(2, "Unable to open file %s\nError reported: %s\n", filename.c_str(), importer->GetStatus().GetErrorString());*/

	//FbxScene*		scene = FbxScene::Create(m_manager, filename.c_str());
	//if (!scene)
	//{
	//	importer->Destroy();
	//	return nullptr;
	//}

	//importer->Import(scene);
	//importer->SetName(filename.c_str());
	//importer->Destroy();

	//try
	//{
	//	m_scenes.push_back(scene);
	//}
	//catch (std::bad_alloc&)
	//{
	//	scene->Destroy();
	//	return nullptr;
	//}
	//return scene;

	return nullptr;
}
