/////////////////////////////////////////////////////////////////////////////////
/// Copyright (C), 2017, xizaixuan. All rights reserved.
/// \brief   Fbx����
/// \author  xizaixuan
/// \date    2017-08
/////////////////////////////////////////////////////////////////////////////////
#ifndef FbxLoader_H_
#define FbxLoader_H_

#include <vector>
#include "../RenderPipeLine/Utility/Singleton.h"
#include "fbxsdk.h"
#include "../RenderPipeLine/Mathematics/Float3.h"
#include <fbxsdk/scene/geometry/fbxmesh.h>
#include "../RenderPipeLine/Utility/RenderBuffer.h"
#include <fbxsdk/core/math/fbxaffinematrix.h>

using namespace std;

class FbxLoader : public Singleton<FbxLoader>
{
	SINGLETON_DEFINE(FbxLoader)
private:
	FbxLoader();
	~FbxLoader();

	void Initialize();
	void Finalize();

public:
	bool LoadScene(const char* pFilename, vector<RenderBuffer>& renderBuffer);

private:
	void ProcessContent(FbxScene* pScene, vector<RenderBuffer>& renderBuffer);
	void ProcessContent(FbxNode* pNode, vector<RenderBuffer>& renderBuffer);
	void ProcessMesh(FbxNode* pNode, vector<float3>& vertices, vector<int>& indices);
	void ProcessControlsPoints(FbxMesh* pMesh, vector<float3>& vertices, FbxAMatrix mat);
	void ProcessPolygons(FbxMesh* pMesh, vector<int>& indices);

public:
	FbxManager* m_pFbxManager;
	FbxScene* m_pFbxScene;
};

#endif