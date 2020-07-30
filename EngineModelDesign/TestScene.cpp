#include"TestScene.h"
#pragma comment(lib, "D3D11.lib") 

PancySubModel::PancySubModel()
{
	model_mesh = NULL;
	material_use = 0;

}
PancySubModel::~PancySubModel()
{
	if (model_mesh != NULL)
	{
		delete model_mesh;
	}
}
PancyModelBasic::PancyModelBasic(const std::string &desc_file_in) :PancystarEngine::PancyBasicVirtualResource(desc_file_in)
{
	GetRootPath(desc_file_in);
}
void PancyModelBasic::GetRootPath(const std::string &desc_file_in)
{
	int end = 0;
	for (int32_t i = desc_file_in.size() - 1; i >= 0; --i)
	{
		if (desc_file_in[i] == '\\' || desc_file_in[i] == '/')
		{
			end = i + 1;
			break;
		}
	}
	model_root_path = desc_file_in.substr(0, end);
}
void PancyModelBasic::GetRenderMesh(std::vector<PancySubModel*> &render_mesh)
{
	for (auto sub_mesh_data = model_resource_list.begin(); sub_mesh_data < model_resource_list.end(); ++sub_mesh_data)
	{
		render_mesh.push_back(*sub_mesh_data);
	}
}
PancystarEngine::EngineFailReason PancyModelBasic::InitResource(const std::string &resource_desc_file)
{
	PancystarEngine::EngineFailReason check_error;
	check_error = LoadModel(resource_desc_file, model_resource_list, material_list, texture_list);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancyModelBasic::~PancyModelBasic()
{
	for (auto data_submodel = model_resource_list.begin(); data_submodel != model_resource_list.end(); ++data_submodel)
	{
		delete *data_submodel;
	}
	model_resource_list.clear();
	material_list.clear();
	for (auto id_tex = texture_list.begin(); id_tex != texture_list.end(); ++id_tex)
	{
		PancystarEngine::PancyTextureControl::GetInstance()->DeleteResurceReference(*id_tex);
	}
}
//FBX��񶯻�
mesh_animation_FBX::mesh_animation_FBX(std::string file_name_in)
{
	vertex_pack_num = 0;
	if_fbx_file = false;
	if (file_name_in.size() >= 3)
	{
		int tail = file_name_in.size() - 1;
		if (file_name_in[tail] == 'x' && file_name_in[tail - 1] == 'b' && file_name_in[tail - 2] == 'f')
		{
			if_fbx_file = true;
		}
	}
	lFilePath = new FbxString(file_name_in.c_str());
}
PancystarEngine::EngineFailReason mesh_animation_FBX::create(
	const std::vector<int32_t> &vertex_buffer_num_list,
	const std::vector<int32_t> &index_buffer_num_list,
	const std::vector<std::vector<IndexType>> &index_buffer_data_list,
	const std::vector<std::vector<DirectX::XMFLOAT2>> &UV_buffer_data_list
)
{
	if (!if_fbx_file)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "isn't a fbx file",error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error;
	InitializeSdkObjects(lSdkManager, lScene);
	if (lFilePath->IsEmpty())
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "need a file name",error_message);
		return error_message;
	}
	else
	{
		bool if_succeed = LoadScene(lSdkManager, lScene, lFilePath->Buffer());
		if (if_succeed == false)
		{
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(E_FAIL, "An error occurred while loading the scene" + std::string(lFilePath->Buffer()),error_message);
			return error_message;
		}
	}
	auto pNode = lScene->GetRootNode();
	//��ȡ�����Ϣ
	if (pNode == NULL)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "could not find the root node of FBX file" + std::string(lFilePath->Buffer()),error_message);
		return error_message;
	}
	find_tree_mesh(pNode);
	if (lMesh_list.size() == 0)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "could not find the mesh data in FBX file",error_message);
		
		return error_message;
	}
	if (lMesh_list.size() != index_buffer_num_list.size())
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "FBX mesh have different mesh part number with assimp & fbxsdk",error_message);
		
		return error_message;
	}
	anim_data_list.resize(lMesh_list.size());
	for (int i = 0; i < lMesh_list.size(); ++i)
	{
		int32_t lPolygonCount = lMesh_list[i]->GetPolygonCount();
		int32_t lPolygonCount_check = index_buffer_num_list[i];
		if (lPolygonCount != lPolygonCount_check)
		{
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(E_FAIL, "FBX mesh have different triangle num with assimp & fbxsdk",error_message);
			
			return error_message;
		}
		//���鶯����Ϣ
		int deformer_num = lMesh_list[i]->GetDeformerCount(FbxDeformer::eVertexCache);
		if (deformer_num > 0)
		{
			auto check_data = lMesh_list[i]->GetDeformer(0, FbxDeformer::eVertexCache);
			auto vertex_catch_data = static_cast<FbxVertexCacheDeformer*>(check_data);
			int a = 0;
		}
		const bool lHasVertexCache = lMesh_list[i]->GetDeformerCount(FbxDeformer::eVertexCache) &&
			(static_cast<FbxVertexCacheDeformer*>(lMesh_list[i]->GetDeformer(0, FbxDeformer::eVertexCache)))->Active.Get();
		if (!lHasVertexCache)
		{
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(E_FAIL, "the FBX file don't have animation message" + std::string(lFilePath->Buffer()),error_message);
			//return error_message;
		}
		//��ȡʱ����Ϣ
		PreparePointCacheData(lScene, anim_start, anim_end);
		auto FPS_rec = anim_end.GetFrameRate(fbxsdk::FbxTime::EMode::eDefaultMode);
		auto framenum_rec = anim_end.GetFrameCount();
		frame_per_second = static_cast<int>(FPS_rec);
		frame_num = static_cast<int>(framenum_rec);
		anim_frame.SetTime(0, 0, 0, 1, 0, lScene->GetGlobalSettings().GetTimeMode());
		if (frame_num == 0)
		{
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(E_FAIL, "could not find the mesh animation of FBX file" + std::string(lFilePath->Buffer()),error_message);
			//return error_message;
		}
		//����㶯�����
		auto time_now = anim_start;
		int num_vertex_now = lMesh_list[i]->GetControlPointsCount();
		FbxVector4* lVertexArray = NULL;
		lVertexArray = new FbxVector4[num_vertex_now];

		for (int j = 0; j < frame_num; ++j)
		{
			time_now += anim_frame;
			int check = time_now.GetFrameCount();
			check_error = ReadVertexCacheData(lMesh_list[i], time_now, lVertexArray);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			UpdateVertexPosition(i, lMesh_list[i], lVertexArray, vertex_buffer_num_list[i], index_buffer_data_list[i], UV_buffer_data_list[i]);
		}
	}

	for (int i = 0; i < vertex_buffer_num_list.size(); ++i)
	{
		vertex_pack_num += vertex_buffer_num_list[i];
	}
	//���㷨�
	//compute_normal();
	bool lResult = true;
	DestroySdkObjects(lSdkManager, lResult);
	check_error = build_buffer();
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}

