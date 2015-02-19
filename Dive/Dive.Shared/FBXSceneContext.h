#pragma once

#include "fbxsdk.h"
#include "Common/DeviceResources.h"

namespace Dive
{
	class FBXSceneContext
	{
	public:
		FBXSceneContext(char const* filename, FbxManager* fbxManager, std::shared_ptr<DX::DeviceResources> const& m_deviceResources);

		bool	Initialize();
		void	Deinitialize();

	private:
		char const*	m_filename;

		FbxManager*		m_manager;
		FbxScene*		m_scene;
		FbxImporter*	m_importer;
		FbxAnimLayer*	m_currentAnimLayer;

		FbxArray<FbxString*>	m_animStackNameArray;
		FbxArray<FbxNode*>		m_cameraArray;
		FbxArray<FbxPose*>		m_poseArray;

		std::shared_ptr<DX::DeviceResources>	m_deviceResources;

	private:
		void	FillCameraArray();
		void	FillCameraArrayRecursive(FbxNode* node);
		void	LoadCacheRecursive();
		void	LoadCacheRecursive(FbxNode* node);
	};
}