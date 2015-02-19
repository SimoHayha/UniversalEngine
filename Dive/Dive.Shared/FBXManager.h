#pragma once

#include <list>
#include <string>

#include "fbxsdk.h"

namespace Dive
{
	class FBXManager
	{
	public:
		FBXManager();
		bool		Initialize();
		void		Deinitialize();
		FbxScene*	LoadScene(std::string const& name);

	private:
		FbxManager*						m_manager;
		FbxIOSettings*					m_ioSettings;
		std::list<FbxScene*>			m_scenes;
	};
}