PancystarEngine::EngineFailReason mesh_animation_FBX::build_buffer()
{
	return PancystarEngine::succeed;
}
void mesh_animation_FBX::release()
{
}
void mesh_animation_FBX::find_tree_mesh(FbxNode *pNode)
{
	FbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();
	if (lNodeAttribute && lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
	{
		auto lMesh = pNode->GetMesh();
		auto lVertexCount = lMesh->GetControlPointsCount();
		if (lVertexCount != 0)
		{
			lMesh_list.push_back(lMesh);
		}
	}
	const int lChildCount = pNode->GetChildCount();
	for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
	{
		find_tree_mesh(pNode->GetChild(lChildIndex));
	}
}
void mesh_animation_FBX::InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
{
	//The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
	pManager = FbxManager::Create();
	if (!pManager)
	{
		FBXSDK_printf("Error: Unable to create FBX Manager!\n");
		exit(1);
	}
	else FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());

	//Create an IOSettings object. This object holds all import/export settings.
	FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
	pManager->SetIOSettings(ios);

	//Load plugins from the executable directory (optional)
	FbxString lPath = FbxGetApplicationDirectory();
	pManager->LoadPluginsDirectory(lPath.Buffer());

	//Create an FBX scene. This object holds most objects imported/exported from/to files.
	pScene = FbxScene::Create(pManager, "My Scene");
	if (!pScene)
	{
		FBXSDK_printf("Error: Unable to create FBX scene!\n");
		exit(1);
	}
}
void mesh_animation_FBX::DestroySdkObjects(FbxManager* pManager, bool pExitStatus)
{
	//Delete the FBX Manager. All the objects that have been allocated using the FBX Manager and that haven't been explicitly destroyed are also automatically destroyed.
	if (pManager) pManager->Destroy();
	if (pExitStatus) FBXSDK_printf("Program Success!\n");
}
bool mesh_animation_FBX::SaveScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename, int pFileFormat, bool pEmbedMedia)
{
	int lMajor, lMinor, lRevision;
	bool lStatus = true;

	// Create an exporter.
	FbxExporter* lExporter = FbxExporter::Create(pManager, "");

	if (pFileFormat < 0 || pFileFormat >= pManager->GetIOPluginRegistry()->GetWriterFormatCount())
	{
		// Write in fall back format in less no ASCII format found
		pFileFormat = pManager->GetIOPluginRegistry()->GetNativeWriterFormat();

		//Try to export in ASCII if possible
		int lFormatIndex, lFormatCount = pManager->GetIOPluginRegistry()->GetWriterFormatCount();

		for (lFormatIndex = 0; lFormatIndex < lFormatCount; lFormatIndex++)
		{
			if (pManager->GetIOPluginRegistry()->WriterIsFBX(lFormatIndex))
			{
				FbxString lDesc = pManager->GetIOPluginRegistry()->GetWriterFormatDescription(lFormatIndex);
				const char *lASCII = "ascii";
				if (lDesc.Find(lASCII) >= 0)
				{
					pFileFormat = lFormatIndex;
					break;
				}
			}
		}
	}

	// Set the export states. By default, the export states are always set to 
	// true except for the option eEXPORT_TEXTURE_AS_EMBEDDED. The code below 
	// shows how to change these states.
	IOS_REF.SetBoolProp(EXP_FBX_MATERIAL, true);
	IOS_REF.SetBoolProp(EXP_FBX_TEXTURE, true);
	IOS_REF.SetBoolProp(EXP_FBX_EMBEDDED, pEmbedMedia);
	IOS_REF.SetBoolProp(EXP_FBX_SHAPE, true);
	IOS_REF.SetBoolProp(EXP_FBX_GOBO, true);
	IOS_REF.SetBoolProp(EXP_FBX_ANIMATION, true);
	IOS_REF.SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);

	// Initialize the exporter by providing a filename.
	if (lExporter->Initialize(pFilename, pFileFormat, pManager->GetIOSettings()) == false)
	{
		FBXSDK_printf("Call to FbxExporter::Initialize() failed.\n");
		FBXSDK_printf("Error returned: %s\n\n", lExporter->GetStatus().GetErrorString());
		return false;
	}

	FbxManager::GetFileFormatVersion(lMajor, lMinor, lRevision);
	FBXSDK_printf("FBX file format version %d.%d.%d\n\n", lMajor, lMinor, lRevision);

	// Export the scene.
	lStatus = lExporter->Export(pScene);

	// Destroy the exporter.
	lExporter->Destroy();
	return lStatus;
}
bool mesh_animation_FBX::LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename)
{
	int lFileMajor, lFileMinor, lFileRevision;
	int lSDKMajor, lSDKMinor, lSDKRevision;
	//int lFileFormat = -1;
	int i, lAnimStackCount;
	bool lStatus;
	char lPassword[1024];

	// Get the file version number generate by the FBX SDK.
	FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

	// Create an importer.
	FbxImporter* lImporter = FbxImporter::Create(pManager, "");

	// Initialize the importer by providing a filename.
	const bool lImportStatus = lImporter->Initialize(pFilename, -1, pManager->GetIOSettings());
	lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

	if (!lImportStatus)
	{
		FbxString error = lImporter->GetStatus().GetErrorString();
		FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
		FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

		if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
		{
			FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
			FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);
		}

		return false;
	}

	FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

	if (lImporter->IsFBX())
	{
		FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);

		// From this point, it is possible to access animation stack information without
		// the expense of loading the entire file.

		FBXSDK_printf("Animation Stack Information\n");

		lAnimStackCount = lImporter->GetAnimStackCount();

		FBXSDK_printf("    Number of Animation Stacks: %d\n", lAnimStackCount);
		FBXSDK_printf("    Current Animation Stack: \"%s\"\n", lImporter->GetActiveAnimStackName().Buffer());
		FBXSDK_printf("\n");

		for (i = 0; i < lAnimStackCount; i++)
		{
			FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

			FBXSDK_printf("    Animation Stack %d\n", i);
			FBXSDK_printf("         Name: \"%s\"\n", lTakeInfo->mName.Buffer());
			FBXSDK_printf("         Description: \"%s\"\n", lTakeInfo->mDescription.Buffer());

			// Change the value of the import name if the animation stack should be imported 
			// under a different name.
			FBXSDK_printf("         Import Name: \"%s\"\n", lTakeInfo->mImportName.Buffer());

			// Set the value of the import state to false if the animation stack should be not
			// be imported. 
			FBXSDK_printf("         Import State: %s\n", lTakeInfo->mSelect ? "true" : "false");
			FBXSDK_printf("\n");
		}

		// Set the import states. By default, the import states are always set to 
		// true. The code below shows how to change these states.
		IOS_REF.SetBoolProp(IMP_FBX_MATERIAL, true);
		IOS_REF.SetBoolProp(IMP_FBX_TEXTURE, true);
		IOS_REF.SetBoolProp(IMP_FBX_LINK, true);
		IOS_REF.SetBoolProp(IMP_FBX_SHAPE, true);
		IOS_REF.SetBoolProp(IMP_FBX_GOBO, true);
		IOS_REF.SetBoolProp(IMP_FBX_ANIMATION, true);
		IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	}

	// Import the scene.
	lStatus = lImporter->Import(pScene);

	if (lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
	{
		FBXSDK_printf("Please enter password: ");

		lPassword[0] = '\0';

		FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
			scanf("%s", lPassword);
		FBXSDK_CRT_SECURE_NO_WARNING_END

			FbxString lString(lPassword);

		IOS_REF.SetStringProp(IMP_FBX_PASSWORD, lString);
		IOS_REF.SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

		lStatus = lImporter->Import(pScene);

		if (lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
		{
			FBXSDK_printf("\nPassword is wrong, import aborted.\n");
		}
	}

	// Destroy the importer.
	lImporter->Destroy();

	return lStatus;
}
void mesh_animation_FBX::PreparePointCacheData(FbxScene* pScene, FbxTime &pCache_Start, FbxTime &pCache_Stop)
{
	// This function show how to cycle through scene elements in a linear way.
	const int lNodeCount = pScene->GetSrcObjectCount<FbxNode>();
	FbxStatus lStatus;

	for (int lIndex = 0; lIndex < lNodeCount; lIndex++)
	{
		FbxNode* lNode = pScene->GetSrcObject<FbxNode>(lIndex);

		if (lNode->GetGeometry())
		{
			int i, lVertexCacheDeformerCount = lNode->GetGeometry()->GetDeformerCount(FbxDeformer::eVertexCache);

			// There should be a maximum of 1 Vertex Cache Deformer for the moment
			lVertexCacheDeformerCount = lVertexCacheDeformerCount > 0 ? 1 : 0;

			for (i = 0; i < lVertexCacheDeformerCount; ++i)
			{
				// Get the Point Cache object
				FbxVertexCacheDeformer* lDeformer = static_cast<FbxVertexCacheDeformer*>(lNode->GetGeometry()->GetDeformer(i, FbxDeformer::eVertexCache));
				if (!lDeformer) continue;
				FbxCache* lCache = lDeformer->GetCache();
				if (!lCache) continue;

				// Process the point cache data only if the constraint is active
				if (lDeformer->Active.Get())
				{
					auto data_check = lCache->GetCacheFileFormat();
					if (lCache->GetCacheFileFormat() == FbxCache::eMaxPointCacheV2)
					{
						// This code show how to convert from PC2 to MC point cache format
						// turn it on if you need it.
#if 0 
						if (!lCache->ConvertFromPC2ToMC(FbxCache::eMCOneFile,
							FbxTime::GetFrameRate(pScene->GetGlobalTimeSettings().GetTimeMode())))
						{
							// Conversion failed, retrieve the error here
							FbxString lTheErrorIs = lCache->GetStaus().GetErrorString();
						}
#endif
					}
					else if (lCache->GetCacheFileFormat() == FbxCache::eMayaCache)
					{
						// This code show how to convert from MC to PC2 point cache format
						// turn it on if you need it.
						//#if 0 
						if (!lCache->ConvertFromMCToPC2(FbxTime::GetFrameRate(pScene->GetGlobalSettings().GetTimeMode()), 0, &lStatus))
						{
							// Conversion failed, retrieve the error here
							FbxString lTheErrorIs = lStatus.GetErrorString();
						}
						//#endif
					}


					// Now open the cache file to read from it
					if (!lCache->OpenFileForRead(&lStatus))
					{
						// Cannot open file 
						FbxString lTheErrorIs = lStatus.GetErrorString();

						// Set the deformer inactive so we don't play it back
						lDeformer->Active = false;
					}
					else
					{
						// get the start and stop time of the cache
						FbxTime lChannel_Start;
						FbxTime lChannel_Stop;
						int lChannelIndex = lCache->GetChannelIndex(lDeformer->Channel.Get());
						if (lCache->GetAnimationRange(lChannelIndex, lChannel_Start, lChannel_Stop))
						{
							// get the smallest start time
							if (lChannel_Start < pCache_Start) pCache_Start = lChannel_Start;

							// get the biggest stop time
							if (lChannel_Stop > pCache_Stop)  pCache_Stop = lChannel_Stop;
						}
					}
				}
			}
		}
	}
}
DirectX::XMFLOAT3 mesh_animation_FBX::get_normal_vert(FbxMesh * pMesh, int vertex_count)
{
	DirectX::XMFLOAT3 pNormal;
	int rec = pMesh->GetElementNormalCount();
	FbxGeometryElementNormal* leNormal = pMesh->GetElementNormal(0);
	auto mode_map = leNormal->GetMappingMode();
	auto mode_reference = leNormal->GetReferenceMode();
	if (mode_reference == FbxGeometryElement::eDirect)
	{
		pNormal.x = leNormal->GetDirectArray().GetAt(vertex_count)[0];
		pNormal.y = leNormal->GetDirectArray().GetAt(vertex_count)[1];
		pNormal.z = leNormal->GetDirectArray().GetAt(vertex_count)[2];
	}
	else
	{
		int id = leNormal->GetIndexArray().GetAt(vertex_count);
		pNormal.x = leNormal->GetDirectArray().GetAt(id)[0];
		pNormal.y = leNormal->GetDirectArray().GetAt(id)[1];
		pNormal.z = leNormal->GetDirectArray().GetAt(id)[2];
	}
	return pNormal;
}
PancystarEngine::EngineFailReason mesh_animation_FBX::ReadVertexCacheData(FbxMesh* pMesh, FbxTime& pTime, FbxVector4* pVertexArray)
{
	FbxVertexCacheDeformer* lDeformer = static_cast<FbxVertexCacheDeformer*>(pMesh->GetDeformer(0, FbxDeformer::eVertexCache));
	FbxCache*               lCache = lDeformer->GetCache();
	int                     lChannelIndex = lCache->GetChannelIndex(lDeformer->Channel.Get());
	unsigned int            lVertexCount = (unsigned int)pMesh->GetControlPointsCount();
	bool                    lReadSucceed = false;
	float*                  lReadBuf = NULL;
	unsigned int			BufferSize = 0;
	FbxString lChnlName, lChnlInterp;
	lCache->GetChannelInterpretation(lChannelIndex, lChnlInterp);
	if (lDeformer->Type.Get() != FbxVertexCacheDeformer::ePositions)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "animation data type not support",error_message);
		return error_message;
	}
	unsigned int Length = 0;
	lCache->Read(NULL, Length, FBXSDK_TIME_ZERO, lChannelIndex);
	if (Length != lVertexCount * 3)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "the content of the cache is by vertex not by control points (we don't support it here)",error_message);
		return error_message;
	}
	lReadSucceed = lCache->Read(&lReadBuf, BufferSize, pTime, lChannelIndex);
	//lReadSucceed = lCache->Read(&lReadBuf, BufferSize, pTime, 1);
	if (lReadSucceed)
	{
		unsigned int lReadBufIndex = 0;
		while (lReadBufIndex < 3 * lVertexCount)
		{
			pVertexArray[lReadBufIndex / 3].mData[0] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
			pVertexArray[lReadBufIndex / 3].mData[1] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
			pVertexArray[lReadBufIndex / 3].mData[2] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
		}
	}
	else
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "read animation data error",error_message);
		return error_message;
	}
	/*
	//߼�����Ƶ�任Ч�
	FbxVector4 check_rec1 = pMesh->GetControlPointAt(500);
	std::vector<XMFLOAT3> normle_need,normle_need2;
	for (int i = 0; i < pMesh->GetPolygonCount() * 3; ++i)
	{
		normle_need.push_back(get_normal_vert(pMesh,i));
	}
	for (int i = 0; i < pMesh->GetControlPointsCount(); ++i)
	{
		FbxVector4 pVertices_now = pVertexArray[i];
		pMesh->SetControlPointAt(pVertices_now, i);
	}
	FbxVector4 check_rec2 = pMesh->GetControlPointAt(500);
	//��µķ��߼����
	bool check_tre = pMesh->GenerateNormals(true);
	bool check_tre2 = pMesh->GenerateTangentsDataForAllUVSets(true);
	//����¼��
	for (int i = 0; i < pMesh->GetPolygonCount() * 3; ++i)
	{
		normle_need2.push_back(get_normal_vert(pMesh, i));
	}
	*/


	return PancystarEngine::succeed;
}
void mesh_animation_FBX::UpdateVertexPosition(
	const int32_t &animation_id,
	FbxMesh * pMesh,
	const FbxVector4 * pVertices,
	const int32_t &vertex_num_assimp,
	const std::vector<IndexType> &index_assimp,
	const std::vector<DirectX::XMFLOAT2> &uv_assimp
)
{
	//鴴�����assimpڵĶ������
	std::vector<mesh_animation_data> now_frame_data;
	now_frame_data.resize(vertex_num_assimp);
	//��ȡfbx������
	int TRIANGLE_VERTEX_COUNT = 3;
	int VERTEX_STRIDE = 4;
	// Convert to the same sequence with data in GPU.
	float * lVertices = NULL;
	int lVertexCount = 0;
	const int lPolygonCount = pMesh->GetPolygonCount();
	lVertexCount = lPolygonCount * TRIANGLE_VERTEX_COUNT;
	lVertices = new float[lVertexCount * VERTEX_STRIDE];

	lVertexCount = 0;
	std::vector<float> vertex_normal_num;
	for (int i = 0; i < vertex_num_assimp; ++i)
	{
		vertex_normal_num.push_back(0);
	}
	for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
	{

		//ݻ�ȡ��ǰ��Ӧ�assimpĶ��
		int traingle_point_0 = lPolygonIndex * TRIANGLE_VERTEX_COUNT + 0;
		int traingle_point_1 = lPolygonIndex * TRIANGLE_VERTEX_COUNT + 1;
		int traingle_point_2 = lPolygonIndex * TRIANGLE_VERTEX_COUNT + 2;
		const int lControlPointIndex_0 = pMesh->GetPolygonVertex(lPolygonIndex, 0);
		const int lControlPointIndex_1 = pMesh->GetPolygonVertex(lPolygonIndex, 1);
		const int lControlPointIndex_2 = pMesh->GetPolygonVertex(lPolygonIndex, 2);
		int vertex_index_assimp_0 = index_assimp[traingle_point_2];
		int vertex_index_assimp_1 = index_assimp[traingle_point_1];
		int vertex_index_assimp_2 = index_assimp[traingle_point_0];
		now_frame_data[vertex_index_assimp_0].position.x = static_cast<float>(pVertices[lControlPointIndex_0][0]);
		now_frame_data[vertex_index_assimp_0].position.y = static_cast<float>(pVertices[lControlPointIndex_0][1]);
		now_frame_data[vertex_index_assimp_0].position.z = -static_cast<float>(pVertices[lControlPointIndex_0][2]);

		now_frame_data[vertex_index_assimp_1].position.x = static_cast<float>(pVertices[lControlPointIndex_1][0]);
		now_frame_data[vertex_index_assimp_1].position.y = static_cast<float>(pVertices[lControlPointIndex_1][1]);
		now_frame_data[vertex_index_assimp_1].position.z = -static_cast<float>(pVertices[lControlPointIndex_1][2]);

		now_frame_data[vertex_index_assimp_2].position.x = static_cast<float>(pVertices[lControlPointIndex_2][0]);
		now_frame_data[vertex_index_assimp_2].position.y = static_cast<float>(pVertices[lControlPointIndex_2][1]);
		now_frame_data[vertex_index_assimp_2].position.z = -static_cast<float>(pVertices[lControlPointIndex_2][2]);


		lVertexCount += 3;
	}
	DirectX::XMFLOAT3 *point_buffer = new DirectX::XMFLOAT3[now_frame_data.size()];
	DirectX::XMFLOAT3 *normal_buffer = new DirectX::XMFLOAT3[now_frame_data.size()];
	DirectX::XMFLOAT4 *tangent_buffer = new DirectX::XMFLOAT4[now_frame_data.size()];
	for (int i = 0; i < now_frame_data.size(); ++i)
	{
		point_buffer[i] = now_frame_data[i].position;
	}
	DirectX::ComputeNormals(&index_assimp[0], index_assimp.size() / 3, point_buffer, now_frame_data.size(), DirectX::CNORM_DEFAULT, normal_buffer);
	DirectX::ComputeTangentFrame(&index_assimp[0], index_assimp.size() / 3, point_buffer, normal_buffer, &uv_assimp[0], now_frame_data.size(), tangent_buffer);
	for (int i = 0; i < now_frame_data.size(); ++i)
	{
		now_frame_data[i].normal = normal_buffer[i];
		now_frame_data[i].tangent.x = tangent_buffer[i].x;
		now_frame_data[i].tangent.y = tangent_buffer[i].y;
		now_frame_data[i].tangent.z = tangent_buffer[i].z;
		/*
		//���㷨�
		DirectX::XMFLOAT3 Tangent1, Tangent2;
		Tangent1.x = now_frame_data[index_assimp[i]].position.x - now_frame_data[index_assimp[i + 1]].position.x;
		Tangent1.y = now_frame_data[index_assimp[i]].position.y - now_frame_data[index_assimp[i + 1]].position.y;
		Tangent1.z = now_frame_data[index_assimp[i]].position.z - now_frame_data[index_assimp[i + 1]].position.z;

		Tangent2.x = now_frame_data[index_assimp[i]].position.x - now_frame_data[index_assimp[i + 2]].position.x;
		Tangent2.y = now_frame_data[index_assimp[i]].position.y - now_frame_data[index_assimp[i + 2]].position.y;
		Tangent2.z = now_frame_data[index_assimp[i]].position.z - now_frame_data[index_assimp[i + 2]].position.z;
		DirectX::XMVECTOR new_normal = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&Tangent1)), DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&Tangent2))));
		DirectX::XMFLOAT3 normal_data;
		DirectX::XMStoreFloat3(&normal_data, DirectX::XMVector3Normalize(new_normal));
		//߽�������Ϣ�������붥�
		now_frame_data[index_assimp[i]].normal.x = (now_frame_data[index_assimp[i]].normal.x*vertex_normal_num[index_assimp[i]] + normal_data.x) / (vertex_normal_num[index_assimp[i]] + 1);
		now_frame_data[index_assimp[i]].normal.y = (now_frame_data[index_assimp[i]].normal.y*vertex_normal_num[index_assimp[i]] + normal_data.y) / (vertex_normal_num[index_assimp[i]] + 1);
		now_frame_data[index_assimp[i]].normal.z = (now_frame_data[index_assimp[i]].normal.z*vertex_normal_num[index_assimp[i]] + normal_data.z) / (vertex_normal_num[index_assimp[i]] + 1);
		vertex_normal_num[index_assimp[i]] += 1;
		now_frame_data[index_assimp[i + 1]].normal.x = (now_frame_data[index_assimp[i + 1]].normal.x*vertex_normal_num[index_assimp[i + 1]] + normal_data.x) / (vertex_normal_num[index_assimp[i + 1]] + 1);
		now_frame_data[index_assimp[i + 1]].normal.y = (now_frame_data[index_assimp[i + 1]].normal.y*vertex_normal_num[index_assimp[i + 1]] + normal_data.y) / (vertex_normal_num[index_assimp[i + 1]] + 1);
		now_frame_data[index_assimp[i + 1]].normal.z = (now_frame_data[index_assimp[i + 1]].normal.z*vertex_normal_num[index_assimp[i + 1]] + normal_data.z) / (vertex_normal_num[index_assimp[i + 1]] + 1);
		vertex_normal_num[index_assimp[i + 1]] += 1;
		now_frame_data[index_assimp[i + 2]].normal.x = (now_frame_data[index_assimp[i + 2]].normal.x*vertex_normal_num[index_assimp[i + 2]] + normal_data.x) / (vertex_normal_num[index_assimp[i + 2]] + 1);
		now_frame_data[index_assimp[i + 2]].normal.y = (now_frame_data[index_assimp[i + 2]].normal.y*vertex_normal_num[index_assimp[i + 2]] + normal_data.y) / (vertex_normal_num[index_assimp[i + 2]] + 1);
		now_frame_data[index_assimp[i + 2]].normal.z = (now_frame_data[index_assimp[i + 2]].normal.z*vertex_normal_num[index_assimp[i + 2]] + normal_data.z) / (vertex_normal_num[index_assimp[i + 2]] + 1);
		vertex_normal_num[index_assimp[i + 2]] += 1;
		*/
	}
	delete[] point_buffer;
	delete[] normal_buffer;
	anim_data_list[animation_id].push_back(now_frame_data);
	int a = 0;
}
void mesh_animation_FBX::GetMeshAnimData(mesh_animation_data *data)
{
	int32_t count_data = 0;
	for (int i = 0; i < frame_num; ++i)
	{
		for (int j = 0; j < anim_data_list.size(); ++j)
		{
			for (int k = 0; k < anim_data_list[j][i].size(); ++k)
			{
				data[count_data] = anim_data_list[j][i][k];
				count_data += 1;
			}
		}
	}
}
/*
void mesh_animation_FBX::compute_normal()
{
	FbxGeometryConverter lGeometryConverter(lSdkManager);
	int triangle_num = point_index_num / 3;

	DirectX::XMFLOAT3 *positions = new DirectX::XMFLOAT3[point_num];
	DirectX::XMFLOAT3 *normals = new DirectX::XMFLOAT3[point_num];
	UINT *index_test = new UINT[point_index_num];
	for (int i = 0; i < triangle_num; ++i)
	{
		index_test[3 * i + 0] = index_buffer[3 * i + 0];
		index_test[3 * i + 1] = index_buffer[3 * i + 1];
		index_test[3 * i + 2] = index_buffer[3 * i + 2];
	}
	for (auto now_frame_data = anim_data_list.begin(); now_frame_data != anim_data_list.end(); ++now_frame_data)
	{

		for (int i = 0; i < now_frame_data._Ptr->point_num; ++i)
		{
			positions[i] = now_frame_data._Ptr->point_data[i].position;
		}
		int a = 0;
	}
}
*/
//ASSIMP�ģ�ͽ��
PancyModelAssimp::PancyModelAssimp(const std::string &desc_file_in, const std::string &pso_in) :PancyModelBasic(desc_file_in)
{
	model_move_skin = NULL;
	if_animation_choose = false;
	model_pbr_type = PbrType_MetallicRoughness;
	if_skinmesh = false;
	if_pointmesh = false;
	pso_use = pso_in;
	model_boundbox = NULL;
	model_size.max_box_pos.x = -999999999.0f;
	model_size.max_box_pos.y = -999999999.0f;
	model_size.max_box_pos.z = -999999999.0f;

	model_size.min_box_pos.x = 999999999.0f;
	model_size.min_box_pos.y = 999999999.0f;
	model_size.min_box_pos.z = 999999999.0f;
	//������
	root_skin = new skin_tree;
	strcpy(root_skin->bone_ID, "root_node");
	root_skin->son = new skin_tree;
	bone_num = 0;
	for (int i = 0; i < 100; ++i)
	{
		DirectX::XMStoreFloat4x4(&bone_matrix_array[i], DirectX::XMMatrixIdentity());
		DirectX::XMStoreFloat4x4(&offset_matrix_array[i], DirectX::XMMatrixIdentity());
		DirectX::XMStoreFloat4x4(&final_matrix_array[i], DirectX::XMMatrixIdentity());
	}
	for (int i = 0; i < 100; ++i)
	{
		for (int j = 0; j < 100; ++j)
		{
			tree_node_num[i][j] = 0;
		}
	}
	//FBXݵ㻺����
	FBXanim_import = NULL;
	//ݶ�����
	now_animation_play_station = 0.0f;
}
PancyModelAssimp::~PancyModelAssimp()
{
	for (int i = 0; i < cbuffer.size(); ++i)
	{
		SubresourceControl::GetInstance()->FreeSubResource(cbuffer[i]);
	}
	//todo:�ɾ������
	PancyDescriptorHeapControl::GetInstance()->FreeResourceView(table_offset[0].resource_view_pack_id);
	delete model_boundbox;
}
PancystarEngine::EngineFailReason PancyModelAssimp::BuildTextureRes(std::string tex_name, const int &if_force_srgb, pancy_object_id &id_tex)
{
	PancystarEngine::EngineFailReason check_error;
	std::string texture_file_name, tex_real_name;
	int32_t st_pos = 0;
	texture_file_name = tex_name;
	for (int32_t i = 0; i < tex_name.size(); ++i)
	{
		if (tex_name[i] == '\\' || tex_name[i] == '/')
		{
			st_pos = i + 1;
		}
	}
	if (st_pos < tex_name.size())
	{
		tex_real_name = tex_name.substr(st_pos, tex_name.size() - st_pos);
	}
	else
	{
		tex_real_name = "";
	}
	//�ǿ�ƽ������޸�Ϊdds
	int32_t name_length = 0;
	for (int i = 0; i < tex_real_name.size(); ++i)
	{
		if (tex_real_name[i] != '.')
		{
			name_length += 1;
		}
		else
		{
			break;
		}
	}
	//��������json��ļ���
	string json_file_out = model_root_path + tex_real_name.substr(0, name_length) + ".json";
	texture_file_name = tex_real_name.substr(0, name_length) + ".json";
	tex_real_name = tex_real_name.substr(0, name_length) + ".dds";
	if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(json_file_out))
	{
		//�Ϊ�json���������һ�������ʽ�
		Json::Value json_data_out;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfFromFile", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "FileName", tex_real_name);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfAutoBuildMipMap", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfForceSrgb", if_force_srgb);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "MaxSize", 0);
		check_error = PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, json_file_out);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		//��ļ���Ϊ�Ѿ�����
		PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(json_file_out);
	}
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(model_root_path + texture_file_name, id_tex);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
pancy_object_id PancyModelAssimp::insert_new_texture(std::vector<pancy_object_id> &texture_use, const pancy_object_id &tex_id)
{
	for (int i = 0; i < texture_use.size(); ++i)
	{
		if (texture_use[i] == tex_id)
		{
			return i;
		}
	}
	texture_use.push_back(tex_id);
	return texture_use.size() - 1;
}
void PancyModelAssimp::SaveBoneTree(skin_tree *bone_data)
{
	out_stream.write("*heaphead*", sizeof("*heaphead*"));
	out_stream.write(reinterpret_cast<char *>(bone_data), sizeof(*bone_data));
	if (bone_data->son != NULL)
	{
		SaveBoneTree(bone_data->son);
	}
	out_stream.write("*heaptail*", sizeof("*heaptail*"));
	if (bone_data->brother != NULL)
	{
		SaveBoneTree(bone_data->brother);
	}
}
PancystarEngine::EngineFailReason PancyModelAssimp::LoadModel(
	const std::string &resource_desc_file,
	std::vector<PancySubModel*> &model_resource,
	std::unordered_map<pancy_object_id, std::unordered_map<TexType, pancy_object_id>> &material_list,
	std::vector<pancy_object_id> &texture_use
)
{
	PancystarEngine::EngineFailReason check_error;
	PbrMaterialType pbr_type;
	std::string model_name = resource_desc_file;
	bool if_auto_material = false;
	bool if_self_lod = false;
	std::unordered_map<TexType, string> tex_tail_name;//���ֲ��ʵ�β׺
	std::unordered_map<TexType, pancy_object_id> tex_empty;//���ڴ��治��������Ŀհ����
	std::vector<std::string> texture_not_find;//�洢δ�ҵ������
	//��ȡģ�͵ĸ�ʽ
	if (CheckIFJson(resource_desc_file))
	{
		pancy_json_value rec_value;
		Json::Value root_value;
		check_error = PancyJsonTool::GetInstance()->LoadJsonFile(resource_desc_file, root_value);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		//ģ���ļ��
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, root_value, "ModelFileName", pancy_json_data_type::json_data_string, rec_value);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		model_name = model_root_path + rec_value.string_value;
		//��Ƿ��json��ļ��Զ��������
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, root_value, "IfBuildMaterial", pancy_json_data_type::json_data_bool, rec_value);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		if_auto_material = rec_value.bool_value;
		//�ģ���Ƿ��Ѿ���lod��
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, root_value, "IfSelfLod", pancy_json_data_type::json_data_bool, rec_value);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		if_self_lod = rec_value.bool_value;
		//�ģ���Ƿ����������Ϣ
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, root_value, "IfHaveSkinAnimation", pancy_json_data_type::json_data_bool, rec_value);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		if_skinmesh = rec_value.bool_value;
		//ģ���Ƿ��㶯����Ϣ
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, root_value, "IfHavePoinAnimation", pancy_json_data_type::json_data_bool, rec_value);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		if_pointmesh = rec_value.bool_value;
		//ģ�Pbr͸�ʽ
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, root_value, "PbrType", pancy_json_data_type::json_data_enum, rec_value);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		model_pbr_type = static_cast<PbrMaterialType>(rec_value.int_value);
		//Ԥ���������
		if (if_auto_material)
		{
			Json::Value material_value = root_value.get("MaterialPack", Json::Value::null);
			int num_material = material_value.size();
			for (int i = 0; i < num_material; ++i)
			{
				std::unordered_map<TexType, pancy_object_id> mat_tex_list;
				//ݲ�����
				std::string material_name;
				//ƻ����ͼ
				std::string tex_albedo;
				std::string tex_normal;
				std::string tex_ambient;
				//�������ͼ
				std::string tex_metallic;
				std::string tex_roughness;
				//����&�ƽ����ͼ
				std::string tex_specsmooth;
				pancy_object_id id_need;
				//������
				check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value[i], "MaterialName", pancy_json_data_type::json_data_string, rec_value);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				material_name = rec_value.string_value;
				//�������
				check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value[i], "Albedotex", pancy_json_data_type::json_data_string, rec_value);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				tex_albedo = rec_value.string_value;
				check_error = BuildTextureRes(tex_albedo.c_str(), 0, id_need);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				pancy_object_id now_texture_id_diffuse = insert_new_texture(texture_use, id_need);
				mat_tex_list.insert(std::pair<TexType, pancy_object_id>(TexType::tex_diffuse, now_texture_id_diffuse));
				//texture_use.push_back(id_need);
				//ʷ�����ͼ���
				check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value[i], "Normaltex", pancy_json_data_type::json_data_string, rec_value);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				tex_normal = rec_value.string_value;
				check_error = BuildTextureRes(tex_normal.c_str(), 0, id_need);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				now_texture_id_diffuse = insert_new_texture(texture_use, id_need);
				mat_tex_list.insert(std::pair<TexType, pancy_object_id>(TexType::tex_normal, now_texture_id_diffuse));
				//texture_use.push_back(id_need);
				//ʻ������ڱβ��
				check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value[i], "Ambienttex", pancy_json_data_type::json_data_string, rec_value);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				tex_ambient = rec_value.string_value;
				check_error = BuildTextureRes(tex_ambient.c_str(), 0, id_need);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				now_texture_id_diffuse = insert_new_texture(texture_use, id_need);
				mat_tex_list.insert(std::pair<TexType, pancy_object_id>(TexType::tex_ambient, now_texture_id_diffuse));
				//texture_use.push_back(id_need);
				//Pbr����
				if (model_pbr_type == PbrMaterialType::PbrType_MetallicRoughness)
				{
					//����Ȳ��
					check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value[i], "MetallicTex", pancy_json_data_type::json_data_string, rec_value);
					if (!check_error.if_succeed)
					{
						return check_error;
					}
					tex_metallic = rec_value.string_value;
					check_error = BuildTextureRes(tex_metallic.c_str(), 0, id_need);
					if (!check_error.if_succeed)
					{
						return check_error;
					}
					now_texture_id_diffuse = insert_new_texture(texture_use, id_need);
					mat_tex_list.insert(std::pair<TexType, pancy_object_id>(TexType::tex_metallic, now_texture_id_diffuse));
					//texture_use.push_back(id_need);
					//ʴֲڶȲ��
					check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value[i], "RoughnessTex", pancy_json_data_type::json_data_string, rec_value);
					if (!check_error.if_succeed)
					{
						return check_error;
					}
					tex_roughness = rec_value.string_value;
					check_error = BuildTextureRes(tex_roughness.c_str(), 0, id_need);
					if (!check_error.if_succeed)
					{
						return check_error;
					}
					now_texture_id_diffuse = insert_new_texture(texture_use, id_need);
					mat_tex_list.insert(std::pair<TexType, pancy_object_id>(TexType::tex_roughness, now_texture_id_diffuse));
					//texture_use.push_back(id_need);
				}
				else if (model_pbr_type == PbrMaterialType::PbrType_SpecularSmoothness)
				{
					//ʾ���&�ƽ�������
					check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value[i], "SpecsmoothnessTex", pancy_json_data_type::json_data_string, rec_value);
					if (!check_error.if_succeed)
					{
						return check_error;
					}
					tex_specsmooth = rec_value.string_value;
					check_error = BuildTextureRes(tex_specsmooth.c_str(), 0, id_need);
					if (!check_error.if_succeed)
					{
						return check_error;
					}
					now_texture_id_diffuse = insert_new_texture(texture_use, id_need);
					mat_tex_list.insert(std::pair<TexType, pancy_object_id>(TexType::tex_specular_smoothness, now_texture_id_diffuse));
					//texture_use.push_back(id_need);
				}
				//������ʼ�����ʱ
				material_list.insert(std::pair<pancy_object_id, std::unordered_map<TexType, pancy_object_id>>(i, mat_tex_list));
			}
		}
		else
		{
			//���ʵĺ�׺��
			Json::Value material_value = root_value.get("MaterialTail", Json::Value::null);
			std::string tex_tail;
			//��������
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value, "Albedotex", pancy_json_data_type::json_data_string, rec_value);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			tex_tail_name.insert(std::pair<TexType, string>(TexType::tex_diffuse, rec_value.string_value));
			//�������ͼ
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value, "Normaltex", pancy_json_data_type::json_data_string, rec_value);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			tex_tail_name.insert(std::pair<TexType, string>(TexType::tex_normal, rec_value.string_value));
			//�������ڱ���ͼ
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value, "Ambienttex", pancy_json_data_type::json_data_string, rec_value);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			tex_tail_name.insert(std::pair<TexType, string>(TexType::tex_ambient, rec_value.string_value));
			if (model_pbr_type == PbrMaterialType::PbrType_MetallicRoughness)
			{
				//�������ͼ
				check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value, "MetallicTex", pancy_json_data_type::json_data_string, rec_value);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				tex_tail_name.insert(std::pair<TexType, string>(TexType::tex_metallic, rec_value.string_value));
				//�ֲڶ���ͼ
				check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value, "RoughnessTex", pancy_json_data_type::json_data_string, rec_value);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				tex_tail_name.insert(std::pair<TexType, string>(TexType::tex_roughness, rec_value.string_value));
			}
			else if (model_pbr_type == PbrMaterialType::PbrType_SpecularSmoothness)
			{
				//����&�⻬����ͼ
				check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value, "SpecsmoothnessTex", pancy_json_data_type::json_data_string, rec_value);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				tex_tail_name.insert(std::pair<TexType, string>(TexType::tex_specular_smoothness, rec_value.string_value));
			}

			//�հ�������
			Json::Value empty_material_value = root_value.get("MaterialEmpty", Json::Value::null);
			pancy_object_id id_empty_tex = 0;
			//�����������
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, empty_material_value, "EmptyNormal", pancy_json_data_type::json_data_string, rec_value);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			string empty_tex_name = rec_value.string_value;
			check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(empty_tex_name, id_empty_tex);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			tex_empty.insert(std::pair<TexType, pancy_object_id>(TexType::tex_normal, id_empty_tex));
			//ao�������
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, empty_material_value, "EmptyAmbient", pancy_json_data_type::json_data_string, rec_value);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			empty_tex_name = rec_value.string_value;
			check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(empty_tex_name, id_empty_tex);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			tex_empty.insert(std::pair<TexType, pancy_object_id>(TexType::tex_ambient, id_empty_tex));
			if (model_pbr_type == PbrMaterialType::PbrType_MetallicRoughness)
			{
				//��������
				check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, empty_material_value, "EmptyMetallic", pancy_json_data_type::json_data_string, rec_value);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				empty_tex_name = rec_value.string_value;
				check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(empty_tex_name, id_empty_tex);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				tex_empty.insert(std::pair<TexType, pancy_object_id>(TexType::tex_metallic, id_empty_tex));
				//�ֲڶ����
				check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, empty_material_value, "EmptyRoughness", pancy_json_data_type::json_data_string, rec_value);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				empty_tex_name = rec_value.string_value;
				check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(empty_tex_name, id_empty_tex);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				tex_empty.insert(std::pair<TexType, pancy_object_id>(TexType::tex_roughness, id_empty_tex));
			}
			else if (model_pbr_type == PbrMaterialType::PbrType_SpecularSmoothness)
			{
				//�ֲڶ&Ⱦ�������
				check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, empty_material_value, "EmptySpecsmoothness", pancy_json_data_type::json_data_string, rec_value);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				empty_tex_name = rec_value.string_value;
				check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(empty_tex_name, id_empty_tex);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				tex_empty.insert(std::pair<TexType, pancy_object_id>(TexType::tex_specular_smoothness, id_empty_tex));
			}

		}
		//�Ԥ���LOD���
		if (if_self_lod)
		{
			Json::Value Lod_value = root_value.get("LodDivide", Json::Value::null);
			int num_lod_value = Lod_value.size();
			for (int i = 0; i < num_lod_value; ++i)
			{
				int num_block_data = Lod_value[i].size();
				std::vector<int32_t> Lod_block_list;
				for (int j = 0; j < num_block_data; ++j)
				{
					check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, Lod_value[i], j, pancy_json_data_type::json_data_int, rec_value);
					if (!check_error.if_succeed)
					{
						return check_error;
					}
					Lod_block_list.push_back(rec_value.int_value);
				}
				model_lod_divide.push_back(Lod_block_list);
			}
		}
	}

	//ݿ�ʼ����ģ�
	const aiScene *model_need;//assimp�ģ�ͱ��

	model_need = importer.ReadFile(model_name,
		aiProcess_MakeLeftHanded |
		aiProcess_FlipWindingOrder |
		aiProcess_CalcTangentSpace |
		aiProcess_JoinIdenticalVertices
	);//ݽ���ͬͼԪ���õ���ͬ��ģ����ȥ��ͼƬ���Ϳ����ǵ㡢ֱ�ߡ�����ε
	const char *error_str;
	error_str = importer.GetErrorString();
	if (model_need == NULL)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "read model" + resource_desc_file + "error",error_message);
		
		return error_message;
	}

	std::vector<int32_t> vertex_num;
	std::vector<int32_t> index_num;
	std::vector<std::vector<IndexType>> index_data;
	std::vector<std::vector<DirectX::XMFLOAT2>> UV_data;
	for (int i = 0; i < model_need->mNumMeshes; i++)
	{
		std::vector<DirectX::XMFLOAT2> new_uv_list;
		std::vector<IndexType> index_pack;
		const aiMesh* paiMesh = model_need->mMeshes[i];
		vertex_num.push_back(paiMesh->mNumVertices);
		index_num.push_back(paiMesh->mNumFaces);
		for (unsigned int j = 0; j < paiMesh->mNumFaces; j++)
		{
			if (paiMesh->mFaces[j].mNumIndices == 3)
			{
				index_pack.push_back(static_cast<IndexType>(paiMesh->mFaces[j].mIndices[0]));
				index_pack.push_back(static_cast<IndexType>(paiMesh->mFaces[j].mIndices[1]));
				index_pack.push_back(static_cast<IndexType>(paiMesh->mFaces[j].mIndices[2]));
			}
			else
			{
				PancystarEngine::EngineFailReason error_message;
				PancyDebugLogError(E_FAIL, "model" + resource_desc_file + "find no triangle face",error_message);
				
				return error_message;

			}
		}
		for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
		{
			DirectX::XMFLOAT2 now_uv;
			if (paiMesh->HasTextureCoords(0))
			{
				now_uv.x = paiMesh->mTextureCoords[0][j].x;
				now_uv.y = paiMesh->mTextureCoords[0][j].y;
				now_uv.y = 1 - now_uv.y;
			}
			else
			{
				now_uv.x = 0.0f;
				now_uv.y = 0.0f;
			}
			new_uv_list.push_back(now_uv);
		}
		UV_data.push_back(new_uv_list);
		index_data.push_back(index_pack);
	}
	std::unordered_map<pancy_object_id, pancy_object_id> real_material_list;//����������ʺ�Ĳ��ʱ����֮ǰ�ı�ŶԱ
	if (!if_auto_material)
	{
		int32_t real_material_num = 0;
		for (unsigned int i = 0; i < model_need->mNumMaterials; ++i)
		{
			std::unordered_map<TexType, pancy_object_id> mat_tex_list;
			const aiMaterial* pMaterial = model_need->mMaterials[i];
			aiString Path;
			//��������
			if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0 && pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
			{
				pancy_object_id id_need;
				check_error = BuildTextureRes(Path.C_Str(), 0, id_need);
				if (!check_error.if_succeed)
				{
					return check_error;
				}
				string common_file_name = Path.C_Str();
				//��޸�����·������ȡ������
				int32_t st_pos = 0;
				std::string tex_real_name;
				for (int32_t i = 0; i < common_file_name.size(); ++i)
				{
					if (common_file_name[i] == '\\' || common_file_name[i] == '/')
					{
						st_pos = i + 1;
					}
				}
				if (st_pos < common_file_name.size())
				{
					tex_real_name = common_file_name.substr(st_pos, common_file_name.size() - st_pos);
				}
				else
				{
					tex_real_name = "";
				}
				//ƻ�ȡ�������Ҫ��
				int32_t name_length = 0;
				for (int i = 0; i < tex_real_name.size(); ++i)
				{
					if (tex_real_name[i] != '.')
					{
						name_length += 1;
					}
					else
					{
						break;
					}
				}
				auto diffuse_mask_name = tex_tail_name.find(TexType::tex_diffuse);

				if (diffuse_mask_name != tex_tail_name.end())
				{
					std::string diffuse_tail = diffuse_mask_name->second;
					//ƻ�ȡ������
					string name_diffuse = tex_real_name.substr(0, name_length) + ".dds";
					int start_compare = name_diffuse.size() - diffuse_tail.size();
					for (int i = 0; i < diffuse_tail.size(); ++i)
					{
						if (diffuse_tail[i] != name_diffuse[start_compare + i])
						{
							PancystarEngine::EngineFailReason error_message;
							PancyDebugLogError(E_FAIL, "diffuse tail dismatch in model: " + resource_desc_file,error_message);
							
							return error_message;
						}
					}
					string root_name = name_diffuse.substr(0, start_compare);
					material_name_list.insert(std::pair<pancy_object_id, std::string>(real_material_num, root_name));
					//ƽ�������ݼ��ص����ʱ
					auto now_texture_id_diffuse = insert_new_texture(texture_use, id_need);
					mat_tex_list.insert(std::pair<TexType, pancy_object_id>(TexType::tex_diffuse, now_texture_id_diffuse));
					//texture_use.push_back(id_need);
					for (auto tex_deal = tex_tail_name.begin(); tex_deal != tex_tail_name.end(); ++tex_deal)
					{
						if (tex_deal->first != TexType::tex_diffuse)
						{
							if (tex_deal->second != "")
							{
								bool if_have_file = true;
								std::string tex_name_combine = root_name + tex_deal->second;
								//�����ļ��Ƿ���
								fstream _file;
								_file.open(model_root_path + tex_name_combine, ios::in);
								if (!_file)
								{
									if_have_file = false;
								}
								_file.close();
								if (if_have_file)
								{
									check_error = BuildTextureRes(tex_name_combine, 0, id_need);
									if (!check_error.if_succeed)
									{
										return check_error;
									}
								}
								else
								{
									//������ļ���ڣ�ʹ�ÿհ�ͼƬ���
									texture_not_find.push_back(tex_name_combine);
									auto empty_texture_now = tex_empty.find(tex_deal->first);
									if (empty_texture_now != tex_empty.end())
									{
										id_need = empty_texture_now->second;
									}
									else
									{
										PancystarEngine::EngineFailReason error_message;
										PancyDebugLogError(E_FAIL, "empty texture haven't load: " + resource_desc_file,error_message);
										
										return error_message;
									}
								}

							}
							else
							{
								//������ļ���ڣ�ʹ�ÿհ�ͼƬ���
								texture_not_find.push_back(root_name + "::empty");
								auto empty_texture_now = tex_empty.find(tex_deal->first);
								if (empty_texture_now != tex_empty.end())
								{
									id_need = empty_texture_now->second;
								}
								else
								{
									PancystarEngine::EngineFailReason error_message;
									PancyDebugLogError(E_FAIL, "empty texture haven't load: " + resource_desc_file,error_message);
									
									return error_message;
								}
							}
							//潫������ݼ��ص����ʱ
							auto now_texture_id_other = insert_new_texture(texture_use, id_need);
							mat_tex_list.insert(std::pair<TexType, pancy_object_id>(tex_deal->first, now_texture_id_other));
							//texture_use.push_back(id_need);
						}
					}
				}
				else
				{
					material_name_list.insert(std::pair<pancy_object_id, std::string>(real_material_num, tex_real_name.substr(0, name_length)));
					//���������ݼ��ص����ʱ
					auto now_texture_id_diffuse = insert_new_texture(texture_use, id_need);
					mat_tex_list.insert(std::pair<TexType, pancy_object_id>(TexType::tex_diffuse, now_texture_id_diffuse));
					//texture_use.push_back(id_need);
				}

				material_list.insert(std::pair<pancy_object_id, std::unordered_map<TexType, pancy_object_id>>(real_material_num, mat_tex_list));
				real_material_list.insert(std::pair<pancy_object_id, pancy_object_id>(i, real_material_num));
				real_material_num += 1;
			}
		}
	}
	else
	{
		//�ֻ�����ʶ�Ӧ��������ȡ������Ϣ
		if (model_need->mNumMaterials == material_list.size())
		{
			for (int i = 0; i < material_list.size(); ++i)
			{
				real_material_list.insert(std::pair<pancy_object_id, pancy_object_id>(i, i));
			}
		}
		else
		{
			pancy_object_id st_pos = model_need->mNumMaterials - material_list.size();
			for (int i = 0; i < st_pos; ++i)
			{
				real_material_list.insert(std::pair<pancy_object_id, pancy_object_id>(i, 0));
			}
			for (int i = st_pos; i < model_need->mNumMaterials; ++i)
			{
				real_material_list.insert(std::pair<pancy_object_id, pancy_object_id>(i, i - st_pos));
			}
		}
	}
	if (model_lod_divide.size() == 0)
	{
		std::vector<int32_t> Lod_block_list;
		for (int i = 0; i < model_need->mNumMeshes; i++)
		{
			Lod_block_list.push_back(i);
		}
		model_lod_divide.push_back(Lod_block_list);
	}
	//���ض��㶯����Ϣ
	if (if_pointmesh)
	{
		//���Զ�ȡ���㶯��
		FBXanim_import = new mesh_animation_FBX(model_name);
		check_error = FBXanim_import->create(vertex_num, index_num, index_data, UV_data);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		int point_number = FBXanim_import->GetMeshAnimNumber();
		mesh_animation_data *new_data = new mesh_animation_data[point_number];
		FBXanim_import->GetMeshAnimData(new_data);
		//�������㶯����ݵ�buffer
		SubMemoryPointer vertex_buffer_upload;
		PancyRenderCommandList *copy_render_list;
		uint32_t copy_render_list_ID;
		auto copy_contex = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(NULL, &copy_render_list, copy_render_list_ID);
		check_error = BuildDefaultBuffer(copy_render_list->GetCommandList().Get(), 4194304, 131072, vertex_anim_buffer, vertex_buffer_upload, new_data, point_number * sizeof(mesh_animation_data), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		//�����Ⱦ���в��ύ����
		copy_render_list->UnlockPrepare();
		ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(1, &copy_render_list_ID);
		//�GPU��ϲ�һ������λ
		ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_DIRECT)->SetGpuBrokenFence(upload_fence_value);
		//�ȴ���
		ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_DIRECT)->WaitGpuBrokenFence(upload_fence_value);
		//�ɾ��̬��Դ
		SubresourceControl::GetInstance()->FreeSubResource(vertex_buffer_upload);
		delete[] new_data;
	}
	//Ԥ���ع�����Ϣ
	if (if_skinmesh)
	{
		build_skintree(model_need->mRootNode, root_skin->son);
	}
	//��伸������Ϣ
	int now_used_bone_num = 0;

	int mesh_vertex_offset = 0;
	for (int i = 0; i < model_need->mNumMeshes; i++)
	{
		const aiMesh* paiMesh = model_need->mMeshes[i];
		//��ȡģ�͵Ĳ��ʱ�
		auto real_material_find = real_material_list.find(paiMesh->mMaterialIndex);
		if (real_material_find == real_material_list.end())
		{
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(E_FAIL, "the material id: " + std::to_string(paiMesh->mMaterialIndex) + " of model " + resource_desc_file + " have been delete(do not have diffuse map)",error_message);
			
			return error_message;
		}
		pancy_object_id material_use = real_material_find->second;

		auto mat_list_now = material_list.find(material_use);
		if (mat_list_now == material_list.end())
		{
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(E_FAIL, "havn't load the material id: " + std::to_string(material_use) + "in model:" + resource_desc_file,error_message);
			
			return error_message;
		}
		//ż�������ƫ���
		int mat_stat_id = 0;
		for (int j = 0; j < material_use; ++j)
		{
			mat_stat_id += material_list[j].size();
		}
		//����������
		IndexType *index_need = new IndexType[paiMesh->mNumFaces * 3];
		for (unsigned int j = 0; j < paiMesh->mNumFaces; j++)
		{
			if (paiMesh->mFaces[j].mNumIndices == 3)
			{
				index_need[j * 3 + 0] = static_cast<IndexType>(paiMesh->mFaces[j].mIndices[0]);
				index_need[j * 3 + 1] = static_cast<IndexType>(paiMesh->mFaces[j].mIndices[1]);
				index_need[j * 3 + 2] = static_cast<IndexType>(paiMesh->mFaces[j].mIndices[2]);
			}
			else
			{
				PancystarEngine::EngineFailReason error_message;
				PancyDebugLogError(E_FAIL, "model" + resource_desc_file + "find no triangle face",error_message);
				
				return error_message;
			}
		}
		//������
		if (if_skinmesh)
		{
			PancystarEngine::PointSkinCommon8 *point_need = new PancystarEngine::PointSkinCommon8[paiMesh->mNumVertices];
			check_error = BuildModelData(point_need, paiMesh, mat_stat_id);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			//���䶯����
			float *bone_removed_weight = new float[paiMesh->mNumVertices];
			//���չ�����Ƥ��Ϣ
			for (int j = 0; j < paiMesh->mNumVertices; ++j)
			{
				point_need[j].bone_id.x = (MaxBoneNum + 99) * (MaxBoneNum + 100) + (MaxBoneNum + 99);
				point_need[j].bone_id.y = (MaxBoneNum + 99) * (MaxBoneNum + 100) + (MaxBoneNum + 99);
				point_need[j].bone_id.z = (MaxBoneNum + 99) * (MaxBoneNum + 100) + (MaxBoneNum + 99);
				point_need[j].bone_id.w = (MaxBoneNum + 99) * (MaxBoneNum + 100) + (MaxBoneNum + 99);
				point_need[j].bone_weight0.x = 0.0f;
				point_need[j].bone_weight0.y = 0.0f;
				point_need[j].bone_weight0.z = 0.0f;
				point_need[j].bone_weight0.w = 0.0f;
				point_need[j].bone_weight1.x = 0.0f;
				point_need[j].bone_weight1.y = 0.0f;
				point_need[j].bone_weight1.z = 0.0f;
				point_need[j].bone_weight1.w = 0.0f;
				bone_removed_weight[j] = 0.0f;
			}
			//Ԥ�����Ƥ��Ϣ
			for (int j = 0; j < paiMesh->mNumBones; ++j)
			{
				skin_tree * now_node = find_tree(root_skin, paiMesh->mBones[j]->mName.data);
				if (now_node->bone_number == NouseAssimpStruct)
				{
					now_node->bone_number = now_used_bone_num++;
					bone_num += 1;
				}
				tree_node_num[i][j] = now_node->bone_number;
				for (int k = 0; k < paiMesh->mBones[j]->mNumWeights; ++k)
				{
					//�Ƚ�8�����idĺ����ȡ���Ա�����ʹ�
					uint32_t bone_id[8];
					bone_id[0] = point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_id.x / (MaxBoneNum + 100);
					bone_id[1] = point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_id.y / (MaxBoneNum + 100);
					bone_id[2] = point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_id.z / (MaxBoneNum + 100);
					bone_id[3] = point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_id.w / (MaxBoneNum + 100);
					bone_id[4] = point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_id.x % (MaxBoneNum + 100);
					bone_id[5] = point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_id.y % (MaxBoneNum + 100);
					bone_id[6] = point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_id.z % (MaxBoneNum + 100);
					bone_id[7] = point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_id.w % (MaxBoneNum + 100);
					float bone_weight[8];
					bone_weight[0] = point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_weight0.x;
					bone_weight[1] = point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_weight0.y;
					bone_weight[2] = point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_weight0.z;
					bone_weight[3] = point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_weight0.w;
					bone_weight[4] = point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_weight1.x;
					bone_weight[5] = point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_weight1.y;
					bone_weight[6] = point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_weight1.z;
					bone_weight[7] = point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_weight1.w;
					//���ѡһ�ID���еĹ�����ݽ���д�
					bool if_success_load = false;
					for (int i = 0; i < 8; ++i)
					{
						if (bone_id[i] == MaxBoneNum + 99)
						{
							bone_id[i] = now_node->bone_number;
							bone_weight[i] = paiMesh->mBones[j]->mWeights[k].mWeight;
							if_success_load = true;
							break;
						}
					}
					//���8������ռ������ѡһ��Ȩ������Ĺ�������Ƚ��Ƿ���Ҫ���
					if (!if_success_load)
					{
						int min_id = 0;
						float min_weight = 99.99;
						for (int i = 0; i < 8; ++i)
						{
							if (min_weight > bone_weight[i])
							{
								min_id = i;
								min_weight = bone_weight[i];
							}
						}
						if (bone_weight[min_id] < paiMesh->mBones[j]->mWeights[k].mWeight)
						{
							//½����Ƴ�Ĺ���Ȩ�ر����Ա�֮����е�ͼ�Ȩ
							bone_removed_weight[paiMesh->mBones[j]->mWeights[k].mVertexId] += bone_weight[min_id];
							//ʹ���µĹ������֮ǰȨ�ؽ�С�Ĺ��
							bone_id[min_id] = now_node->bone_number;
							bone_weight[min_id] = paiMesh->mBones[j]->mWeights[k].mWeight;
						}
						else
						{
							bone_removed_weight[paiMesh->mBones[j]->mWeights[k].mVertexId] += paiMesh->mBones[j]->mWeights[k].mWeight;
						}
					}
					//��������ϵĹ�����Ƥ��Ϣ��ԭ�����
					point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_id.x = bone_id[0] * (MaxBoneNum + 100) + bone_id[4];
					point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_id.y = bone_id[1] * (MaxBoneNum + 100) + bone_id[5];
					point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_id.z = bone_id[2] * (MaxBoneNum + 100) + bone_id[6];
					point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_id.w = bone_id[3] * (MaxBoneNum + 100) + bone_id[7];

					point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_weight0.x = bone_weight[0];
					point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_weight0.y = bone_weight[1];
					point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_weight0.z = bone_weight[2];
					point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_weight0.w = bone_weight[3];

					point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_weight1.x = bone_weight[4];
					point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_weight1.y = bone_weight[5];
					point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_weight1.z = bone_weight[6];
					point_need[paiMesh->mBones[j]->mWeights[k].mVertexId].bone_weight1.w = bone_weight[7];


				}
			}
			//㽫���������Ƥ��Ϣ�������ʹ�õĹ��
			for (int j = 0; j < paiMesh->mNumVertices; ++j)
			{
				float now_bone_weight[8];
				//������Ȩ��ȡ�
				now_bone_weight[0] = point_need[j].bone_weight0.x;
				now_bone_weight[1] = point_need[j].bone_weight0.y;
				now_bone_weight[2] = point_need[j].bone_weight0.z;
				now_bone_weight[3] = point_need[j].bone_weight0.w;

				now_bone_weight[4] = point_need[j].bone_weight1.x;
				now_bone_weight[5] = point_need[j].bone_weight1.y;
				now_bone_weight[6] = point_need[j].bone_weight1.z;
				now_bone_weight[7] = point_need[j].bone_weight1.w;
				float final_weight_use = 0.0f;
				for (int k = 0; k < 8; ++k)
				{
					final_weight_use += now_bone_weight[k];
				}
				for (int k = 0; k < 8; ++k)
				{
					now_bone_weight[k] += (now_bone_weight[k] / final_weight_use) * bone_removed_weight[j];
				}
				//������Ĺ���Ȩ�ػָ�
				point_need[j].bone_weight0.x = now_bone_weight[0];
				point_need[j].bone_weight0.y = now_bone_weight[1];
				point_need[j].bone_weight0.z = now_bone_weight[2];
				point_need[j].bone_weight0.w = now_bone_weight[3];
				point_need[j].bone_weight1.x = now_bone_weight[4];
				point_need[j].bone_weight1.y = now_bone_weight[5];
				point_need[j].bone_weight1.z = now_bone_weight[6];
				point_need[j].bone_weight1.w = now_bone_weight[7];
			}
			PancySubModel *new_submodel = new PancySubModel();
			check_error = new_submodel->Create(point_need, index_need, paiMesh->mNumVertices, paiMesh->mNumFaces * 3, material_use);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			model_resource.push_back(new_submodel);
			delete[] point_need;
			delete[] bone_removed_weight;
		}
		else if (if_pointmesh)
		{
			PancystarEngine::PointCatchCommon *point_need = new PancystarEngine::PointCatchCommon[paiMesh->mNumVertices];
			check_error = BuildModelData(point_need, paiMesh, mat_stat_id);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			//��䶯����
			for (int j = 0; j < paiMesh->mNumVertices; ++j)
			{
				point_need[j].anim_id.x = 1;
				point_need[j].anim_id.y = mesh_vertex_offset + j;
			}
			PancySubModel *new_submodel = new PancySubModel();
			check_error = new_submodel->Create(point_need, index_need, paiMesh->mNumVertices, paiMesh->mNumFaces * 3, material_use);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			model_resource.push_back(new_submodel);
			delete[] point_need;
		}
		else
		{
			PancystarEngine::PointCommon *point_need = new PancystarEngine::PointCommon[paiMesh->mNumVertices];;
			check_error = BuildModelData(point_need, paiMesh, mat_stat_id);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			PancySubModel *new_submodel = new PancySubModel();
			check_error = new_submodel->Create(point_need, index_need, paiMesh->mNumVertices, paiMesh->mNumFaces * 3, material_use);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			model_resource.push_back(new_submodel);
			delete[] point_need;
		}
		delete[] index_need;
		mesh_vertex_offset += paiMesh->mNumVertices;
	}
	if (if_skinmesh)
	{
		//ݲ�������ȷ������λ�õĸ���
		FindRootBone(root_skin);
		aiMatrix4x4 root_mat_identity;
		GetRootSkinOffsetMatrix(model_need->mRootNode, root_mat_identity);
		//����ƫ�ƾ��
		update_mesh_offset(model_need);
		//���ض�����Ϣ
		build_animation_list(model_need, "Pancystar_LocalModel");
		//���ض���Ķ�����Ϣ
		if (CheckIFJson(resource_desc_file))
		{
			pancy_json_value rec_value;
			Json::Value root_value;
			check_error = PancyJsonTool::GetInstance()->LoadJsonFile(resource_desc_file, root_value);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			Json::Value extra_animation_list = root_value.get("Extra_Animation", Json::Value::null);
			if (extra_animation_list != Json::Value::null)
			{
				int32_t num_extra_animation = extra_animation_list.size();
				for (int32_t i = 0; i < num_extra_animation; ++i)
				{
					std::string animation_name;
					std::string animation_file_name;
					//������
					check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, extra_animation_list[i], "animation_name", pancy_json_data_type::json_data_string, rec_value);
					if (!check_error.if_succeed)
					{
						return check_error;
					}
					animation_name = rec_value.string_value;
					//ƶ����ļ���
					check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, extra_animation_list[i], "animation_file", pancy_json_data_type::json_data_string, rec_value);
					if (!check_error.if_succeed)
					{
						return check_error;
					}
					animation_file_name = rec_value.string_value;
					//Ƽ��ض���
					check_error = LoadAnimation(model_root_path + animation_file_name, animation_name);
					if (!check_error.if_succeed)
					{
						return check_error;
					}
				}
			}
		}
		//��ʼ������������Ϣ
		if (skin_animation_map.size() > 0)
		{
			now_animation_use = skin_animation_map.begin()->second;
			if_animation_choose = true;
		}
		now_animation_play_station = 0.0f;
	}
	//ɾ�assimp��ڴ
	importer.FreeScene();
	model_need = NULL;

	//洴����Χ�ж��
	float center_pos_x = (model_size.max_box_pos.x + model_size.min_box_pos.x) / 2.0f;
	float center_pos_y = (model_size.max_box_pos.y + model_size.min_box_pos.y) / 2.0f;
	float center_pos_z = (model_size.max_box_pos.z + model_size.min_box_pos.z) / 2.0f;

	float distance_pos_x = (model_size.max_box_pos.x - model_size.min_box_pos.x) / 2.0f;
	float distance_pos_y = (model_size.max_box_pos.y - model_size.min_box_pos.y) / 2.0f;
	float distance_pos_z = (model_size.max_box_pos.z - model_size.min_box_pos.z) / 2.0f;
	PancystarEngine::PointPositionSingle square_test[] =
	{
		DirectX::XMFLOAT4(-1.0, -1.0, -1.0,1.0),
		DirectX::XMFLOAT4(-1.0, 1.0, -1.0,1.0),
		DirectX::XMFLOAT4(1.0, 1.0, -1.0,1.0),
		DirectX::XMFLOAT4(1.0, -1.0, -1.0,1.0),
		DirectX::XMFLOAT4(-1.0, -1.0, 1.0,1.0),
		DirectX::XMFLOAT4(-1.0, 1.0, 1.0,1.0),
		DirectX::XMFLOAT4(-1.0, 1.0, -1.0,1.0),
		DirectX::XMFLOAT4(-1.0, -1.0, -1.0,1.0),
		DirectX::XMFLOAT4(1.0, -1.0, 1.0,1.0),
		DirectX::XMFLOAT4(1.0, 1.0, 1.0,1.0),
		DirectX::XMFLOAT4(-1.0, 1.0, 1.0,1.0),
		DirectX::XMFLOAT4(-1.0, -1.0, 1.0,1.0),
		DirectX::XMFLOAT4(1.0, -1.0, -1.0,1.0),
		DirectX::XMFLOAT4(1.0, 1.0, -1.0,1.0),
		DirectX::XMFLOAT4(1.0, 1.0, 1.0,1.0),
		DirectX::XMFLOAT4(1.0, -1.0, 1.0,1.0),
		DirectX::XMFLOAT4(-1.0, 1.0, -1.0,1.0),
		DirectX::XMFLOAT4(-1.0, 1.0, 1.0,1.0),
		DirectX::XMFLOAT4(1.0, 1.0, 1.0,1.0),
		DirectX::XMFLOAT4(1.0, 1.0, -1.0,1.0),
		DirectX::XMFLOAT4(-1.0, -1.0, 1.0,1.0),
		DirectX::XMFLOAT4(-1.0, -1.0, -1.0,1.0),
		DirectX::XMFLOAT4(1.0, -1.0, -1.0,1.0),
		DirectX::XMFLOAT4(1.0, -1.0, 1.0,1.0)
	};
	IndexType indices[] = { 0,1,2, 0,2,3, 4,5,6, 4,6,7, 8,9,10, 8,10,11, 12,13,14, 12,14,15, 16,17,18, 16,18,19, 20,21,22, 20,22,23 };
	for (int i = 0; i < 24; ++i)
	{
		square_test[i].position.x = square_test[i].position.x * distance_pos_x + center_pos_x;
		square_test[i].position.y = square_test[i].position.y * distance_pos_y + center_pos_y;
		square_test[i].position.z = square_test[i].position.z * distance_pos_z + center_pos_z;
	}
	model_boundbox = new PancystarEngine::GeometryCommonModel<PancystarEngine::PointPositionSingle>(square_test, indices, 24, 36);
	check_error = model_boundbox->Create();
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	//������ʱ����Ⱦ���

	//򴴽�cbuffer

	std::unordered_map<std::string, std::string> Cbuffer_Heap_desc;
	PancyEffectGraphic::GetInstance()->GetPSO(pso_use)->GetCbufferHeapName(Cbuffer_Heap_desc);
	std::vector<DescriptorTableDesc> descriptor_use_data;
	PancyEffectGraphic::GetInstance()->GetPSO(pso_use)->GetDescriptorHeapUse(descriptor_use_data);

	for (auto cbuffer_data = Cbuffer_Heap_desc.begin(); cbuffer_data != Cbuffer_Heap_desc.end(); ++cbuffer_data)
	{
		SubMemoryPointer cbuffer_use;
		check_error = SubresourceControl::GetInstance()->BuildSubresourceFromFile(cbuffer_data->second, cbuffer_use);
		cbuffer.push_back(cbuffer_use);
	}
	ResourceViewPack globel_var;
	ResourceViewPointer new_res_view;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(descriptor_use_data[0].descriptor_heap_name, globel_var);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	new_res_view.resource_view_pack_id = globel_var;
	new_res_view.resource_view_offset_id = descriptor_use_data[0].table_offset[0];
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(new_res_view, cbuffer[0]);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	table_offset.push_back(new_res_view);
	new_res_view.resource_view_pack_id = globel_var;
	new_res_view.resource_view_offset_id = descriptor_use_data[0].table_offset[1];
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(new_res_view, cbuffer[1]);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	table_offset.push_back(new_res_view);

	//������������
	for (int i = 0; i < texture_use.size(); ++i)
	{
		//����һ�����
		SubMemoryPointer texture_need;
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(texture_use[i], texture_need);
		new_res_view.resource_view_pack_id = globel_var;
		new_res_view.resource_view_offset_id = descriptor_use_data[0].table_offset[2] + i;
		D3D12_SHADER_RESOURCE_VIEW_DESC SRV_desc;
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(texture_use[i], SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_res_view, texture_need, SRV_desc);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		if (i == 0)
		{
			table_offset.push_back(new_res_view);
		}
	}
	//���cbuffer
	/*
	int64_t per_memory_size;
	auto data_submemory = SubresourceControl::GetInstance()->GetResourceData(cbuffer[0], per_memory_size);
	DirectX::XMFLOAT4X4 world_mat[2];
	DirectX::XMStoreFloat4x4(&world_mat[0], DirectX::XMMatrixIdentity());


	DirectX::XMFLOAT3 pos = DirectX::XMFLOAT3(0, 2, -5);
	DirectX::XMFLOAT3 look = DirectX::XMFLOAT3(0, 2, -4);
	DirectX::XMFLOAT3 up = DirectX::XMFLOAT3(0, 1, 0);
	DirectX::XMFLOAT4X4 matrix_view;
	DirectX::XMStoreFloat4x4(&matrix_view, DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&pos), DirectX::XMLoadFloat3(&look), DirectX::XMLoadFloat3(&up)));
	DirectX::XMStoreFloat4x4(&world_mat[1],  DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&pos), DirectX::XMLoadFloat3(&look), DirectX::XMLoadFloat3(&up)) * DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 1000.0f));
	//DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, DirectX::XM_PIDIV4, 0.1f, 1000.0f);
	check_error = data_submemory->WriteFromCpuToBuffer(cbuffer[0].offset* per_memory_size, &world_mat, sizeof(world_mat));
	if (!check_error.if_succeed)
	{
	return check_error;
	}*/
	return PancystarEngine::succeed;
}
void PancyModelAssimp::update(DirectX::XMFLOAT4X4 world_matrix, DirectX::XMFLOAT4X4 uv_matrix, float delta_time)
{

	PancystarEngine::EngineFailReason check_error;

	DirectX::XMFLOAT4X4 view_mat;
	PancyCamera::GetInstance()->CountViewMatrix(&view_mat);
	//���cbuffer
	int64_t per_memory_size;
	auto data_submemory = SubresourceControl::GetInstance()->GetResourceData(cbuffer[0], per_memory_size);
	DirectX::XMFLOAT4X4 world_mat[3];
	DirectX::XMMATRIX proj_mat = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 1000.0f);
	DirectX::XMStoreFloat4x4(&world_mat[0], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&world_matrix)));
	DirectX::XMStoreFloat4x4(&world_mat[1], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&world_matrix) * DirectX::XMLoadFloat4x4(&view_mat) * proj_mat));
	DirectX::XMStoreFloat4x4(&world_mat[2], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&uv_matrix)));
	//DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, DirectX::XM_PIDIV4, 0.1f, 1000.0f);
	check_error = data_submemory->WriteFromCpuToBuffer(cbuffer[0].offset* per_memory_size, &world_mat, sizeof(world_mat));
}
PancystarEngine::EngineFailReason PancyModelAssimp::SaveModelToFile(ID3D11Device* device_pancy, const std::string &out_file_in)
{
	PancystarEngine::EngineFailReason check_error;
	//䴦��洢�ļ���ļ��
	std::string file_root_name = out_file_in.substr(0, out_file_in.size() - 5);
	std::string file_root_real_name;
	int32_t st_pos = 0;
	for (int32_t i = 0; i < file_root_name.size(); ++i)
	{
		if (file_root_name[i] == '\\' || file_root_name[i] == '/')
		{
			st_pos = i + 1;
		}
	}
	if (st_pos < file_root_name.size())
	{
		file_root_real_name = file_root_name.substr(st_pos, file_root_name.size() - st_pos);
	}
	else
	{
		file_root_real_name = "";
	}
	//�json�ļ
	Json::Value json_data_outmodel;
	PancyJsonTool::GetInstance()->SetJsonValue(json_data_outmodel, "IfHaveSkinAnimation", if_skinmesh);
	PancyJsonTool::GetInstance()->SetJsonValue(json_data_outmodel, "IfHavePoinAnimation", if_pointmesh);
	PancyJsonTool::GetInstance()->SetJsonValue(json_data_outmodel, "PbrType", PancyJsonTool::GetInstance()->GetEnumName(typeid(model_pbr_type).name(), model_pbr_type));
	PancyJsonTool::GetInstance()->SetJsonValue(json_data_outmodel, "model_num", model_lod_divide.size());
	PancyJsonTool::GetInstance()->SetJsonValue(json_data_outmodel, "material_num", material_list.size());
	PancyJsonTool::GetInstance()->SetJsonValue(json_data_outmodel, "texture_num", texture_list.size());
	for (int i = 0; i < texture_list.size(); ++i)
	{
		//�洢ÿһ��������
		std::string now_tex_file_name = file_root_name + "_tex" + std::to_string(i) + ".dds";
		std::string now_tex_file_real_name = file_root_real_name + "_tex" + std::to_string(i) + ".dds";
		PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_outmodel, "texture_file", now_tex_file_real_name);
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->SaveTextureToFile(device_pancy, texture_list[i], now_tex_file_name, true, true);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
	}
	//���䶯����Ϣ
	if (if_skinmesh)
	{
		//�洢������
		out_stream.open(file_root_name + ".bone", ios::binary);
		SaveBoneTree(root_skin);
		out_stream.close();
		//ݴ洢������
		for (auto animation_deal = skin_animation_map.begin(); animation_deal != skin_animation_map.end(); ++animation_deal)
		{
			//ݽ�������Ϣд�json
			std::string animation_real_name_now = file_root_real_name + "_anim_" + animation_deal->first + ".skinanim";
			PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_outmodel, "SkinAnimation", animation_real_name_now);
			//뽫����ϸ�ڵ��뵽�ļ
			std::string animation_name_now = file_root_name + "_anim_" + animation_deal->first + ".skinanim";
			out_stream.open(animation_name_now, ios::binary);
			//�һ�����ÿ�����ı任��Ϣд���ļ
			for (int i = 0; i < animation_deal->second.data_animition.size(); ++i)
			{
				//�ǰ�任�Ĺ�����Ƶĳ��
				auto bone_name_size = animation_deal->second.data_animition[i].bone_name.size();
				out_stream.write(reinterpret_cast<char*>(&bone_name_size), sizeof(bone_name_size));
				//ȵ�ǰ�任�Ĺ�����
				out_stream.write(animation_deal->second.data_animition[i].bone_name.c_str(), animation_deal->second.data_animition[i].bone_name.size() * sizeof(char));
				//�������ת��
				auto rottation_key_size = animation_deal->second.data_animition[i].rotation_key.size();
				out_stream.write(reinterpret_cast<char*>(&rottation_key_size), sizeof(rottation_key_size));
				out_stream.write(reinterpret_cast<char*>(&animation_deal->second.data_animition[i].rotation_key[0]), animation_deal->second.data_animition[i].rotation_key.size() * sizeof(animation_deal->second.data_animition[i].rotation_key[0]));
				//�����ƽ����
				auto translation_key_size = animation_deal->second.data_animition[i].translation_key.size();
				out_stream.write(reinterpret_cast<char*>(&translation_key_size), sizeof(translation_key_size));
				out_stream.write(reinterpret_cast<char*>(&animation_deal->second.data_animition[i].translation_key[0]), animation_deal->second.data_animition[i].translation_key.size() * sizeof(animation_deal->second.data_animition[i].translation_key[0]));
				//����������
				auto scaling_key_size = animation_deal->second.data_animition[i].scaling_key.size();
				out_stream.write(reinterpret_cast<char*>(&scaling_key_size), sizeof(scaling_key_size));
				out_stream.write(reinterpret_cast<char*>(&animation_deal->second.data_animition[i].scaling_key[0]), animation_deal->second.data_animition[i].scaling_key.size() * sizeof(animation_deal->second.data_animition[i].scaling_key[0]));
			}
			out_stream.close();
		}
	}
	else if (if_pointmesh)
	{
		//ݴ洢���㶯����
		int point_number = FBXanim_import->GetMeshAnimNumber();
		mesh_animation_data *new_data = new mesh_animation_data[point_number];
		FBXanim_import->GetMeshAnimData(new_data);
		//ݽ�������Ϣд�json
		std::string animation_real_name_now = file_root_real_name + "_anim_" + "basic" + ".pointcatch";
		PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_outmodel, "PointAnimation", animation_real_name_now);
		//뽫����ϸ�ڵ��뵽�ļ
		std::string animation_name_now = file_root_name + "_anim_" + "basic" + ".pointcatch";
		out_stream.open(animation_name_now, ios::binary);
		auto all_frame_num = FBXanim_import->get_frame_num();
		auto perframe_size = FBXanim_import->GetMeshSizePerFrame();
		out_stream.write(reinterpret_cast<char*>(&all_frame_num), sizeof(all_frame_num));
		out_stream.write(reinterpret_cast<char*>(&perframe_size), sizeof(perframe_size));
		out_stream.write(reinterpret_cast<char*>(&point_number), sizeof(point_number));
		int32_t size_need = point_number * sizeof(mesh_animation_data);
		out_stream.write(reinterpret_cast<char*>(new_data), size_need);
		
		out_stream.close();
		delete[] new_data;
	}
	//���϶�����
	for (int i = 0; i < model_lod_divide.size(); ++i)
	{
		if (if_skinmesh)
		{
			//���䶥�㼰���
			std::vector<PancystarEngine::PointSkinCommon8> vertex_data_pack;
			std::vector<IndexType> index_data_pack;
			for (int j = 0; j < model_lod_divide[i].size(); ++j)
			{
				std::vector<PancystarEngine::PointSkinCommon8> vertex_data_in;
				std::vector<IndexType> index_data_in;
				model_resource_list[model_lod_divide[i][j]]->GetSubModelData(vertex_data_in, index_data_in);
				for (int k = 0; k < index_data_in.size(); ++k)
				{
					//��������
					index_data_pack.push_back(index_data_in[i] + vertex_data_pack.size());
				}
				for (int k = 0; k < vertex_data_in.size(); ++k)
				{
					//���Ӷ�����Ϣ
					vertex_data_pack.push_back(vertex_data_in[i]);
				}
			}
			//�洢���㼰���
			out_stream.open(file_root_name + std::to_string(i) + ".vertex", ios::binary);
			out_stream.write(reinterpret_cast<char*>(&vertex_data_pack[0]), vertex_data_pack.size() * sizeof(vertex_data_pack[0]));
			out_stream.close();
			out_stream.open(file_root_name + std::to_string(i) + ".index", ios::binary);
			out_stream.write(reinterpret_cast<char*>(&index_data_pack[0]), index_data_pack.size() * sizeof(index_data_pack[0]));
			out_stream.close();
		}
		else if (if_pointmesh)
		{
			//���䶥�㼰���
			std::vector<PancystarEngine::PointCatchCommon> vertex_data_pack;
			std::vector<IndexType> index_data_pack;
			for (int j = 0; j < model_lod_divide[i].size(); ++j)
			{
				std::vector<PancystarEngine::PointCatchCommon> vertex_data_in;
				std::vector<IndexType> index_data_in;
				model_resource_list[model_lod_divide[i][j]]->GetSubModelData(vertex_data_in, index_data_in);
				for (int k = 0; k < index_data_in.size(); ++k)
				{
					//��������
					index_data_pack.push_back(index_data_in[i] + vertex_data_pack.size());
				}
				for (int k = 0; k < vertex_data_in.size(); ++k)
				{
					//���Ӷ�����Ϣ
					vertex_data_pack.push_back(vertex_data_in[i]);
				}
			}
			//�洢���㼰���
			out_stream.open(file_root_name + std::to_string(i) + ".vertex", ios::binary);
			out_stream.write(reinterpret_cast<char*>(&vertex_data_pack[0]), vertex_data_pack.size() * sizeof(vertex_data_pack[0]));
			out_stream.close();
			out_stream.open(file_root_name + std::to_string(i) + ".index", ios::binary);
			out_stream.write(reinterpret_cast<char*>(&index_data_pack[0]), index_data_pack.size() * sizeof(index_data_pack[0]));
			out_stream.close();
		}
		else
		{
			std::vector<PancystarEngine::PointCommon> vertex_data_pack;
			std::vector<IndexType> index_data_pack;
			//���䶥�㼰���
			for (int j = 0; j < model_lod_divide[i].size(); ++j)
			{
				std::vector<PancystarEngine::PointCommon> vertex_data_in;
				std::vector<IndexType> index_data_in;
				model_resource_list[model_lod_divide[i][j]]->GetSubModelData(vertex_data_in, index_data_in);
				for (int k = 0; k < index_data_in.size(); ++k)
				{
					//��������
					index_data_pack.push_back(index_data_in[i] + vertex_data_pack.size());
				}
				for (int k = 0; k < vertex_data_in.size(); ++k)
				{
					//���Ӷ�����Ϣ
					vertex_data_pack.push_back(vertex_data_in[i]);
				}
			}
			//�洢���㼰���
			out_stream.open(file_root_name + std::to_string(i) + ".vertex", ios::binary);
			out_stream.write(reinterpret_cast<char*>(&vertex_data_pack[0]), vertex_data_pack.size() * sizeof(vertex_data_pack[0]));
			out_stream.close();
			out_stream.open(file_root_name + std::to_string(i) + ".index", ios::binary);
			out_stream.write(reinterpret_cast<char*>(&index_data_pack[0]), index_data_pack.size() * sizeof(index_data_pack[0]));
			out_stream.close();
		}

	}
	//��json��ļ�д�������Ϣ

	for (auto material_data_deal = material_list.begin(); material_data_deal != material_list.end(); ++material_data_deal)
	{
		Json::Value json_data_material;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_material, "materialID", material_data_deal->first);
		if (material_data_deal->second.find(TexType::tex_diffuse) != material_data_deal->second.end())
		{
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_material, "Albedotex", material_data_deal->second[TexType::tex_diffuse]);
		}
		else
		{
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(E_FAIL, "could not find the albedo tex to export",error_message);
			
			return error_message;
		}
		if (material_data_deal->second.find(TexType::tex_normal) != material_data_deal->second.end())
		{
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_material, "Normaltex", material_data_deal->second[TexType::tex_normal]);
		}
		else
		{
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(E_FAIL, "could not find the normal tex to export",error_message);
			
			return error_message;
		}
		if (material_data_deal->second.find(TexType::tex_ambient) != material_data_deal->second.end())
		{
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_material, "Ambienttex", material_data_deal->second[TexType::tex_ambient]);
		}
		else
		{
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(E_FAIL, "could not find the ambient tex to export",error_message);
			
			return error_message;
		}
		if (model_pbr_type == PbrMaterialType::PbrType_MetallicRoughness)
		{
			if (material_data_deal->second.find(TexType::tex_metallic) != material_data_deal->second.end())
			{
				PancyJsonTool::GetInstance()->SetJsonValue(json_data_material, "MetallicTex", material_data_deal->second[TexType::tex_metallic]);
			}
			else
			{
				PancystarEngine::EngineFailReason error_message;
				PancyDebugLogError(E_FAIL, "could not find the metallic tex to export",error_message);
				
				return error_message;
			}
			if (material_data_deal->second.find(TexType::tex_roughness) != material_data_deal->second.end())
			{
				PancyJsonTool::GetInstance()->SetJsonValue(json_data_material, "RoughnessTex", material_data_deal->second[TexType::tex_roughness]);
			}
			else
			{
				PancystarEngine::EngineFailReason error_message;
				PancyDebugLogError(E_FAIL, "could not find the roughness tex to export",error_message);
				
				return error_message;
			}
		}
		else if (model_pbr_type == PbrMaterialType::PbrType_SpecularSmoothness) 
		{
			if (material_data_deal->second.find(TexType::tex_specular_smoothness) != material_data_deal->second.end())
			{
				PancyJsonTool::GetInstance()->SetJsonValue(json_data_material, "SpecularSmoothTex", material_data_deal->second[TexType::tex_specular_smoothness]);
			}
			else
			{
				PancystarEngine::EngineFailReason error_message;
				PancyDebugLogError(E_FAIL, "could not find the specularsmooth tex to export",error_message);
				
				return error_message;
			}
		}
		PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_outmodel, "material", json_data_material);
	}
	check_error = PancyJsonTool::GetInstance()->WriteValueToJson(json_data_outmodel, out_file_in);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
