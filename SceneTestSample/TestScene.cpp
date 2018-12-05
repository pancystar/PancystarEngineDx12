#include"TestScene.h"

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
	for (int32_t i = desc_file_in.size()-1; i >= 0; --i) 
	{
		if (desc_file_in[i] == '\\') 
		{
			end = i+1;
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
	if (!check_error.CheckIfSucceed()) 
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
PancyModelAssimp::PancyModelAssimp(const std::string &desc_file_in, const std::string &pso_in) :PancyModelBasic(desc_file_in)
{
	pso_use = pso_in;
}
PancystarEngine::EngineFailReason PancyModelAssimp::LoadModel(
	const std::string &resource_desc_file,
	std::vector<PancySubModel*> &model_resource,
	std::unordered_map<pancy_object_id, std::unordered_map<TexType, pancy_object_id>> &material_list,
	std::vector<pancy_object_id> &texture_use
){
	PancystarEngine::EngineFailReason check_error;
	model_need = importer.ReadFile(resource_desc_file,
		aiProcess_MakeLeftHanded |
		aiProcess_FlipWindingOrder |
		aiProcess_CalcTangentSpace |
		aiProcess_JoinIdenticalVertices
	);//将不同图元放置到不同的模型中去，图片类型可能是点、直线、三角形等
	if (!model_need)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "read model" + resource_desc_file + "error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load model from Assimp", error_message);
		return error_message;
	}
	bool use_skin_mesh = false;
	bool use_point_anim = false;
	for (unsigned int i = 0; i < model_need->mNumMaterials; ++i)
	{
		std::unordered_map<TexType, pancy_object_id> mat_tex_list;
		
		const aiMaterial* pMaterial = model_need->mMaterials[i];
		aiString Path;
		//漫反射纹理
		if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0 && pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
		{
			pancy_object_id id_need;
			check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(model_root_path + Path.C_Str(),id_need);
			if (!check_error.CheckIfSucceed()) 
			{
				return check_error;
			}
			//将纹理数据加载到材质表
			mat_tex_list.insert(std::pair<TexType, pancy_object_id>(TexType::tex_diffuse,texture_use.size()));
			texture_use.push_back(id_need);
		}
		//法线纹理
		if (pMaterial->GetTextureCount(aiTextureType_HEIGHT) > 0 && pMaterial->GetTexture(aiTextureType_HEIGHT, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
		{
			pancy_object_id id_need;
			check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(model_root_path + Path.C_Str(), id_need);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			//将纹理数据加载到材质表
			mat_tex_list.insert(std::pair<TexType, pancy_object_id>(TexType::tex_normal, texture_use.size()));
			texture_use.push_back(id_need);
		}
		else if (pMaterial->GetTextureCount(aiTextureType_NORMALS) > 0 && pMaterial->GetTexture(aiTextureType_NORMALS, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
		{
			pancy_object_id id_need;
			check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(model_root_path + Path.C_Str(), id_need);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			//将纹理数据加载到材质表
			mat_tex_list.insert(std::pair<TexType, pancy_object_id>(TexType::tex_normal, texture_use.size()));
			texture_use.push_back(id_need);
		}
		//ao纹理
		if (pMaterial->GetTextureCount(aiTextureType_AMBIENT) > 0 && pMaterial->GetTexture(aiTextureType_AMBIENT, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
		{
			pancy_object_id id_need;
			check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(model_root_path + Path.C_Str(), id_need);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			//将纹理数据加载到材质表
			mat_tex_list.insert(std::pair<TexType, pancy_object_id>(TexType::tex_ambient, texture_use.size()));
			texture_use.push_back(id_need);
		}
		material_list.insert(std::pair<pancy_object_id, std::unordered_map<TexType, pancy_object_id>>(i, mat_tex_list));
	}
	for (int i = 0; i < model_need->mNumMeshes; i++)
	{
		//获取模型的第i个模块
		const aiMesh* paiMesh = model_need->mMeshes[i];
		//获取模型的材质编号
		pancy_object_id material_use = paiMesh->mMaterialIndex;
		auto mat_list_now = material_list.find(material_use);
		if (mat_list_now == material_list.end()) 
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "havn't load the material id: "+std::to_string(material_use)+"in model:" + resource_desc_file);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load model from Assimp", error_message);
			return error_message;
		}
		//创建索引缓冲区
		IndexType *index_need = new IndexType[paiMesh->mNumFaces * 3];
		for (unsigned int j = 0; j < paiMesh->mNumFaces; j++)
		{
			if (paiMesh->mFaces[j].mNumIndices == 3)
			{
				index_need[j] = static_cast<IndexType>(paiMesh->mFaces[j].mIndices[0]);
				index_need[j] = static_cast<IndexType>(paiMesh->mFaces[j].mIndices[1]);
				index_need[j] = static_cast<IndexType>(paiMesh->mFaces[j].mIndices[2]);
			}
			else
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "model" + resource_desc_file + "find no triangle face");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load model from Assimp", error_message);
				return error_message;
			}
		}
		//创建顶点缓冲区
		if (use_skin_mesh)
		{
		}
		else if (use_point_anim)
		{
		}
		else 
		{
			PancystarEngine::PointCommon *point_need = new PancystarEngine::PointCommon[paiMesh->mNumVertices];
			for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
			{
				//从assimp中读取的数据
				point_need[j].position.x = paiMesh->mVertices[j].x;
				point_need[j].position.y = paiMesh->mVertices[j].y;
				point_need[j].position.z = paiMesh->mVertices[j].z;

				point_need[j].normal.x = paiMesh->mNormals[j].x;
				point_need[j].normal.y = paiMesh->mNormals[j].y;
				point_need[j].normal.z = paiMesh->mNormals[j].z;

				if (paiMesh->HasTextureCoords(0))
				{
					point_need[j].tex_uv.x = paiMesh->mTextureCoords[0][j].x;
					point_need[j].tex_uv.y = paiMesh->mTextureCoords[0][j].y;
					point_need[j].tex_uv.y = 1 - point_need[j].tex_uv.y;
				}
				else
				{
					point_need[j].tex_uv.x = 0.0f;
					point_need[j].tex_uv.y = 0.0f;
				}
				if (paiMesh->mTangents != NULL)
				{
					point_need[j].tangent.x = paiMesh->mTangents[j].x;
					point_need[j].tangent.y = paiMesh->mTangents[j].y;
					point_need[j].tangent.z = paiMesh->mTangents[j].z;
				}
				else
				{
					point_need[j].tangent.x = 0.0f;
					point_need[j].tangent.y = 0.0f;
					point_need[j].tangent.z = 0.0f;
				}
				//生成纹理使用数据
				//使用漫反射纹理作为第一个纹理的偏移量,uvid的y通量记录纹理数量
				point_need[j].tex_id.x = mat_list_now->second.find(TexType::tex_diffuse)->second;
				point_need[j].tex_id.y = mat_list_now->second.size();
			}
			PancySubModel *new_submodel = new PancySubModel();
			check_error = new_submodel->Create(point_need, index_need, paiMesh->mNumVertices, paiMesh->mNumFaces*3, material_use);
			if (!check_error.CheckIfSucceed()) 
			{
				return check_error;
			}
			model_resource.push_back(new_submodel);
			delete[] point_need;
		}
		delete[] index_need;
	}

	//加载临时的渲染规则
	
	//创建cbuffer
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
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	new_res_view.resource_view_pack_id = globel_var;
	new_res_view.resource_view_offset_id = descriptor_use_data[0].table_offset[0];
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(new_res_view, cbuffer[0]);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	table_offset.push_back(new_res_view);
	new_res_view.resource_view_pack_id = globel_var;
	new_res_view.resource_view_offset_id = descriptor_use_data[0].table_offset[1];
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(new_res_view, cbuffer[1]);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	table_offset.push_back(new_res_view);
	//创建纹理访问器
	for (int i = 0; i < texture_use.size(); ++i) 
	{
		//加载一张纹理
		SubMemoryPointer texture_need;
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(texture_use[i], texture_need);
		new_res_view.resource_view_pack_id = globel_var;
		new_res_view.resource_view_offset_id = descriptor_use_data[0].table_offset[2]+i;
		D3D12_SHADER_RESOURCE_VIEW_DESC SRV_desc;
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(texture_use[i], SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_res_view, texture_need, SRV_desc);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		if (i == 0) 
		{
			table_offset.push_back(new_res_view);
		}
	}
	//填充cbuffer
	int64_t per_memory_size;
	auto data_submemory = SubresourceControl::GetInstance()->GetResourceData(cbuffer[0], per_memory_size);
	DirectX::XMFLOAT4X4 world_mat[2];
	DirectX::XMStoreFloat4x4(&world_mat[0], DirectX::XMMatrixIdentity());


	DirectX::XMFLOAT3 pos = DirectX::XMFLOAT3(0, 0, -10);
	DirectX::XMFLOAT3 look = DirectX::XMFLOAT3(0, 0, 1);
	DirectX::XMFLOAT3 up = DirectX::XMFLOAT3(0, 1, 0);

	
	DirectX::XMStoreFloat4x4(&world_mat[1], DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&pos), DirectX::XMLoadFloat3(&look), DirectX::XMLoadFloat3(&up)) * DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4,1280/800, 0.1f, 1000.0f));
	//DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, DirectX::XM_PIDIV4, 0.1f, 1000.0f);
	CD3DX12_RANGE readRange(0, 0);
	UINT8* m_pCbvDataBegin;
	check_error =data_submemory->WriteFromCpuToBuffer(cbuffer[0].offset* per_memory_size, &world_mat, sizeof(world_mat));
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;

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


