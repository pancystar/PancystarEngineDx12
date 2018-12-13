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
				index_need[j * 3 + 0] = static_cast<IndexType>(paiMesh->mFaces[j].mIndices[0]);
				index_need[j * 3 + 1] = static_cast<IndexType>(paiMesh->mFaces[j].mIndices[1]);
				index_need[j * 3 + 2] = static_cast<IndexType>(paiMesh->mFaces[j].mIndices[2]);
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
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}*/
	return PancystarEngine::succeed;

}
void PancyModelAssimp::update(DirectX::XMFLOAT4X4 world_matrix,DirectX::XMFLOAT4X4 uv_matrix, float delta_time)
{
	
	PancystarEngine::EngineFailReason check_error;
	
	DirectX::XMFLOAT4X4 view_mat;
	PancyCamera::GetInstance()->CountViewMatrix(&view_mat);
	//填充cbuffer
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


PancystarEngine::EngineFailReason scene_test_simple::ScreenChange()
{
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
	//先创建两个cbufferview
	table_offset_model[0].resource_view_pack_id = globel_var;
	table_offset_model[0].resource_view_offset_id = descriptor_use_data[0].table_offset[0];
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(table_offset_model[0], cbuffer_model[0]);
	table_offset_model[1].resource_view_pack_id = globel_var;
	table_offset_model[1].resource_view_offset_id = descriptor_use_data[0].table_offset[1];
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(table_offset_model[1], cbuffer_model[1]);
	//创建纹理srv
	SubMemoryPointer texture_need;
	//tex_id = tex_brdf_id;
	table_offset_model[2].resource_view_pack_id = globel_var;
	table_offset_model[2].resource_view_offset_id = descriptor_use_data[0].table_offset[2];
	ResourceViewPointer new_rvp = table_offset_model[2];
	D3D12_SHADER_RESOURCE_VIEW_DESC SRV_desc;
	//镜面反射环境光
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_ibl_spec_id, texture_need);
	PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_ibl_spec_id, SRV_desc);
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//漫反射环境光
	new_rvp.resource_view_offset_id += 1;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_ibl_diffuse_id, texture_need);
	PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_ibl_diffuse_id, SRV_desc);
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//brdf预处理纹理
	new_rvp.resource_view_offset_id += 1;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_brdf_id, texture_need);
	PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_brdf_id, SRV_desc);
	SRV_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//模型自身的纹理
	table_offset_model[3].resource_view_pack_id = globel_var;
	table_offset_model[3].resource_view_offset_id = descriptor_use_data[0].table_offset[3];
	new_rvp = table_offset_model[3];
	for (int i = 0; i < model_deal->GetSubModelNum(); ++i) 
	{
		pancy_object_id now_tex_id = model_deal->GetSubModelTexture(i,TexType::tex_diffuse);
		//漫反射纹理
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(now_tex_id, texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(now_tex_id, SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_rvp.resource_view_offset_id += 1;
		//法线纹理
		now_tex_id = model_deal->GetSubModelTexture(i, TexType::tex_normal);
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(now_tex_id, texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(now_tex_id, SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_rvp.resource_view_offset_id += 1;
		//金属度纹理
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_metallic_id, texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_metallic_id, SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_rvp.resource_view_offset_id += 1;
		//粗糙度纹理
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_roughness_id, texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_roughness_id, SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::Init()
{
	PancystarEngine::EngineFailReason check_error;
	//创建全屏三角形
	PancystarEngine::Point2D point[4];
	point[0].position = DirectX::XMFLOAT4(-1.0f, -1.0f, 0.0f, 1.0f);
	point[1].position = DirectX::XMFLOAT4(-1.0f, +1.0f, 0.0f, 1.0f);
	point[2].position = DirectX::XMFLOAT4(+1.0f, +1.0f, 0.0f, 1.0f);
	point[3].position = DirectX::XMFLOAT4(+1.0f, -1.0f, 0.0f, 1.0f);
	point[0].tex_color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
	point[1].tex_color = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	point[2].tex_color = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
	point[3].tex_color = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 0.0f);
	UINT index[] = { 0,1,2 ,0,2,3};
	test_model = new PancystarEngine::GeometryCommonModel<PancystarEngine::Point2D>(point, index, 4, 6);
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
	model_sky = new PancyModelAssimp("model\\ball\\ball.obj", "json\\pipline_state_object\\pso_test.json");
	check_error = model_sky->Create();
	
	
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	model_cube = new PancyModelAssimp("model\\ball\\square.obj", "json\\pipline_state_object\\pso_test.json");
	check_error = model_cube->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_pbr.json");
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	model_deal = new PancyModelAssimp("model\\ball2\\ball.obj", "json\\pipline_state_object\\pso_pbr.json");
	check_error = model_deal->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}

	//创建一个cbuffer
	//加载一个pso
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_sky.json");
	if (!check_error.CheckIfSucceed())
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
	//预处理brdf
	check_error = PretreatBrdf();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}

	//加载需要的pbr纹理
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\Cubemap.json", tex_ibl_spec_id);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\IrradianceMap.json", tex_ibl_diffuse_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\Sphere002_metallic.json", tex_metallic_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\Sphere002_roughness.json", tex_roughness_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//为pbr模型的渲染创建descriptor
	check_error = PretreatPbrDescriptor();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	



	SubMemoryPointer texture_need;
	//tex_id = tex_brdf_id;
	
	
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_ibl_spec_id, texture_need);
	table_offset[2].resource_view_pack_id = globel_var;
	table_offset[2].resource_view_offset_id = descriptor_use_data[0].table_offset[2];
	D3D12_SHADER_RESOURCE_VIEW_DESC SRV_desc;
	PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_ibl_spec_id, SRV_desc);
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(table_offset[2], texture_need, SRV_desc);
	if (!check_error.CheckIfSucceed())
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
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//加载brdf预处理纹理
	//todo:commandalloctor间隔帧需要两个线程池
	//todo：依靠resourcedesc来计算heap及分块的大小
	//pancy_object_id tex_brdf_id;
	SubMemoryPointer texture_brdf_need;
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("json\\texture\\1024_1024_R16B16G16A16FLOAT.json", tex_brdf_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//创建渲染目标
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_brdf_id, texture_brdf_need);
	D3D12_RENDER_TARGET_VIEW_DESC RTV_desc;
	PancystarEngine::PancyTextureControl::GetInstance()->GetRTVDesc(tex_brdf_id, RTV_desc);
	RTV_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	std::string dsv_descriptor_name = "json\\descriptor_heap\\RTV_1_descriptor_heap.json";
	ResourceViewPointer RTV_pointer;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(dsv_descriptor_name, RTV_pointer.resource_view_pack_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	RTV_pointer.resource_view_offset_id = 0;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildRTV(RTV_pointer, texture_brdf_need, RTV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//设置预渲染参数
	PancyRenderCommandList *m_commandList;
	auto pso_data = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_brdfgen.json");
	PancyThreadIdGPU commdlist_id_use;
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(pso_data->GetData(), &m_commandList, commdlist_id_use);
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port_brdf);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect_brdf);
	auto rootsignature_data = pso_data->GetRootSignature()->GetResource();
	m_commandList->GetCommandList()->SetGraphicsRootSignature(rootsignature_data.Get());
	//设置渲染目标
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	int64_t per_mem_size;
	auto rtv_res_data = SubresourceControl::GetInstance()->GetResourceData(texture_brdf_need, per_mem_size);
	ComPtr<ID3D12Resource> screen_rendertarget = rtv_res_data->GetResource();
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(RTV_pointer, rtvHandle);
	
	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, NULL);
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	//渲染到纹理
	m_commandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &test_model->GetVertexBufferView());
	m_commandList->GetCommandList()->IASetIndexBuffer(&test_model->GetIndexBufferView());
	m_commandList->GetCommandList()->DrawIndexedInstanced(test_model->GetIndexNum(), 1, 0, 0, 0);
	
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->UnlockPrepare();
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(1, &commdlist_id_use);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
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
	PopulateCommandListModelDeal();
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(renderlist_ID.size(), &renderlist_ID[0]);
	ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SetGpuBrokenFence(broken_fence_id);
	hr = PancyDx12DeviceBasic::GetInstance()->GetSwapchain()->Present(1, 0);
	WaitForPreviousFrame();
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
	//修改资源格式为dsv
	int32_t now_render_num = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	SubMemoryPointer sub_res_dsv;
	int64_t per_memory_size;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(Default_depthstencil_buffer[now_render_num], sub_res_dsv);
	auto memory_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_dsv, per_memory_size);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_data->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	//获取深度缓冲区
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(Default_depthstencil_view[now_render_num], dsvHandle);

	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH| D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
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
	//设置描述符堆
	ID3D12DescriptorHeap *heap_pointer;
	heap_pointer = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(table_offset[0].resource_view_pack_id.descriptor_heap_type_id).Get();
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &heap_pointer);
	//设置描述符堆的偏移
	for (int i = 0; i < 3; ++i)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
		auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(table_offset[i], srvHandle);
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(i, srvHandle);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ComPtr<ID3D12Resource> screen_rendertarget = PancyDx12DeviceBasic::GetInstance()->GetBackBuffer(rtvHandle);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	
	//修改资源格式为dsv
	int32_t now_render_num = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	SubMemoryPointer sub_res_dsv;
	int64_t per_memory_size;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(Default_depthstencil_buffer[now_render_num], sub_res_dsv);
	auto memory_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_dsv, per_memory_size);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_data->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	//获取深度缓冲区
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
void scene_test_simple::PopulateCommandListModelDeal()
{
	PancystarEngine::EngineFailReason check_error;

	PancyRenderCommandList *m_commandList;
	PancyModelAssimp *render_object = dynamic_cast<PancyModelAssimp*>(model_deal);
	auto pso_data = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_pbr.json");
	PancyThreadIdGPU commdlist_id_use;
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(pso_data->GetData(), &m_commandList, commdlist_id_use);
	renderlist_ID.push_back(commdlist_id_use);
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect);
	auto rootsignature_data = pso_data->GetRootSignature()->GetResource();
	m_commandList->GetCommandList()->SetGraphicsRootSignature(rootsignature_data.Get());
	//设置描述符堆
	ID3D12DescriptorHeap *heap_pointer;
	heap_pointer = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(table_offset_model[0].resource_view_pack_id.descriptor_heap_type_id).Get();
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &heap_pointer);
	//设置描述符堆的偏移
	for (int i = 0; i < 4; ++i)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
		auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(table_offset_model[i], srvHandle);
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(i, srvHandle);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ComPtr<ID3D12Resource> screen_rendertarget = PancyDx12DeviceBasic::GetInstance()->GetBackBuffer(rtvHandle);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));


	//修改资源格式为dsv
	int32_t now_render_num = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	SubMemoryPointer sub_res_dsv;
	int64_t per_memory_size;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(Default_depthstencil_buffer[now_render_num], sub_res_dsv);
	auto memory_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_dsv, per_memory_size);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_data->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	//获取深度缓冲区
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(Default_depthstencil_view[now_render_num], dsvHandle);

	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);


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
	//设置描述符堆
	ID3D12DescriptorHeap *heap_pointer;
	heap_pointer = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(render_object->GetDescriptorHeap()[0].resource_view_pack_id.descriptor_heap_type_id).Get();
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &heap_pointer);
	//设置描述符堆的偏移
	for (int i = 0; i < 3; ++i)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
		auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(render_object->GetDescriptorHeap()[i], srvHandle);
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(i, srvHandle);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ComPtr<ID3D12Resource> screen_rendertarget = PancyDx12DeviceBasic::GetInstance()->GetBackBuffer(rtvHandle);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));


	//修改资源格式为dsv
	int32_t now_render_num = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	SubMemoryPointer sub_res_dsv;
	int64_t per_memory_size;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(Default_depthstencil_buffer[now_render_num], sub_res_dsv);
	auto memory_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_dsv, per_memory_size);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_data->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	//获取深度缓冲区
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
*/
void scene_test_simple::WaitForPreviousFrame()
{
	auto  check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->WaitGpuBrokenFence(broken_fence_id);
}
void scene_test_simple::Update(float delta_time)
{
	PancystarEngine::EngineFailReason check_error;
	updateinput(delta_time);
	DirectX::XMFLOAT4X4 world_mat,uv_mat;
	DirectX::XMStoreFloat4x4(&uv_mat,DirectX::XMMatrixIdentity());
	DirectX::XMStoreFloat4x4(&world_mat,DirectX::XMMatrixIdentity());
	PancyModelAssimp *render_object_sky = dynamic_cast<PancyModelAssimp*>(model_sky);
	render_object_sky->update(world_mat, uv_mat, delta_time);

	DirectX::XMStoreFloat4x4(&uv_mat,  DirectX::XMMatrixScaling(1000,1000,0));
	DirectX::XMStoreFloat4x4(&world_mat, DirectX::XMMatrixScaling(100,2,100)*DirectX::XMMatrixTranslation(0, -5, 0));
	PancyModelAssimp *render_object_cube = dynamic_cast<PancyModelAssimp*>(model_cube);
	render_object_cube->update(world_mat, uv_mat, delta_time);



	int64_t per_memory_size;
	auto data_submemory = SubresourceControl::GetInstance()->GetResourceData(cbuffer[0], per_memory_size);
	DirectX::XMFLOAT4X4 sky_world_mat[4];
	DirectX::XMFLOAT4X4 view_mat, inv_view_mat;
	PancyCamera::GetInstance()->CountViewMatrix(&view_mat);
	PancyCamera::GetInstance()->CountInvviewMatrix(&inv_view_mat);
	//填充天空cbuffer
	DirectX::XMMATRIX proj_mat = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 1000.0f);
	DirectX::XMStoreFloat4x4(&sky_world_mat[0], DirectX::XMMatrixTranspose(DirectX::XMMatrixScaling(100,100,100)));
	DirectX::XMStoreFloat4x4(&sky_world_mat[1], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&sky_world_mat[0]) * DirectX::XMLoadFloat4x4(&view_mat) * proj_mat));
	DirectX::XMStoreFloat4x4(&sky_world_mat[2], DirectX::XMMatrixIdentity());
	DirectX::XMVECTOR x_delta;
	DirectX::XMStoreFloat4x4(&sky_world_mat[3], DirectX::XMMatrixInverse(&x_delta, DirectX::XMLoadFloat4x4(&sky_world_mat[0])));
	check_error = data_submemory->WriteFromCpuToBuffer(cbuffer[0].offset* per_memory_size, sky_world_mat, sizeof(sky_world_mat));
	//填充处理模型的cbuffer
	data_submemory = SubresourceControl::GetInstance()->GetResourceData(cbuffer_model[0], per_memory_size);
	DirectX::XMFLOAT4X4 pbr_world_mat[4];
	DirectX::XMStoreFloat4x4(&pbr_world_mat[0], DirectX::XMMatrixTranspose(DirectX::XMMatrixScaling(1, 1, 1)));
	DirectX::XMStoreFloat4x4(&pbr_world_mat[1], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&pbr_world_mat[0]) * DirectX::XMLoadFloat4x4(&view_mat) * proj_mat));
	DirectX::XMStoreFloat4x4(&pbr_world_mat[2], DirectX::XMMatrixIdentity());
	DirectX::XMStoreFloat4x4(&pbr_world_mat[3], DirectX::XMMatrixInverse(&x_delta, DirectX::XMLoadFloat4x4(&pbr_world_mat[0])));
	check_error = data_submemory->WriteFromCpuToBuffer(cbuffer_model[0].offset* per_memory_size, pbr_world_mat, sizeof(pbr_world_mat));

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
}
scene_test_simple::~scene_test_simple()
{
	WaitForPreviousFrame();
	delete test_model;
	delete model_cube;
	delete model_sky;
	delete model_deal;
}