//���㶯��
PancystarEngine::EngineFailReason PancyModelAssimp::BuildDefaultBuffer(
	PancyNowGraphicsCommandList* cmdList,
	int64_t memory_alignment_size,
	int64_t memory_block_alignment_size,
	SubMemoryPointer &default_buffer,
	SubMemoryPointer &upload_buffer,
	const void* initData,
	const UINT BufferSize,
	D3D12_RESOURCE_STATES buffer_type
)
{
	HRESULT hr;
	PancystarEngine::EngineFailReason check_error;
	//�ȴ���4M����Ĵ洢�
	UINT alignment_buffer_size = (BufferSize + memory_alignment_size) & ~(memory_alignment_size - 1);//4M���
	std::string heapdesc_file_name = "json\\resource_heap\\PointBuffer" + std::to_string(alignment_buffer_size) + ".json";
	std::string dynamic_heapdesc_file_name = "json\\resource_heap\\DynamicBuffer" + std::to_string(alignment_buffer_size) + ".json";
	//������Դ�Ѹ�ʽ����
	if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(heapdesc_file_name))
	{
		//���¸�ʽ�ļ
		Json::Value json_data_out;
		UINT resource_block_num = (4194304 * 20) / alignment_buffer_size;
		if (resource_block_num < 1)
		{
			resource_block_num = 1;
		}
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "commit_block_num", resource_block_num);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "per_block_size", alignment_buffer_size);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "heap_type_in", "D3D12_HEAP_TYPE_DEFAULT");
		PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_out, "heap_flag_in", "D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS");
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, heapdesc_file_name);
		//��ļ���Ϊ�Ѿ�����
		PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(heapdesc_file_name);
	}
	//�����ϴ�����ʱ��Դ�Ѹ�ʽ����
	if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(dynamic_heapdesc_file_name))
	{
		//���¸�ʽ�ļ
		Json::Value json_data_out;
		UINT resource_block_num = (4194304 * 5) / alignment_buffer_size;
		if (resource_block_num < 1)
		{
			resource_block_num = 1;
		}
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "commit_block_num", resource_block_num);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "per_block_size", alignment_buffer_size);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "heap_type_in", "D3D12_HEAP_TYPE_UPLOAD");
		PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_out, "heap_flag_in", "D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS");
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, dynamic_heapdesc_file_name);
		//��ļ���Ϊ�Ѿ�����
		PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(dynamic_heapdesc_file_name);
	}
	//�����洢��Ԫ�ķ����С
	UINT buffer_block_size = (BufferSize + memory_block_alignment_size) & ~(memory_block_alignment_size - 1);//128k���
	if (alignment_buffer_size % buffer_block_size != 0)
	{
		int32_t max_divide_size = alignment_buffer_size / buffer_block_size;
		for (int32_t i = max_divide_size; i >= 0; --i)
		{
			if (alignment_buffer_size % i == 0)
			{
				buffer_block_size = alignment_buffer_size / i;
				break;
			}
		}
	}
	std::string bufferblock_file_name = "json\\resource_view\\PointBufferSub" + std::to_string(buffer_block_size) + ".json";
	std::string dynamic_bufferblock_file_name = "json\\resource_view\\DynamicBufferSub" + std::to_string(buffer_block_size) + ".json";
	//������Դ�Ѵ洢��Ԫ��ʽ����
	if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(bufferblock_file_name))
	{
		//���¸�ʽ�ļ
		Json::Value json_data_resourceview;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "ResourceType", heapdesc_file_name);
		Json::Value json_data_res_desc;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Dimension", "D3D12_RESOURCE_DIMENSION_BUFFER");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Alignment", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Width", alignment_buffer_size);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Height", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "DepthOrArraySize", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "MipLevels", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Format", "DXGI_FORMAT_UNKNOWN");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Layout", "D3D12_TEXTURE_LAYOUT_ROW_MAJOR");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Flags", "D3D12_RESOURCE_FLAG_NONE");
		Json::Value json_data_sample_desc;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_sample_desc, "Count", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_sample_desc, "Quality", 0);
		//�ݹ�ص
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "SampleDesc", json_data_sample_desc);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_DESC", json_data_res_desc);
		//���������
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_STATES", "D3D12_RESOURCE_STATE_COPY_DEST");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "per_block_size", buffer_block_size);
		//�д���ļ���Ϊ�Ѵ���
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_resourceview, bufferblock_file_name);
		PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(bufferblock_file_name);
	}
	//�����ϴ�����ʱ��Դ�Ѵ洢��Ԫ��ʽ����
	if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(dynamic_bufferblock_file_name))
	{
		//���¸�ʽ�ļ
		Json::Value json_data_resourceview;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "ResourceType", dynamic_heapdesc_file_name);
		Json::Value json_data_res_desc;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Dimension", "D3D12_RESOURCE_DIMENSION_BUFFER");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Alignment", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Width", alignment_buffer_size);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Height", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "DepthOrArraySize", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "MipLevels", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Format", "DXGI_FORMAT_UNKNOWN");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Layout", "D3D12_TEXTURE_LAYOUT_ROW_MAJOR");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Flags", "D3D12_RESOURCE_FLAG_NONE");
		Json::Value json_data_sample_desc;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_sample_desc, "Count", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_sample_desc, "Quality", 0);
		//�ݹ�ص
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "SampleDesc", json_data_sample_desc);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_DESC", json_data_res_desc);
		//���������
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_STATES", "D3D12_RESOURCE_STATE_GENERIC_READ");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "per_block_size", buffer_block_size);
		//�д���ļ���Ϊ�Ѵ���
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_resourceview, dynamic_bufferblock_file_name);
		PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(dynamic_bufferblock_file_name);
	}
	check_error = SubresourceControl::GetInstance()->BuildSubresourceFromFile(bufferblock_file_name, default_buffer);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	check_error = SubresourceControl::GetInstance()->BuildSubresourceFromFile(dynamic_bufferblock_file_name, upload_buffer);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	//�CPU�ɷ��ʻ����������
	D3D12_SUBRESOURCE_DATA vertexData_buffer = {};
	vertexData_buffer.pData = initData;
	vertexData_buffer.RowPitch = BufferSize;
	vertexData_buffer.SlicePitch = vertexData_buffer.RowPitch;
	int64_t per_memory_size;
	auto dest_res = SubresourceControl::GetInstance()->GetResourceData(default_buffer, per_memory_size);
	auto copy_res = SubresourceControl::GetInstance()->GetResourceData(upload_buffer, per_memory_size);
	//��Ƚ�cpu�ϵ���ݿ������ϴ������
	check_error = copy_res->WriteFromCpuToBuffer(upload_buffer.offset*per_memory_size, initData, BufferSize);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	//��ϴ����������ݿ������Դ滺���
	cmdList->CopyBufferRegion(dest_res->GetResource().Get(), default_buffer.offset * per_memory_size, copy_res->GetResource().Get(), upload_buffer.offset*per_memory_size, buffer_block_size);
	//��޸Ļ������ʽ
	cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			dest_res->GetResource().Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			buffer_type
		)
	);
	return PancystarEngine::succeed;
}
//�������
bool PancyModelAssimp::check_ifsame(char a[], char b[])
{
	int length = strlen(a);
	if (strlen(a) != strlen(b))
	{
		return false;
	}
	for (int i = 0; i < length; ++i)
	{
		if (a[i] != b[i])
		{
			return false;
		}
	}
	return true;
}
skin_tree* PancyModelAssimp::find_tree(skin_tree* p, char name[])
{
	if (check_ifsame(p->bone_ID, name))
	{
		return p;
	}
	else
	{
		skin_tree* q;
		if (p->brother != NULL)
		{
			q = find_tree(p->brother, name);
			if (q != NULL)
			{
				return q;
			}
		}
		if (p->son != NULL)
		{
			q = find_tree(p->son, name);
			if (q != NULL)
			{
				return q;
			}
		}
	}
	return NULL;
}
skin_tree* PancyModelAssimp::find_tree(skin_tree* p, int num)
{
	if (p->bone_number == num)
	{
		return p;
	}
	else
	{
		skin_tree* q;
		if (p->brother != NULL)
		{
			q = find_tree(p->brother, num);
			if (q != NULL)
			{
				return q;
			}
		}
		if (p->son != NULL)
		{
			q = find_tree(p->son, num);
			if (q != NULL)
			{
				return q;
			}
		}
	}
	return NULL;
}
PancystarEngine::EngineFailReason PancyModelAssimp::build_skintree(aiNode *now_node, skin_tree *now_root)
{
	if (now_node != NULL)
	{
		strcpy(now_root->bone_ID, now_node->mName.data);
		set_matrix(now_root->animation_matrix, &now_node->mTransformation);
		now_root->bone_number = NouseAssimpStruct;
		if (now_node->mNumChildren > 0)
		{
			//���������һ�����������ӽ�
			now_root->son = new skin_tree();
			build_skintree(now_node->mChildren[0], now_root->son);
		}
		skin_tree *p = now_root->son;
		for (int i = 1; i < now_node->mNumChildren; ++i)
		{
			//㽨����е��ֵ��
			p->brother = new skin_tree();
			build_skintree(now_node->mChildren[i], p->brother);
			p = p->brother;
		}
	}
	return PancystarEngine::succeed;
}
void PancyModelAssimp::FindRootBone(skin_tree *now_bone)
{
	if (now_bone->bone_number == NouseAssimpStruct && now_bone->son != NULL && now_bone->son->bone_number != NouseAssimpStruct)
	{
		model_move_skin = now_bone;
	}
	else
	{
		if (now_bone->brother != NULL)
		{
			FindRootBone(now_bone->brother);
		}
		if (now_bone->son != NULL)
		{
			FindRootBone(now_bone->son);
		}
	}
}
void PancyModelAssimp::set_matrix(DirectX::XMFLOAT4X4 &out, aiMatrix4x4 *in)
{
	out._11 = in->a1;
	out._21 = in->a2;
	out._31 = in->a3;
	out._41 = in->a4;

	out._12 = in->b1;
	out._22 = in->b2;
	out._32 = in->b3;
	out._42 = in->b4;

	out._13 = in->c1;
	out._23 = in->c2;
	out._33 = in->c3;
	out._43 = in->c4;

	out._14 = in->d1;
	out._24 = in->d2;
	out._34 = in->d3;
	out._44 = in->d4;
}
void PancyModelAssimp::check_son_num(skin_tree *input, int &count)
{
	if (input != NULL)
	{
		count += 1;
		if (input->brother != NULL)
		{
			check_son_num(input->brother, count);
		}
		if (input->son != NULL)
		{
			check_son_num(input->son, count);
		}
	}
}
void PancyModelAssimp::update_mesh_offset(const aiScene *model_need)
{
	for (int i = 0; i < model_need->mNumMeshes; ++i)
	{
		for (int j = 0; j < model_need->mMeshes[i]->mNumBones; ++j)
		{
			set_matrix(offset_matrix_array[tree_node_num[i][j]], &model_need->mMeshes[i]->mBones[j]->mOffsetMatrix);
		}
	}
}
PancystarEngine::EngineFailReason PancyModelAssimp::build_animation_list(const aiScene *model_need, const std::string animation_name_in)
{
	for (int i = 0; i < model_need->mNumAnimations; ++i)
	{
		std::string now_animation_name = animation_name_in;
		if (now_animation_name == "")
		{
			now_animation_name = model_need->mAnimations[i]->mName.data;
		}
		else
		{
			now_animation_name += "_";
			now_animation_name += model_need->mAnimations[i]->mName.data;
		}
		animation_set now_anim_set;
		now_anim_set.animation_length = model_need->mAnimations[i]->mDuration;
		int32_t number_used_bones = model_need->mAnimations[i]->mNumChannels;
		for (int j = 0; j < number_used_bones; ++j)
		{
			animation_data now_bone_anim_data;
			now_bone_anim_data.bone_name = model_need->mAnimations[i]->mChannels[j]->mNodeName.data;
			now_bone_anim_data.bone_point = find_tree(root_skin, model_need->mAnimations[i]->mChannels[j]->mNodeName.data);
			if (now_bone_anim_data.bone_point == NULL)
			{
				//δ���ֹ��������
				PancystarEngine::EngineFailReason error_message;
				PancyDebugLogError(E_FAIL, "Could not find the bone :" + now_bone_anim_data.bone_name + " from model:" + resource_name, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING,error_message);
				
				continue;
			}
			//���ת��Ԫ�
			int32_t number_rotation = model_need->mAnimations[i]->mChannels[j]->mNumRotationKeys;
			for (int k = 0; k < number_rotation; ++k)
			{
				quaternion_animation new_animation_rotate;
				new_animation_rotate.time = model_need->mAnimations[i]->mChannels[j]->mRotationKeys[k].mTime;
				new_animation_rotate.main_key[0] = model_need->mAnimations[i]->mChannels[j]->mRotationKeys[k].mValue.x;
				new_animation_rotate.main_key[1] = model_need->mAnimations[i]->mChannels[j]->mRotationKeys[k].mValue.y;
				new_animation_rotate.main_key[2] = model_need->mAnimations[i]->mChannels[j]->mRotationKeys[k].mValue.z;
				new_animation_rotate.main_key[3] = model_need->mAnimations[i]->mChannels[j]->mRotationKeys[k].mValue.w;
				now_bone_anim_data.rotation_key.push_back(new_animation_rotate);
			}
			//�ƽ�����
			int32_t number_translation = model_need->mAnimations[i]->mChannels[j]->mNumPositionKeys;
			for (int k = 0; k < number_translation; ++k)
			{
				vector_animation new_animation_translate;
				new_animation_translate.time = model_need->mAnimations[i]->mChannels[j]->mPositionKeys[k].mTime;
				new_animation_translate.main_key[0] = model_need->mAnimations[i]->mChannels[j]->mPositionKeys[k].mValue.x;
				new_animation_translate.main_key[1] = model_need->mAnimations[i]->mChannels[j]->mPositionKeys[k].mValue.y;
				new_animation_translate.main_key[2] = model_need->mAnimations[i]->mChannels[j]->mPositionKeys[k].mValue.z;
				now_bone_anim_data.translation_key.push_back(new_animation_translate);
			}
			//������
			int32_t number_scaling = model_need->mAnimations[i]->mChannels[j]->mNumScalingKeys;
			for (int k = 0; k < number_scaling; ++k)
			{
				vector_animation new_animation_scaling;
				new_animation_scaling.time = model_need->mAnimations[i]->mChannels[j]->mScalingKeys[k].mTime;
				new_animation_scaling.main_key[0] = model_need->mAnimations[i]->mChannels[j]->mScalingKeys[k].mValue.x;
				new_animation_scaling.main_key[1] = model_need->mAnimations[i]->mChannels[j]->mScalingKeys[k].mValue.y;
				new_animation_scaling.main_key[2] = model_need->mAnimations[i]->mChannels[j]->mScalingKeys[k].mValue.z;
				now_bone_anim_data.scaling_key.push_back(new_animation_scaling);
			}
			//���ù���Ķ���������ܶ���
			now_anim_set.data_animition.push_back(now_bone_anim_data);
		}
		if (skin_animation_map.find(now_animation_name) != skin_animation_map.end())
		{
			//�����ظ����أ�����
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(E_FAIL, "repeat load the animation :" + now_animation_name + " from model:" + resource_name, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING,error_message);
			
			continue;
		}
		else
		{
			skin_animation_map.insert(std::pair<string, animation_set>(now_animation_name, now_anim_set));
		}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyModelAssimp::LoadAnimation(const std::string &resource_desc_file, const std::string &animation_name)
{
	PancystarEngine::EngineFailReason check_error;
	//���ʼ����ģ�
	const aiScene *model_need = NULL;//assimp�ģ�ͱ��
	model_need = importer.ReadFile(resource_desc_file,
		aiProcess_MakeLeftHanded |
		aiProcess_FlipWindingOrder |
		aiProcess_CalcTangentSpace |
		aiProcess_JoinIdenticalVertices
	);//ݽ���ͬͼԪ���õ���ͬ��ģ����ȥ��ͼƬ���Ϳ����ǵ㡢ֱ�ߡ�����ε
	if (model_need == NULL)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "read model" + resource_desc_file + "error",error_message);
		
		return error_message;
	}
	//ȼ��ض�����
	check_error = build_animation_list(model_need, animation_name);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	importer.FreeScene();
	model_need = NULL;
	return PancystarEngine::succeed;
}
void PancyModelAssimp::GetAnimationNameList(std::vector<std::string> &animation_name)
{
	animation_name.clear();
	for (auto animation_data = skin_animation_map.begin(); animation_data != skin_animation_map.end(); ++animation_data)
	{
		animation_name.push_back(animation_data->first);
	}
}
void PancyModelAssimp::update_root(skin_tree *root, DirectX::XMFLOAT4X4 matrix_parent)
{
	if (root == NULL)
	{
		return;
	}
	DirectX::XMMATRIX rec = DirectX::XMLoadFloat4x4(&root->animation_matrix);
	DirectX::XMStoreFloat4x4(&root->now_matrix, rec * DirectX::XMLoadFloat4x4(&matrix_parent));
	if (root->bone_number >= 0)
	{
		bone_matrix_array[root->bone_number] = root->now_matrix;
	}
	update_root(root->brother, matrix_parent);
	update_root(root->son, root->now_matrix);
}
void PancyModelAssimp::GetModelBoneData(DirectX::XMFLOAT4X4 *bone_matrix)
{
	//ݸ��¶���
	update_anim_data();
	//ˢ�¹�������
	DirectX::XMFLOAT4X4 matrix_identi;
	DirectX::XMStoreFloat4x4(&matrix_identi, DirectX::XMMatrixIdentity());
	update_root(root_skin, matrix_identi);
	//���º�Ķ���������ƫ�
	for (int i = 0; i < MaxBoneNum; ++i)
	{
		DirectX::XMStoreFloat4x4(&final_matrix_array[i], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&offset_matrix_array[i]) * DirectX::XMLoadFloat4x4(&bone_matrix_array[i])));
		bone_matrix[i] = final_matrix_array[i];
	}
}
void PancyModelAssimp::update_anim_data()
{
	float input_time = now_animation_play_station * now_animation_use.animation_length;
	for (int i = 0; i < now_animation_use.data_animition.size(); ++i)
	{
		animation_data now = now_animation_use.data_animition[i];
		DirectX::XMMATRIX rec_trans, rec_scal;
		DirectX::XMFLOAT4X4 rec_rot;

		int start_anim, end_anim;
		find_anim_sted(input_time, start_anim, end_anim, now.rotation_key);
		//���Ԫ���ֵ��Ѱ�ұ任���
		quaternion_animation rotation_now;
		if (start_anim == end_anim || end_anim >= now.rotation_key.size())
		{
			rotation_now = now.rotation_key[start_anim];
		}
		else
		{
			Interpolate(rotation_now, now.rotation_key[start_anim], now.rotation_key[end_anim], (input_time - now.rotation_key[start_anim].time) / (now.rotation_key[end_anim].time - now.rotation_key[start_anim].time));
		}
		Get_quatMatrix(rec_rot, rotation_now);
		//���ű任
		find_anim_sted(input_time, start_anim, end_anim, now.scaling_key);
		vector_animation scalling_now;
		if (start_anim == end_anim)
		{
			scalling_now = now.scaling_key[start_anim];
		}
		else
		{
			Interpolate(scalling_now, now.scaling_key[start_anim], now.scaling_key[end_anim], (input_time - now.scaling_key[start_anim].time) / (now.scaling_key[end_anim].time - now.scaling_key[start_anim].time));
		}
		rec_scal = DirectX::XMMatrixScaling(scalling_now.main_key[0], scalling_now.main_key[1], scalling_now.main_key[2]);
		//ƽ�Ʊ任
		find_anim_sted(input_time, start_anim, end_anim, now.translation_key);
		vector_animation translation_now;
		if (start_anim == end_anim)
		{
			translation_now = now.translation_key[start_anim];
		}
		else
		{
			Interpolate(translation_now, now.translation_key[start_anim], now.translation_key[end_anim], (input_time - now.translation_key[start_anim].time) / (now.translation_key[end_anim].time - now.translation_key[start_anim].time));
		}
		rec_trans = DirectX::XMMatrixTranslation(translation_now.main_key[0], translation_now.main_key[1], translation_now.main_key[2]);
		//�ܱ任
		XMStoreFloat4x4(&now.bone_point->animation_matrix, rec_scal * XMLoadFloat4x4(&rec_rot) * rec_trans);
	}
}
void PancyModelAssimp::Interpolate(quaternion_animation& pOut, const quaternion_animation &pStart, const quaternion_animation &pEnd, const float &pFactor)
{
	float cosom = pStart.main_key[0] * pEnd.main_key[0] + pStart.main_key[1] * pEnd.main_key[1] + pStart.main_key[2] * pEnd.main_key[2] + pStart.main_key[3] * pEnd.main_key[3];
	quaternion_animation end = pEnd;
	if (cosom < static_cast<float>(0.0))
	{
		cosom = -cosom;
		end.main_key[0] = -end.main_key[0];
		end.main_key[1] = -end.main_key[1];
		end.main_key[2] = -end.main_key[2];
		end.main_key[3] = -end.main_key[3];
	}
	float sclp, sclq;
	if ((static_cast<float>(1.0) - cosom) > static_cast<float>(0.0001))
	{
		float omega, sinom;
		omega = acos(cosom);
		sinom = sin(omega);
		sclp = sin((static_cast<float>(1.0) - pFactor) * omega) / sinom;
		sclq = sin(pFactor * omega) / sinom;
	}
	else
	{
		sclp = static_cast<float>(1.0) - pFactor;
		sclq = pFactor;
	}

	pOut.main_key[0] = sclp * pStart.main_key[0] + sclq * end.main_key[0];
	pOut.main_key[1] = sclp * pStart.main_key[1] + sclq * end.main_key[1];
	pOut.main_key[2] = sclp * pStart.main_key[2] + sclq * end.main_key[2];
	pOut.main_key[3] = sclp * pStart.main_key[3] + sclq * end.main_key[3];
}
void PancyModelAssimp::Interpolate(vector_animation& pOut, const vector_animation &pStart, const vector_animation &pEnd, const float &pFactor)
{
	for (int i = 0; i < 3; ++i)
	{
		pOut.main_key[i] = pStart.main_key[i] + pFactor * (pEnd.main_key[i] - pStart.main_key[i]);
	}
}
void PancyModelAssimp::find_anim_sted(const float &input_time, int &st, int &ed, const std::vector<quaternion_animation> &input)
{
	if (input_time < 0)
	{
		st = 0;
		ed = 0;
		return;
	}
	if (input_time > input[input.size() - 1].time)
	{
		st = input.size() - 1;
		ed = input.size() - 1;
		return;
	}
	for (int i = 0; i < input.size() - 1; ++i)
	{
		if (input_time >= input[i].time && input_time <= input[i + 1].time)
		{
			st = i;
			ed = i + 1;
			return;
		}
	}
	st = input.size() - 1;
	ed = input.size() - 1;
}
void PancyModelAssimp::find_anim_sted(const float &input_time, int &st, int &ed, const std::vector<vector_animation> &input)
{
	if (input_time < 0)
	{
		st = 0;
		ed = 0;
		return;
	}
	if (input_time > input[input.size() - 1].time)
	{
		st = input.size() - 1;
		ed = input.size() - 1;
		return;
	}
	for (int i = 0; i < input.size() - 1; ++i)
	{
		if (input_time >= input[i].time && input_time <= input[i + 1].time)
		{
			st = i;
			ed = i + 1;
			return;
		}
	}
	st = input.size() - 1;
	ed = input.size() - 1;
}
void PancyModelAssimp::Get_quatMatrix(DirectX::XMFLOAT4X4 &resMatrix, const quaternion_animation& pOut)
{
	resMatrix._11 = static_cast<float>(1.0) - static_cast<float>(2.0) * (pOut.main_key[1] * pOut.main_key[1] + pOut.main_key[2] * pOut.main_key[2]);
	resMatrix._21 = static_cast<float>(2.0) * (pOut.main_key[0] * pOut.main_key[1] - pOut.main_key[2] * pOut.main_key[3]);
	resMatrix._31 = static_cast<float>(2.0) * (pOut.main_key[0] * pOut.main_key[2] + pOut.main_key[1] * pOut.main_key[3]);
	resMatrix._41 = 0.0f;

	resMatrix._12 = static_cast<float>(2.0) * (pOut.main_key[0] * pOut.main_key[1] + pOut.main_key[2] * pOut.main_key[3]);
	resMatrix._22 = static_cast<float>(1.0) - static_cast<float>(2.0) * (pOut.main_key[0] * pOut.main_key[0] + pOut.main_key[2] * pOut.main_key[2]);
	resMatrix._32 = static_cast<float>(2.0) * (pOut.main_key[1] * pOut.main_key[2] - pOut.main_key[0] * pOut.main_key[3]);
	resMatrix._42 = 0.0f;

	resMatrix._13 = static_cast<float>(2.0) * (pOut.main_key[0] * pOut.main_key[2] - pOut.main_key[1] * pOut.main_key[3]);
	resMatrix._23 = static_cast<float>(2.0) * (pOut.main_key[1] * pOut.main_key[2] + pOut.main_key[0] * pOut.main_key[3]);
	resMatrix._33 = static_cast<float>(1.0) - static_cast<float>(2.0) * (pOut.main_key[0] * pOut.main_key[0] + pOut.main_key[1] * pOut.main_key[1]);
	resMatrix._43 = 0.0f;

	resMatrix._14 = 0.0f;
	resMatrix._24 = 0.0f;
	resMatrix._34 = 0.0f;
	resMatrix._44 = 1.0f;
}
void PancyModelAssimp::GetRootSkinOffsetMatrix(aiNode *now_node, aiMatrix4x4 matrix_parent)
{
	if (strcmp(now_node->mName.data, model_move_skin->bone_ID) == 0)
	{
		auto mOffsetMatrix = now_node->mTransformation;
		mOffsetMatrix.Inverse();

		mOffsetMatrix = mOffsetMatrix;
		set_matrix(bind_pose_matrix, &mOffsetMatrix);
	}
	else
	{
		if (now_node->mNumChildren > 0)
		{
			for (int i = 0; i < now_node->mNumChildren; ++i)
			{
				aiMatrix4x4 parent_matrix = now_node->mTransformation * matrix_parent;
				GetRootSkinOffsetMatrix(now_node->mChildren[i], parent_matrix);
			}
		}
	}
}
/*
class PancyModelControl : public PancystarEngine::PancyBasicResourceControl
{
private:
	PancyModelControl(const std::string &resource_type_name_in);
public:
	static PancyModelControl* GetInstance()
	{
		static PancyModelControl* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new PancyModelControl("Model Resource Control");
		}
		return this_instance;
	}
private:
	PancystarEngine::EngineFailReason BuildResource(const std::string &desc_file_in, PancystarEngine::PancyBasicVirtualResource** resource_out);
};
PancystarEngine::EngineFailReason BuildResource(const std::string &desc_file_in, PancystarEngine::PancyBasicVirtualResource** resource_out)
{

}
*/