PancystarEngine::EngineFailReason scene_test_simple::ResetScreen(int32_t width_in, int32_t height_in)
{
	view_port.TopLeftX = 0;
	view_port.TopLeftY = 0;
	view_port.Width = static_cast<FLOAT>(width_in);
	view_port.Height = static_cast<FLOAT>(height_in);
	view_port.MaxDepth = 1.0f;
	view_port.MinDepth = 0.0f;
	view_rect.left = 0;
	view_rect.top = 0;
	view_rect.right = width_in;
	view_rect.bottom = height_in;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::Create(int32_t width_in, int32_t height_in)
{
	PancystarEngine::EngineFailReason check_error;
	//创建临时测试
	PancystarEngine::Point2D point[3];
	point[0].position = DirectX::XMFLOAT4(0.0f, 0.25f * 1.77f, 0.0f, 1);
	point[1].position = DirectX::XMFLOAT4(0.25f, -0.25f * 1.77f, 0.0f, 1);
	point[2].position = DirectX::XMFLOAT4(-0.25f, -0.25f * 1.77f, 0.0f, 1);

	point[0].tex_color = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);
	point[1].tex_color = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
	point[2].tex_color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
	UINT index[3] = { 0,1,2 };
	test_model = new PancystarEngine::GeometryCommonModel<PancystarEngine::Point2D>(point, index, 3, 3);
	check_error = test_model->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	
	//加载一个pso
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_test.json");
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}

	//模型加载测试
	new_res = new PancyModelAssimp("model\\ball\\ball.obj", "json\\pipline_state_object\\pso_test.json");
	new_res->Create();
	check_error = ResetScreen(width_in, height_in);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//创建一个cbuffer
	std::unordered_map<std::string, std::string> Cbuffer_Heap_desc;
	PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_test.json")->GetCbufferHeapName(Cbuffer_Heap_desc);
	std::vector<DescriptorTableDesc> descriptor_use_data;
	PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_test.json")->GetDescriptorHeapUse(descriptor_use_data);
	SubMemoryPointer cbuffer[2];
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
	//加载一张纹理
	pancy_object_id tex_id;
	SubMemoryPointer texture_need;
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\test222.json", tex_id);
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_id, texture_need);
	table_offset[2].resource_view_pack_id = globel_var;
	table_offset[2].resource_view_offset_id = descriptor_use_data[0].table_offset[2];
	D3D12_SHADER_RESOURCE_VIEW_DESC SRV_desc;
	PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_id, SRV_desc);
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(table_offset[2], texture_need, SRV_desc);
	//填充cbuffer
	/*
	int64_t per_memory_size;
	auto data_submemory = SubresourceControl::GetInstance()->GetResourceData(cbuffer[0], per_memory_size);
	DirectX::XMFLOAT4X4 world_mat[2];
	DirectX::XMStoreFloat4x4(&world_mat[0], DirectX::XMMatrixIdentity());
	DirectX::XMStoreFloat4x4(&world_mat[1], DirectX::XMMatrixTranslation(0.2, 0.2, 0));
	CD3DX12_RANGE readRange(0, 0);
	UINT8* m_pCbvDataBegin;
	data_submemory->GetResource()->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin));
	memcpy(m_pCbvDataBegin + (cbuffer[0].offset* per_memory_size), &world_mat, sizeof(world_mat));
	DirectX::XMFLOAT4X4 *p = reinterpret_cast<DirectX::XMFLOAT4X4*>(m_pCbvDataBegin);
	data_submemory->GetResource()->Unmap(0, NULL);
	*/
	
	return PancystarEngine::succeed;
}
void scene_test_simple::Display()
{
	HRESULT hr;
	PopulateCommandList();
	auto  check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(1, &renderlist_ID);
	ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SetGpuBrokenFence(broken_fence_id);
	hr = PancyDx12DeviceBasic::GetInstance()->GetSwapchain()->Present(1, 0);
	WaitForPreviousFrame();
}
void scene_test_simple::DisplayEnvironment(DirectX::XMFLOAT4X4 view_matrix, DirectX::XMFLOAT4X4 proj_matrix)
{
}
/*
void scene_test_simple::PopulateCommandList()
{
	PancystarEngine::EngineFailReason check_error;

	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->FreeAlloctor();
	PancyRenderCommandList *m_commandList;
	//auto pso_data = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_test.json")->GetData();
	PancyModelAssimp *render_object = dynamic_cast<PancyModelAssimp*>(new_res);

	auto pso_data = render_object->GetPso()->GetData();
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(pso_data, &m_commandList, renderlist_ID);


	m_commandList->GetCommandList()->RSSetViewports(1, &view_port);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect);

	//auto rootsignature_data = PancyRootSignatureControl::GetInstance()->GetRootSignature("json\\root_signature\\test_root_signature.json")->GetRootSignature();
	auto rootsignature_data = render_object->GetPso()->GetRootSignature()->GetRootSignature();
	m_commandList->GetCommandList()->SetGraphicsRootSignature(rootsignature_data.Get());
	//设置描述符堆
	ID3D12DescriptorHeap *heap_pointer;
	//heap_pointer = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(table_offset[0].resource_view_pack_id.descriptor_heap_type_id).Get();
	heap_pointer = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(render_object->GetDescriptorHeap()[0].resource_view_pack_id.descriptor_heap_type_id).Get();
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &heap_pointer);
	//设置描述符堆的偏移
	for (int i = 0; i < 3; ++i)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
		//CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(heap_pointer->GetGPUDescriptorHandleForHeapStart());
		//auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(table_offset[i], srvHandle);
		auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(render_object->GetDescriptorHeap()[i], srvHandle);
		
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
	std::vector<PancySubModel*> model_resource_list;
	render_object->GetRenderMesh(model_resource_list);
	for (int i = 0; i < model_resource_list.size(); ++i) 
	{
		m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &model_resource_list[i]->GetVertexBufferView());
		m_commandList->GetCommandList()->IASetIndexBuffer(&model_resource_list[i]->GetIndexBufferView());
		m_commandList->GetCommandList()->DrawIndexedInstanced(model_resource_list[i]->GetIndexNum(), 1, 0, 0, 0);
	}
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	m_commandList->UnlockPrepare();
}
*/
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
	//设置描述符堆
	ID3D12DescriptorHeap *heap_pointer;
	heap_pointer = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(table_offset[0].resource_view_pack_id.descriptor_heap_type_id).Get();
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &heap_pointer);
	//设置描述符堆的偏移
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
void scene_test_simple::WaitForPreviousFrame()
{
	auto  check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->WaitGpuBrokenFence(broken_fence_id);
}
void scene_test_simple::Update(float delta_time)
{
}
scene_test_simple::~scene_test_simple()
{
	WaitForPreviousFrame();
	delete test_model;
	delete new_res;
}