void scene_test_simple::updateinput(float delta_time)
{
	float move_speed = 0.15f;
	auto user_input = PancyInput::GetInstance();
	auto scene_camera = PancyCamera::GetInstance();
	user_input->GetInput();
	if (user_input->CheckKeyboard(DIK_A))
	{
		scene_camera->WalkRight(-move_speed);
	}
	if (user_input->CheckKeyboard(DIK_W))
	{
		scene_camera->WalkFront(move_speed);
	}
	if (user_input->CheckKeyboard(DIK_R))
	{
		scene_camera->WalkUp(move_speed);
	}
	if (user_input->CheckKeyboard(DIK_D))
	{
		scene_camera->WalkRight(move_speed);
	}
	if (user_input->CheckKeyboard(DIK_S))
	{
		scene_camera->WalkFront(-move_speed);
	}
	if (user_input->CheckKeyboard(DIK_F))
	{
		scene_camera->WalkUp(-move_speed);
	}
	if (user_input->CheckKeyboard(DIK_Q))
	{
		scene_camera->RotationLook(0.001f);
	}
	if (user_input->CheckKeyboard(DIK_E))
	{
		scene_camera->RotationLook(-0.001f);
	}
	if (user_input->CheckMouseDown(1))
	{
		scene_camera->RotationUp(user_input->MouseMove_X() * 0.001f);
		scene_camera->RotationRight(user_input->MouseMove_Y() * 0.001f);
	}
}
PancystarEngine::EngineFailReason scene_test_simple::ScreenChange()
{
	PancystarEngine::EngineFailReason check_error;
	view_port.TopLeftX = 0;
	view_port.TopLeftY = 0;
	view_port.Width = static_cast<FLOAT>(Scene_width);
	view_port.Height = static_cast<FLOAT>(Scene_height);
	view_port.MaxDepth = 1.0f;
	view_port.MinDepth = 0.0f;
	view_rect.left = 0;
	view_rect.top = 0;
	view_rect.right = Scene_width;
	view_rect.bottom = Scene_height;
	//����������������������Ⱦ�Լ���ݻض
	std::vector<D3D12_HEAP_FLAGS> heap_flags;
	//������Ļ�ռuint4���ȾĿ���ʽ
	D3D12_RESOURCE_DESC uint_tex_desc;
	uint_tex_desc.Alignment = 0;
	uint_tex_desc.DepthOrArraySize = 1;
	uint_tex_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	uint_tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	uint_tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UINT;
	uint_tex_desc.Height = Scene_height;
	uint_tex_desc.Width = Scene_width;
	uint_tex_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	uint_tex_desc.MipLevels = 1;
	uint_tex_desc.SampleDesc.Count = 1;
	uint_tex_desc.SampleDesc.Quality = 0;
	std::string subres_name;
	heap_flags.clear();
	heap_flags.push_back(D3D12_HEAP_FLAG_DENY_BUFFERS);
	heap_flags.push_back(D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES);
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->BuildTextureTypeJson(uint_tex_desc, 1, D3D12_HEAP_TYPE_DEFAULT, heap_flags, D3D12_RESOURCE_STATE_COMMON, subres_name);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	std::string RGB8uint_file_data = "screentarget\\screen_" + std::to_string(Scene_width) + "_" + std::to_string(Scene_height) + "_RGBA8UINT.json";
	if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(RGB8uint_file_data))
	{
		Json::Value json_data_out;
		//�����Դ��ʽ
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfFromFile", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFSRV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfRTV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFUAV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFDSV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "SubResourceFile", subres_name);
		//д���ļ���Ϊ�Ѵ���
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, RGB8uint_file_data);
		//���ļ���Ϊ�Ѿ�����
		PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(RGB8uint_file_data);
	}
	//������Ļ�ռreadback仺���
	uint64_t subresources_size;
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetCopyableFootprints(&uint_tex_desc, 0, 1, 0, nullptr, nullptr, nullptr, &subresources_size);
	if (subresources_size % 65536 != 0)
	{
		//65536���
		subresources_size = (subresources_size + 65536) & ~65535;
	}
	texture_size = subresources_size;
	D3D12_RESOURCE_DESC default_tex_readback;
	default_tex_readback.Alignment = 0;
	default_tex_readback.DepthOrArraySize = 1;
	default_tex_readback.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	default_tex_readback.Flags = D3D12_RESOURCE_FLAG_NONE;
	default_tex_readback.Format = DXGI_FORMAT_UNKNOWN;
	default_tex_readback.Height = 1;
	default_tex_readback.Width = subresources_size;
	default_tex_readback.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	default_tex_readback.MipLevels = 1;
	default_tex_readback.SampleDesc.Count = 1;
	default_tex_readback.SampleDesc.Quality = 0;
	heap_flags.clear();
	heap_flags.push_back(D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->BuildTextureTypeJson(default_tex_readback, 1, D3D12_HEAP_TYPE_READBACK, heap_flags, D3D12_RESOURCE_STATE_COPY_DEST, subres_name);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	std::string readback_data = "screentarget\\screen_" + std::to_string(Scene_width) + "_" + std::to_string(Scene_height) + "_RGBA8_readback.json";
	if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(readback_data))
	{
		Json::Value json_data_out;
		//������Դ��ʽ
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfFromFile", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFSRV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfRTV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFUAV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFDSV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "SubResourceFile", subres_name);
		//д���ļ���Ϊ�Ѵ���
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, readback_data);
		//���ļ���Ϊ�Ѿ�����
		PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(readback_data);
	}
	for (int i = 0; i < 2; ++i)
	{
		//�������ɵĸ�ʽ���������������
		if (if_readback_build)
		{
			//�֮ǰ�Ѿ������������ݣ�ɾ��֮ǰ�ı��
			check_error = PancystarEngine::PancyTextureControl::GetInstance()->DeleteResurceReference(tex_uint_save[i]);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			check_error = PancystarEngine::PancyTextureControl::GetInstance()->DeleteResurceReference(read_back_buffer[i]);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			if_readback_build = false;
		}
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(RGB8uint_file_data, tex_uint_save[i]);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(readback_data, read_back_buffer[i]);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		//ݴ���һ��������Ȼ����
		std::string depth_stencil_use = "screentarget\\screen_" + std::to_string(Scene_width) + "_" + std::to_string(Scene_height) + "_DSV.json";
		auto check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(depth_stencil_use, depth_stencil_mask[i]);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		//����ģ�建��������
		SubMemoryPointer tex_resource_data;
		D3D12_DEPTH_STENCIL_VIEW_DESC DSV_desc;
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(depth_stencil_mask[i], tex_resource_data);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->GetDSVDesc(depth_stencil_mask[i], DSV_desc);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		std::string dsv_descriptor_name = "json\\descriptor_heap\\DSV_1_descriptor_heap.json";
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(dsv_descriptor_name, dsv_mask[i].resource_view_pack_id);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		dsv_mask[i].resource_view_offset_id = 0;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildDSV(dsv_mask[i], tex_resource_data, DSV_desc);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		//���ȾĿ�
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_uint_save[i], tex_resource_data);
		D3D12_RENDER_TARGET_VIEW_DESC RTV_desc;
		PancystarEngine::PancyTextureControl::GetInstance()->GetRTVDesc(tex_uint_save[i], RTV_desc);
		RTV_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		std::string rtv_descriptor_name = "json\\descriptor_heap\\RTV_1_descriptor_heap.json";
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(rtv_descriptor_name, rtv_mask[i].resource_view_pack_id);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		rtv_mask[i].resource_view_offset_id = 0;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildRTV(rtv_mask[i], tex_resource_data, RTV_desc);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		//괴����Ⱦ�ض���ݵĿ�����ʽ
		int64_t per_memory_size;
		SubMemoryPointer sub_res_rtv;
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_uint_save[i], sub_res_rtv);
		auto memory_rtv_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_rtv, per_memory_size);
		SubMemoryPointer sub_res_read_back;
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(read_back_buffer[i], sub_res_read_back);
		auto memory_read_back_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_read_back, per_memory_size);

		UINT64 MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64));
		void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
		UINT64* pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + 1);
		UINT* pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + 1);
		D3D12_RESOURCE_DESC Desc = memory_rtv_data->GetResource()->GetDesc();
		UINT64 RequiredSize = 0;
		PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetCopyableFootprints(&Desc, 0, 1, 0, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);
		CD3DX12_TEXTURE_COPY_LOCATION dst_loc_desc(memory_read_back_data->GetResource().Get(), pLayouts[0]);
		CD3DX12_TEXTURE_COPY_LOCATION src_loc_desc(memory_rtv_data->GetResource().Get(), 0);
		dst_loc = dst_loc_desc;
		src_loc = src_loc_desc;
		HeapFree(GetProcessHeap(), 0, pMem);
	}

	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::PretreatPbrDescriptor()
{
	PancystarEngine::EngineFailReason check_error;
	std::unordered_map<std::string, std::string> Cbuffer_Heap_desc;
	PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_pbr.json")->GetCbufferHeapName(Cbuffer_Heap_desc);
	std::vector<DescriptorTableDesc> descriptor_use_data;
	PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_pbr.json")->GetDescriptorHeapUse(descriptor_use_data);
	int count = 0;
	for (auto cbuffer_data = Cbuffer_Heap_desc.begin(); cbuffer_data != Cbuffer_Heap_desc.end(); ++cbuffer_data)
	{
		check_error = SubresourceControl::GetInstance()->BuildSubresourceFromFile(cbuffer_data->second, cbuffer_model[count]);
		count += 1;
	}
	ResourceViewPack globel_var;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(descriptor_use_data[0].descriptor_heap_name, globel_var);
	//�ȴ�����cbufferview
	table_offset_model[0].resource_view_pack_id = globel_var;
	table_offset_model[0].resource_view_offset_id = descriptor_use_data[0].table_offset[0];
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(table_offset_model[0], cbuffer_model[0]);
	table_offset_model[1].resource_view_pack_id = globel_var;
	table_offset_model[1].resource_view_offset_id = descriptor_use_data[0].table_offset[1];
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(table_offset_model[1], cbuffer_model[1]);
	//�����srv
	SubMemoryPointer texture_need;
	//tex_id = tex_brdf_id;
	table_offset_model[2].resource_view_pack_id = globel_var;
	table_offset_model[2].resource_view_offset_id = descriptor_use_data[0].table_offset[2];
	ResourceViewPointer new_rvp = table_offset_model[2];
	D3D12_SHADER_RESOURCE_VIEW_DESC SRV_desc;
	//����淴�价���
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_ibl_spec_id, texture_need);
	PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_ibl_spec_id, SRV_desc);
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	//����价���
	new_rvp.resource_view_offset_id += 1;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_ibl_diffuse_id, texture_need);
	PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_ibl_diffuse_id, SRV_desc);
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	//brdf�Ԥ�������
	new_rvp.resource_view_offset_id += 1;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_brdf_id, texture_need);
	PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_brdf_id, SRV_desc);
	SRV_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	table_offset_model[3].resource_view_pack_id = globel_var;
	table_offset_model[3].resource_view_offset_id = descriptor_use_data[0].table_offset[3];
	table_offset_model[4].resource_view_pack_id = globel_var;
	table_offset_model[4].resource_view_offset_id = descriptor_use_data[0].table_offset[4];
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::UpdatePbrDescriptor()
{
	PancystarEngine::EngineFailReason check_error;

	if (model_deal->CheckIfPointMesh())
	{
		ResourceViewPointer animation_buffer_pointer = table_offset_model[3];
		PancyModelAssimp *pointer = dynamic_cast<PancyModelAssimp*>(model_deal);
		int32_t buffer_size = 0;
		int32_t stride_size = 0;
		SubMemoryPointer buffer_need = pointer->GetPointAnimationBuffer(buffer_size, stride_size);
		D3D12_SHADER_RESOURCE_VIEW_DESC  SRV_desc = {};
		SRV_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRV_desc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;
		SRV_desc.Buffer.StructureByteStride = stride_size;
		SRV_desc.Buffer.NumElements = buffer_size;
		SRV_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		SRV_desc.Buffer.FirstElement = 0;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(animation_buffer_pointer, buffer_need, SRV_desc);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
	}
	ResourceViewPointer new_rvp = table_offset_model[4];
	SubMemoryPointer texture_need;
	D3D12_SHADER_RESOURCE_VIEW_DESC SRV_desc;
	for (int i = 0; i < model_deal->GetMaterialNum(); ++i)
	{
		pancy_object_id now_tex_id;
		//��������
		check_error = model_deal->GetMateriaTexture(i, TexType::tex_diffuse, now_tex_id);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(now_tex_id, texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(now_tex_id, SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		new_rvp.resource_view_offset_id += 1;
		//��������
		check_error = model_deal->GetMateriaTexture(i, TexType::tex_normal, now_tex_id);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(now_tex_id, texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(now_tex_id, SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		new_rvp.resource_view_offset_id += 1;
		if (model_deal->GetModelPbrDesc() == PbrMaterialType::PbrType_MetallicRoughness)
		{
			//��������
			check_error = model_deal->GetMateriaTexture(i, TexType::tex_metallic, now_tex_id);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(now_tex_id, texture_need);
			PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(now_tex_id, SRV_desc);
			check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			new_rvp.resource_view_offset_id += 1;
			//�ֲڶ����
			check_error = model_deal->GetMateriaTexture(i, TexType::tex_roughness, now_tex_id);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(now_tex_id, texture_need);
			PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(now_tex_id, SRV_desc);
			check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			new_rvp.resource_view_offset_id += 1;
		}
		else if (model_deal->GetModelPbrDesc() == PbrMaterialType::PbrType_SpecularSmoothness)
		{
			//�����&�⻬�����
			check_error = model_deal->GetMateriaTexture(i, TexType::tex_specular_smoothness, now_tex_id);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(now_tex_id, texture_need);
			PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(now_tex_id, SRV_desc);
			check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
			new_rvp.resource_view_offset_id += 1;
		}
		//ao����
		check_error = model_deal->GetMateriaTexture(i, TexType::tex_ambient, now_tex_id);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(now_tex_id, texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(now_tex_id, SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
		new_rvp.resource_view_offset_id += 1;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::LoadDealModel(
	std::string file_name,
	int32_t &model_part_num,
	std::vector<std::vector<int32_t>> &Lod_out,
	bool &if_skin_mesh,
	std::vector<std::string> &animation_list
)
{
	PancystarEngine::EngineFailReason check_error;
	if (if_load_model)
	{
		//����Ѿ����ص�ģ�ͣ���ɾ��ģ�͵ı��
		delete model_deal;
		model_deal = NULL;
		if_load_model = false;
	}
	//ݼ���ģ�
	model_deal = new PancyModelAssimp(file_name, "json\\pipline_state_object\\pso_pbr.json");
	check_error = model_deal->Create();
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	if_skin_mesh = model_deal->CheckIfSkinMesh();
	PancyModelAssimp *assimp_pointer = dynamic_cast<PancyModelAssimp*>(model_deal);
	if (if_skin_mesh)
	{
		assimp_pointer->GetAnimationNameList(animation_list);
	}
	//�Ԥ���ؽ���/ȴֲڶ���
	std::vector<PancySubModel*> model_resource_list;
	model_deal->GetRenderMesh(model_resource_list);
	model_part_num = model_resource_list.size();
	GetDealModelLodPart(Lod_out);
	//ݸ���ģ�͵�������ݵ�descriptor_heap
	check_error = UpdatePbrDescriptor();
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	if_load_model = true;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::Init()
{
	PancystarEngine::EngineFailReason check_error;
	//������ʱ�d3d11��豸��������ѹ�

	UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, 0, 0, D3D11_SDK_VERSION, &device_pancy, &featureLevel, &contex_pancy);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "D3D11CreateDevice Failed.",error_message);
		
		return error_message;
	}
	//��ȫ������
	PancystarEngine::Point2D point[4];
	point[0].position = DirectX::XMFLOAT4(-1.0f, -1.0f, 0.0f, 1.0f);
	point[1].position = DirectX::XMFLOAT4(-1.0f, +1.0f, 0.0f, 1.0f);
	point[2].position = DirectX::XMFLOAT4(+1.0f, +1.0f, 0.0f, 1.0f);
	point[3].position = DirectX::XMFLOAT4(+1.0f, -1.0f, 0.0f, 1.0f);
	point[0].tex_color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
	point[1].tex_color = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	point[2].tex_color = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
	point[3].tex_color = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 0.0f);
	UINT index[] = { 0,1,2 ,0,2,3 };
	test_model = new PancystarEngine::GeometryCommonModel<PancystarEngine::Point2D>(point, index, 4, 6);
	check_error = test_model->Create();
	if (!check_error.if_succeed)
	{
		return check_error;
	}

	//μ���һ�pso
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_test.json");
	if (!check_error.if_succeed)
	{
		return check_error;
	}

	//�ģ�ͼ��ز��
	model_sky = new PancyModelAssimp("model\\ball\\ball.obj", "json\\pipline_state_object\\pso_test.json");
	check_error = model_sky->Create();


	if (!check_error.if_succeed)
	{
		return check_error;
	}
	model_cube = new PancyModelAssimp("model\\ball\\square.obj", "json\\pipline_state_object\\pso_test.json");
	check_error = model_cube->Create();
	if (!check_error.if_succeed)
	{
		return check_error;
	}

	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_pbr.json");
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_pbr_bone.json");
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_pbr_pointcatch.json");
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_geometry_normal.json");
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_screenmask.json");
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_boundbox.json");
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	/*
	model_deal = new PancyModelAssimp("model\\ball2\\ball.obj", "json\\pipline_state_object\\pso_pbr.json");
	check_error = model_deal->Create();
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	*/
	//Դ���һ�cbuffer
	//����һ�pso
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_sky.json");
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	std::unordered_map<std::string, std::string> Cbuffer_Heap_desc;
	PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_sky.json")->GetCbufferHeapName(Cbuffer_Heap_desc);
	std::vector<DescriptorTableDesc> descriptor_use_data;
	PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_sky.json")->GetDescriptorHeapUse(descriptor_use_data);
	int count = 0;
	for (auto cbuffer_data = Cbuffer_Heap_desc.begin(); cbuffer_data != Cbuffer_Heap_desc.end(); ++cbuffer_data)
	{
		check_error = SubresourceControl::GetInstance()->BuildSubresourceFromFile(cbuffer_data->second, cbuffer[count]);
		count += 1;
	}
	ResourceViewPack globel_var;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(descriptor_use_data[0].descriptor_heap_name, globel_var);
	//ResourceViewPointer new_rsv;
	table_offset[0].resource_view_pack_id = globel_var;
	table_offset[0].resource_view_offset_id = descriptor_use_data[0].table_offset[0];
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(table_offset[0], cbuffer[0]);
	table_offset[1].resource_view_pack_id = globel_var;
	table_offset[1].resource_view_offset_id = descriptor_use_data[0].table_offset[1];
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(table_offset[1], cbuffer[1]);
	//�Ԥ���brdf
	check_error = PretreatBrdf();
	if (!check_error.if_succeed)
	{
		return check_error;
	}

	//������Ҫ�pbr����
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\Cubemap.json", tex_ibl_spec_id);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\IrradianceMap.json", tex_ibl_diffuse_id);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	/*
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\Sphere002_roughness.json", tex_roughness_id);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	*/
	//�Ϊpbrģ�͵���Ⱦ����descriptor
	check_error = PretreatPbrDescriptor();
	if (!check_error.if_succeed)
	{
		return check_error;
	}

	SubMemoryPointer texture_need;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_ibl_spec_id, texture_need);
	table_offset[2].resource_view_pack_id = globel_var;
	table_offset[2].resource_view_offset_id = descriptor_use_data[0].table_offset[2];
	D3D12_SHADER_RESOURCE_VIEW_DESC SRV_desc;
	PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_ibl_spec_id, SRV_desc);
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(table_offset[2], texture_need, SRV_desc);
	if (!check_error.if_succeed)
	{
		return check_error;
	}

	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::PretreatBrdf()
{
	CD3DX12_VIEWPORT view_port_brdf;
	CD3DX12_RECT view_rect_brdf;
	view_port_brdf.TopLeftX = 0;
	view_port_brdf.TopLeftY = 0;
	view_port_brdf.Width = 1024.0;
	view_port_brdf.Height = 1024.0;
	view_port_brdf.MaxDepth = 1.0f;
	view_port_brdf.MinDepth = 0.0f;
	view_rect_brdf.left = 0;
	view_rect_brdf.top = 0;
	view_rect_brdf.right = 1024;
	view_rect_brdf.bottom = 1024;
	PancystarEngine::EngineFailReason check_error;
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_brdfgen.json");
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	//���brdf�Ԥ�������
	//todo:commandalloctor���֡��Ҫ����̳߳
	//todoأ����resourcedesc����heap㼰�ֿ�Ĵ�С
	//pancy_object_id tex_brdf_id;
	SubMemoryPointer texture_brdf_need;
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("json\\texture\\1024_1024_R16B16G16A16FLOAT.json", tex_brdf_id);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	//������ȾĿ�
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_brdf_id, texture_brdf_need);
	D3D12_RENDER_TARGET_VIEW_DESC RTV_desc;
	PancystarEngine::PancyTextureControl::GetInstance()->GetRTVDesc(tex_brdf_id, RTV_desc);
	RTV_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	std::string dsv_descriptor_name = "json\\descriptor_heap\\RTV_1_descriptor_heap.json";
	ResourceViewPointer RTV_pointer;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(dsv_descriptor_name, RTV_pointer.resource_view_pack_id);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	RTV_pointer.resource_view_offset_id = 0;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildRTV(RTV_pointer, texture_brdf_need, RTV_desc);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	//�����Ԥ��Ⱦ���
	PancyRenderCommandList *m_commandList;
	auto pso_data = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_brdfgen.json");
	PancyThreadIdGPU commdlist_id_use;
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(pso_data->GetData(), &m_commandList, commdlist_id_use);
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port_brdf);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect_brdf);
	auto rootsignature_data = pso_data->GetRootSignature()->GetResource();
	m_commandList->GetCommandList()->SetGraphicsRootSignature(rootsignature_data.Get());
	//�������ȾĿ�
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	int64_t per_mem_size;
	auto rtv_res_data = SubresourceControl::GetInstance()->GetResourceData(texture_brdf_need, per_mem_size);
	ComPtr<ID3D12Resource> screen_rendertarget = rtv_res_data->GetResource();
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(RTV_pointer, rtvHandle);

	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, NULL);
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	//���Ⱦ�����
	m_commandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &test_model->GetVertexBufferView());
	m_commandList->GetCommandList()->IASetIndexBuffer(&test_model->GetIndexBufferView());
	m_commandList->GetCommandList()->DrawIndexedInstanced(test_model->GetIndexNum(), 1, 0, 0, 0);

	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->UnlockPrepare();
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(1, &commdlist_id_use);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
void scene_test_simple::ReadBackData(int x_mouse, int y_mouse)
{
	int64_t per_memory_size;
	int32_t now_render_num = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	SubMemoryPointer sub_res_read_back;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(read_back_buffer[now_render_num], sub_res_read_back);
	auto memory_read_back_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_read_back, per_memory_size);
	uint8_t* read_back_data;
	CD3DX12_RANGE readRange(0, 0);
	memory_read_back_data->GetResource()->Map(0, &readRange, reinterpret_cast<void**>(&read_back_data));
	now_point_answer = read_back_data[y_mouse *Scene_width * 4 + x_mouse * 4 + 0];
	memory_read_back_data->GetResource()->Unmap(0, &readRange);
}
void scene_test_simple::Display()
{
	HRESULT hr;
	renderlist_ID.clear();
	auto check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->FreeAlloctor();
	ClearScreen();
	//PopulateCommandList(model_sky);
	PopulateCommandList(model_cube);
	PopulateCommandListSky();
	if (if_load_model)
	{
		PopulateCommandListModelDeal();
		if (if_show_boundbox)
		{
			PopulateCommandListModelDealBound();
		}
		PopulateCommandListReadBack();
	}
	if (renderlist_ID.size() > 0)
	{
		check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(renderlist_ID.size(), &renderlist_ID[0]);
		ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SetGpuBrokenFence(broken_fence_id);
		hr = PancyDx12DeviceBasic::GetInstance()->GetSwapchain()->Present(1, 0);
		WaitForPreviousFrame();
		ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SetGpuBrokenFence(broken_fence_id);
	}
	//�ضGPU���
	if (if_pointed && if_load_model)
	{
		ReadBackData(x_point, y_point);
		if_pointed = false;
	}
}
void scene_test_simple::DisplayEnvironment(DirectX::XMFLOAT4X4 view_matrix, DirectX::XMFLOAT4X4 proj_matrix)
{
}
void scene_test_simple::ClearScreen()
{
	PancyRenderCommandList *m_commandList;
	PancyThreadIdGPU commdlist_id_use;
	auto check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(NULL, &m_commandList, commdlist_id_use);
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ComPtr<ID3D12Resource> screen_rendertarget = PancyDx12DeviceBasic::GetInstance()->GetBackBuffer(rtvHandle);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	//��޸���Դ��ʽΪdsv
	int32_t now_render_num = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	SubMemoryPointer sub_res_dsv;
	int64_t per_memory_size;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(Default_depthstencil_buffer[now_render_num], sub_res_dsv);
	auto memory_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_dsv, per_memory_size);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_data->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	//��ȡ��Ȼ����
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(Default_depthstencil_view[now_render_num], dsvHandle);

	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_data->GetResource().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->UnlockPrepare();
	renderlist_ID.push_back(commdlist_id_use);
}
void scene_test_simple::PopulateCommandListSky()
{
	PancystarEngine::EngineFailReason check_error;

	PancyRenderCommandList *m_commandList;
	PancyModelAssimp *render_object = dynamic_cast<PancyModelAssimp*>(model_sky);
	auto pso_data = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_sky.json");
	PancyThreadIdGPU commdlist_id_use;
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(pso_data->GetData(), &m_commandList, commdlist_id_use);
	renderlist_ID.push_back(commdlist_id_use);
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect);
	auto rootsignature_data = pso_data->GetRootSignature()->GetResource();
	m_commandList->GetCommandList()->SetGraphicsRootSignature(rootsignature_data.Get());
	//����������
	ID3D12DescriptorHeap *heap_pointer;
	heap_pointer = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(table_offset[0].resource_view_pack_id.descriptor_heap_type_id).Get();
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &heap_pointer);
	//����������ѵ�ƫ�
	for (int i = 0; i < 3; ++i)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
		auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(table_offset[i], srvHandle);
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(i, srvHandle);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ComPtr<ID3D12Resource> screen_rendertarget = PancyDx12DeviceBasic::GetInstance()->GetBackBuffer(rtvHandle);

	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));


	//��޸���Դ��ʽΪdsv
	int32_t now_render_num = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	screen_rendertarget->SetName(PancystarEngine::PancyString("back_buffer" + std::to_string(now_render_num)).GetUnicodeString().c_str());
	SubMemoryPointer sub_res_dsv;
	int64_t per_memory_size;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(Default_depthstencil_buffer[now_render_num], sub_res_dsv);
	auto memory_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_dsv, per_memory_size);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_data->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	//��ȡ��Ȼ����
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(Default_depthstencil_view[now_render_num], dsvHandle);

	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	//const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	//m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	std::vector<PancySubModel*> model_resource_list;
	render_object->GetRenderMesh(model_resource_list);
	for (int i = 0; i < model_resource_list.size(); ++i)
	{
		m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &model_resource_list[i]->GetVertexBufferView());
		m_commandList->GetCommandList()->IASetIndexBuffer(&model_resource_list[i]->GetIndexBufferView());
		m_commandList->GetCommandList()->DrawIndexedInstanced(model_resource_list[i]->GetIndexNum(), 1, 0, 0, 0);
	}
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_data->GetResource().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->UnlockPrepare();
}
void scene_test_simple::PopulateCommandListReadBack()
{
	PancystarEngine::EngineFailReason check_error;
	int32_t now_render_num = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	PancyRenderCommandList *m_commandList;
	PancyModelAssimp *render_object = dynamic_cast<PancyModelAssimp*>(model_deal);
	auto pso_data = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_screenmask.json");
	PancyThreadIdGPU commdlist_id_use;
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(pso_data->GetData(), &m_commandList, commdlist_id_use);
	renderlist_ID.push_back(commdlist_id_use);
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect);
	auto rootsignature_data = pso_data->GetRootSignature()->GetResource();
	m_commandList->GetCommandList()->SetGraphicsRootSignature(rootsignature_data.Get());
	//����������
	ID3D12DescriptorHeap *heap_pointer;
	heap_pointer = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(table_offset_model[0].resource_view_pack_id.descriptor_heap_type_id).Get();
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &heap_pointer);
	//����������ѵ�ƫ�
	for (int i = 0; i < 2; ++i)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
		auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(table_offset_model[i], srvHandle);
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(i, srvHandle);
	}
	//bindless texture
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
	auto heap_offset_bindless = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(table_offset_model[4], srvHandle);
	m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(2, srvHandle);
	//���ȾĿ�
	int64_t per_memory_size;
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	SubMemoryPointer sub_res_rtv;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_uint_save[now_render_num], sub_res_rtv);
	auto memory_rtv_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_rtv, per_memory_size);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_rtv_data->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(rtv_mask[now_render_num], rtvHandle);
	//��޸���Դ��ʽΪdsv

	SubMemoryPointer sub_res_dsv;

	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(depth_stencil_mask[now_render_num], sub_res_dsv);
	auto memory_dsv_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_dsv, per_memory_size);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_dsv_data->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	//��ȡ��Ȼ����
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(dsv_mask[now_render_num], dsvHandle);

	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	const float clearColor[] = { 255.0f, 255.0f, 255.0f, 255.0f };
	m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_commandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	std::vector<PancySubModel*> model_resource_list;
	render_object->GetRenderMesh(model_resource_list);
	for (int i = 0; i < model_resource_list.size(); ++i)
	{
		m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &model_resource_list[i]->GetVertexBufferView());
		m_commandList->GetCommandList()->IASetIndexBuffer(&model_resource_list[i]->GetIndexBufferView());
		m_commandList->GetCommandList()->DrawIndexedInstanced(model_resource_list[i]->GetIndexNum(), 1, 0, 0, 0);
	}
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_rtv_data->GetResource().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	SubMemoryPointer sub_res_read_back;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(read_back_buffer[now_render_num], sub_res_read_back);
	auto memory_read_back_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_read_back, per_memory_size);

	//CD3DX12_TEXTURE_COPY_LOCATION Dst(tex_data_res->GetResource().Get(), i + 0);
	//CD3DX12_TEXTURE_COPY_LOCATION Src(copy_data_res->GetResource().Get(), pLayouts[i]);
	m_commandList->GetCommandList()->CopyTextureRegion(&dst_loc, 0, 0, 0, &src_loc, nullptr);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_dsv_data->GetResource().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->UnlockPrepare();
}
void scene_test_simple::PopulateCommandListModelDeal()
{
	int32_t now_render_num = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	PancystarEngine::EngineFailReason check_error;

	PancyRenderCommandList *m_commandList;
	PancyModelAssimp *render_object = dynamic_cast<PancyModelAssimp*>(model_deal);
	PancyPiplineStateObjectGraph* pso_data;
	if (render_object->CheckIfSkinMesh())
	{
		pso_data = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_pbr_bone.json");
	}
	else if (render_object->CheckIfPointMesh())
	{
		if (if_show_normal && !if_show_normal_point)
		{
			pso_data = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_geometry_normal.json");
		}
		else
		{
			pso_data = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_pbr_pointcatch.json");
		}
	}
	else
	{
		pso_data = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_pbr.json");
	}
	PancyThreadIdGPU commdlist_id_use;
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(pso_data->GetData(), &m_commandList, commdlist_id_use);
	renderlist_ID.push_back(commdlist_id_use);
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect);
	auto rootsignature_data = pso_data->GetRootSignature()->GetResource();
	m_commandList->GetCommandList()->SetGraphicsRootSignature(rootsignature_data.Get());
	//����������
	ID3D12DescriptorHeap *heap_pointer;
	heap_pointer = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(table_offset_model[0].resource_view_pack_id.descriptor_heap_type_id).Get();
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &heap_pointer);
	//����������ѵ�ƫ�
	for (int i = 0; i < 5; ++i)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
		auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(table_offset_model[i], srvHandle);
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(i, srvHandle);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ComPtr<ID3D12Resource> screen_rendertarget = PancyDx12DeviceBasic::GetInstance()->GetBackBuffer(rtvHandle);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	//��޸���Դ��ʽΪdsv
	SubMemoryPointer sub_res_dsv;
	int64_t per_memory_size;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(Default_depthstencil_buffer[now_render_num], sub_res_dsv);
	auto memory_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_dsv, per_memory_size);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_data->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	//��ȡ��Ȼ����
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(Default_depthstencil_view[now_render_num], dsvHandle);

	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);


	m_commandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	std::vector<PancySubModel*> model_resource_list;
	render_object->GetRenderMesh(model_resource_list);
	if (if_only_show_part)
	{
		for (int i = 0; i < now_show_part.size(); ++i)
		{
			m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &model_resource_list[now_show_part[i]]->GetVertexBufferView());
			m_commandList->GetCommandList()->IASetIndexBuffer(&model_resource_list[now_show_part[i]]->GetIndexBufferView());
			m_commandList->GetCommandList()->DrawIndexedInstanced(model_resource_list[now_show_part[i]]->GetIndexNum(), 1, 0, 0, 0);
		}
	}
	else
	{
		for (int i = 0; i < model_resource_list.size(); ++i)
		{
			m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &model_resource_list[i]->GetVertexBufferView());
			m_commandList->GetCommandList()->IASetIndexBuffer(&model_resource_list[i]->GetIndexBufferView());
			m_commandList->GetCommandList()->DrawIndexedInstanced(model_resource_list[i]->GetIndexNum(), 1, 0, 0, 0);
		}
	}
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_data->GetResource().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->UnlockPrepare();
}
void scene_test_simple::PopulateCommandList(PancyModelBasic *now_res)
{
	PancystarEngine::EngineFailReason check_error;

	PancyRenderCommandList *m_commandList;
	PancyModelAssimp *render_object = dynamic_cast<PancyModelAssimp*>(now_res);
	auto pso_data = render_object->GetPso()->GetData();
	PancyThreadIdGPU commdlist_id_use;
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(pso_data, &m_commandList, commdlist_id_use);
	renderlist_ID.push_back(commdlist_id_use);
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect);
	auto rootsignature_data = render_object->GetPso()->GetRootSignature()->GetResource();
	m_commandList->GetCommandList()->SetGraphicsRootSignature(rootsignature_data.Get());
	//����������
	ID3D12DescriptorHeap *heap_pointer;
	heap_pointer = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(render_object->GetDescriptorHeap()[0].resource_view_pack_id.descriptor_heap_type_id).Get();
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &heap_pointer);
	//����������ѵ�ƫ�
	for (int i = 0; i < 3; ++i)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
		auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(render_object->GetDescriptorHeap()[i], srvHandle);
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(i, srvHandle);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ComPtr<ID3D12Resource> screen_rendertarget = PancyDx12DeviceBasic::GetInstance()->GetBackBuffer(rtvHandle);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));


	//��޸���Դ��ʽΪdsv
	int32_t now_render_num = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	SubMemoryPointer sub_res_dsv;
	int64_t per_memory_size;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(Default_depthstencil_buffer[now_render_num], sub_res_dsv);
	auto memory_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_dsv, per_memory_size);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_data->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	//��ȡ��Ȼ����
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(Default_depthstencil_view[now_render_num], dsvHandle);

	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	//const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	//m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	std::vector<PancySubModel*> model_resource_list;
	render_object->GetRenderMesh(model_resource_list);
	for (int i = 0; i < model_resource_list.size(); ++i)
	{
		m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &model_resource_list[i]->GetVertexBufferView());
		m_commandList->GetCommandList()->IASetIndexBuffer(&model_resource_list[i]->GetIndexBufferView());
		m_commandList->GetCommandList()->DrawIndexedInstanced(model_resource_list[i]->GetIndexNum(), 1, 0, 0, 0);
	}
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_data->GetResource().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->UnlockPrepare();
}
void scene_test_simple::PopulateCommandListModelDealBound()
{
	int32_t now_render_num = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	PancystarEngine::EngineFailReason check_error;

	PancyRenderCommandList *m_commandList;
	PancyModelAssimp *render_object = dynamic_cast<PancyModelAssimp*>(model_deal);
	auto pso_data = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_boundbox.json");
	PancyThreadIdGPU commdlist_id_use;
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(pso_data->GetData(), &m_commandList, commdlist_id_use);
	renderlist_ID.push_back(commdlist_id_use);
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect);
	auto rootsignature_data = pso_data->GetRootSignature()->GetResource();
	m_commandList->GetCommandList()->SetGraphicsRootSignature(rootsignature_data.Get());
	//����������
	ID3D12DescriptorHeap *heap_pointer;
	heap_pointer = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(table_offset_model[0].resource_view_pack_id.descriptor_heap_type_id).Get();
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &heap_pointer);
	//����������ѵ�ƫ�
	for (int i = 0; i < 2; ++i)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
		auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(table_offset_model[i], srvHandle);
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(i, srvHandle);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ComPtr<ID3D12Resource> screen_rendertarget = PancyDx12DeviceBasic::GetInstance()->GetBackBuffer(rtvHandle);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));


	//��޸���Դ��ʽΪdsv

	SubMemoryPointer sub_res_dsv;
	int64_t per_memory_size;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(Default_depthstencil_buffer[now_render_num], sub_res_dsv);
	auto memory_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_dsv, per_memory_size);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_data->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	//��ȡ��Ȼ����
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(Default_depthstencil_view[now_render_num], dsvHandle);

	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);


	m_commandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &render_object->GetBoundBox()->GetVertexBufferView());
	m_commandList->GetCommandList()->IASetIndexBuffer(&render_object->GetBoundBox()->GetIndexBufferView());
	m_commandList->GetCommandList()->DrawIndexedInstanced(render_object->GetBoundBox()->GetIndexNum(), 1, 0, 0, 0);

	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_data->GetResource().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->UnlockPrepare();
}
/*
void scene_test_simple::PopulateCommandList()
{
	PancystarEngine::EngineFailReason check_error;

	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->FreeAlloctor();
	PancyRenderCommandList *m_commandList;
	auto pso_data = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_test.json")->GetData();
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(pso_data, &m_commandList, renderlist_ID);


	m_commandList->GetCommandList()->RSSetViewports(1, &view_port);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect);

	auto rootsignature_data = PancyRootSignatureControl::GetInstance()->GetRootSignature("json\\root_signature\\test_root_signature.json")->GetRootSignature();
	m_commandList->GetCommandList()->SetGraphicsRootSignature(rootsignature_data.Get());
	//����������
	ID3D12DescriptorHeap *heap_pointer;
	heap_pointer = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(table_offset[0].resource_view_pack_id.descriptor_heap_type_id).Get();
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &heap_pointer);
	//����������ѵ�ƫ�
	for (int i = 0; i < 3; ++i)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
		//CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(heap_pointer->GetGPUDescriptorHandleForHeapStart());
		auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(table_offset[i], srvHandle);
		//srvHandle.Offset(heap_offset);
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(i, srvHandle);
	}
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ComPtr<ID3D12Resource> screen_rendertarget = PancyDx12DeviceBasic::GetInstance()->GetBackBuffer(rtvHandle);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &test_model->GetVertexBufferView());
	m_commandList->GetCommandList()->IASetIndexBuffer(&test_model->GetIndexBufferView());
	m_commandList->GetCommandList()->DrawIndexedInstanced(3, 1, 0, 0, 0);

	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	m_commandList->UnlockPrepare();
}
*/
void scene_test_simple::WaitForPreviousFrame()
{
	auto  check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->WaitGpuBrokenFence(broken_fence_id);
}
void scene_test_simple::Update(float delta_time)
{
	PancystarEngine::EngineFailReason check_error;
	updateinput(delta_time);
	DirectX::XMFLOAT4X4 world_mat, uv_mat;
	DirectX::XMStoreFloat4x4(&uv_mat, DirectX::XMMatrixIdentity());
	DirectX::XMStoreFloat4x4(&world_mat, DirectX::XMMatrixIdentity());
	PancyModelAssimp *render_object_sky = dynamic_cast<PancyModelAssimp*>(model_sky);
	render_object_sky->update(world_mat, uv_mat, delta_time);

	DirectX::XMStoreFloat4x4(&uv_mat, DirectX::XMMatrixScaling(1000, 1000, 0));
	DirectX::XMStoreFloat4x4(&world_mat, DirectX::XMMatrixScaling(100, 2, 100)*DirectX::XMMatrixTranslation(0, -5, 0));
	PancyModelAssimp *render_object_cube = dynamic_cast<PancyModelAssimp*>(model_cube);
	render_object_cube->update(world_mat, uv_mat, delta_time);



	int64_t per_memory_size;
	auto data_submemory = SubresourceControl::GetInstance()->GetResourceData(cbuffer[0], per_memory_size);
	DirectX::XMFLOAT4X4 sky_world_mat[4];
	DirectX::XMFLOAT4X4 view_mat, inv_view_mat;
	PancyCamera::GetInstance()->CountViewMatrix(&view_mat);
	PancyCamera::GetInstance()->CountInvviewMatrix(&inv_view_mat);
	//������cbuffer
	DirectX::XMMATRIX proj_mat = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 1000.0f);
	DirectX::XMStoreFloat4x4(&sky_world_mat[0], DirectX::XMMatrixTranspose(DirectX::XMMatrixScaling(100, 100, 100)));
	DirectX::XMStoreFloat4x4(&sky_world_mat[1], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&sky_world_mat[0]) * DirectX::XMLoadFloat4x4(&view_mat) * proj_mat));
	DirectX::XMStoreFloat4x4(&sky_world_mat[2], DirectX::XMMatrixIdentity());
	DirectX::XMVECTOR x_delta;
	DirectX::XMStoreFloat4x4(&sky_world_mat[3], DirectX::XMMatrixInverse(&x_delta, DirectX::XMLoadFloat4x4(&sky_world_mat[0])));
	check_error = data_submemory->WriteFromCpuToBuffer(cbuffer[0].offset* per_memory_size, sky_world_mat, sizeof(sky_world_mat));
	//���䴦��ģ�͵cbuffer
	data_submemory = SubresourceControl::GetInstance()->GetResourceData(cbuffer_model[0], per_memory_size);
	DirectX::XMFLOAT4X4 pbr_world_mat[4];
	DirectX::XMMATRIX pbr_pre_world_mat = DirectX::XMMatrixScaling(scale_size, scale_size, scale_size) * DirectX::XMMatrixRotationX((DirectX::XM_PI * rotation_angle.x) / 180.0f) * DirectX::XMMatrixRotationY((DirectX::XM_PI * rotation_angle.y) / 180.0f) *  DirectX::XMMatrixRotationZ((DirectX::XM_PI * rotation_angle.z) / 180.0f) * DirectX::XMMatrixTranslation(translation_pos.x, translation_pos.y, translation_pos.z);
	DirectX::XMStoreFloat4x4(&pbr_world_mat[0], DirectX::XMMatrixTranspose(pbr_pre_world_mat));
	DirectX::XMStoreFloat4x4(&pbr_world_mat[1], DirectX::XMMatrixTranspose(pbr_pre_world_mat * DirectX::XMLoadFloat4x4(&view_mat) * proj_mat));
	DirectX::XMStoreFloat4x4(&pbr_world_mat[2], DirectX::XMMatrixIdentity());
	//��ȼ��3*3�������ת�
	DirectX::XMMATRIX normal_need = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(&x_delta, pbr_pre_world_mat));
	DirectX::XMFLOAT4X4 mat_normal;
	DirectX::XMStoreFloat4x4(&mat_normal, normal_need);
	mat_normal._41 = 0.0f;
	mat_normal._42 = 0.0f;
	mat_normal._43 = 0.0f;
	mat_normal._44 = 0.0f;
	DirectX::XMStoreFloat4x4(&pbr_world_mat[3], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mat_normal)));
	check_error = data_submemory->WriteFromCpuToBuffer(cbuffer_model[0].offset* per_memory_size, pbr_world_mat, sizeof(pbr_world_mat));
	PancyModelAssimp *render_object_deal = dynamic_cast<PancyModelAssimp*>(model_deal);
	if (render_object_deal != NULL)
	{
		if (render_object_deal->CheckIfSkinMesh())
		{
			DirectX::XMFLOAT4X4 bound_box_offset_mat = render_object_deal->GetModelAnimationOffset();
			DirectX::XMFLOAT4X4 bone_mat[MaxBoneNum];
			render_object_deal->GetModelBoneData(bone_mat);
			check_error = data_submemory->WriteFromCpuToBuffer(cbuffer_model[0].offset* per_memory_size + sizeof(pbr_world_mat), &bound_box_offset_mat, sizeof(bound_box_offset_mat));
			check_error = data_submemory->WriteFromCpuToBuffer(cbuffer_model[0].offset* per_memory_size + sizeof(pbr_world_mat) + sizeof(bound_box_offset_mat), bone_mat, sizeof(bone_mat));
		}
		else
		{
			DirectX::XMFLOAT4X4 bound_box_offset_mat;
			DirectX::XMStoreFloat4x4(&bound_box_offset_mat, DirectX::XMMatrixIdentity());
			check_error = data_submemory->WriteFromCpuToBuffer(cbuffer_model[0].offset* per_memory_size + sizeof(pbr_world_mat), &bound_box_offset_mat, sizeof(bound_box_offset_mat));
		}
	}
	data_submemory = SubresourceControl::GetInstance()->GetResourceData(cbuffer_model[1], per_memory_size);
	per_view_pack view_buffer_data;
	DirectX::XMStoreFloat4x4(&view_buffer_data.view_matrix, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&view_mat)));
	DirectX::XMStoreFloat4x4(&view_buffer_data.projectmatrix, DirectX::XMMatrixTranspose(proj_mat));
	DirectX::XMStoreFloat4x4(&view_buffer_data.invview_matrix, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&inv_view_mat)));
	DirectX::XMFLOAT3 view_pos;
	PancyCamera::GetInstance()->GetViewPosition(&view_pos);
	view_buffer_data.view_position.x = view_pos.x;
	view_buffer_data.view_position.y = view_pos.y;
	view_buffer_data.view_position.z = view_pos.z;
	view_buffer_data.view_position.w = 1.0f;
	check_error = data_submemory->WriteFromCpuToBuffer(cbuffer_model[1].offset* per_memory_size, &view_buffer_data, sizeof(view_buffer_data));
	//ö��㶯����
	if (render_object_deal != NULL && render_object_deal->CheckIfPointMesh())
	{
		DirectX::XMUINT4 vertex_animation_size;
		render_object_deal->GetPointAnimationFrame(vertex_animation_size.x, vertex_animation_size.y);
		//��Ƿ���ʾ���
		if (if_show_normal && if_show_normal_point)
		{
			vertex_animation_size.z = 1;
		}
		else
		{
			vertex_animation_size.z = 0;
		}
		check_error = data_submemory->WriteFromCpuToBuffer(cbuffer_model[1].offset* per_memory_size + sizeof(view_buffer_data), &vertex_animation_size, sizeof(vertex_animation_size));
	}
}
scene_test_simple::~scene_test_simple()
{
	WaitForPreviousFrame();
	delete test_model;
	delete model_cube;
	delete model_sky;
	if (if_load_model)
	{
		delete model_deal;
	}
	device_pancy->Release();
	contex_pancy->Release();
}

