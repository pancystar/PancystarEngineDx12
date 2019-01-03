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
	model_boundbox = NULL;
	model_size.max_box_pos.x = -999999999.0f;
	model_size.max_box_pos.y = -999999999.0f;
	model_size.max_box_pos.z = -999999999.0f;

	model_size.min_box_pos.x = 999999999.0f;
	model_size.min_box_pos.y = 999999999.0f;
	model_size.min_box_pos.z = 999999999.0f;
}
PancyModelAssimp::~PancyModelAssimp()
{
	for (int i = 0; i < cbuffer.size(); ++i)
	{
		SubresourceControl::GetInstance()->FreeSubResource(cbuffer[i]);
	}
	//todo:删除描述符
	PancyDescriptorHeapControl::GetInstance()->FreeResourceView(table_offset[0].resource_view_pack_id);
	delete model_boundbox;
}
PancystarEngine::EngineFailReason PancyModelAssimp::BuildTextureRes(std::string tex_name, const int &if_force_srgb, pancy_object_id &id_tex)
{
	PancystarEngine::EngineFailReason check_error;
	std::string texture_file_name;
	texture_file_name = tex_name;
	if (tex_name.length() > 5 && tex_name[tex_name.length() - 1] == 'n' && tex_name[tex_name.length() - 2] == 'o' && tex_name[tex_name.length() - 3] == 's' && tex_name[tex_name.length() - 4] == 'j' && tex_name[tex_name.length() - 5] == '.')
	{
	}
	else
	{
		int32_t length_real = tex_name.length();
		for (int i = tex_name.length() - 1; i >= 0; --i)
		{
			length_real -= 1;
			if (tex_name[i] == '.')
			{
				break;
			}
		}
		string json_file_out = model_root_path + texture_file_name.substr(0, length_real) + ".json";
		if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(json_file_out))
		{
			//为非json纹理创建一个纹理格式符
			Json::Value json_data_out;
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfFromFile", 1);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "FileName", texture_file_name);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfAutoBuildMipMap", 0);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfForceSrgb", if_force_srgb);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "MaxSize", 0);
			check_error = PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, json_file_out);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			//将文件标记为已经创建
			PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(json_file_out);
		}
		texture_file_name = texture_file_name.substr(0, length_real) + ".json";
	}
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(model_root_path + texture_file_name, id_tex);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyModelAssimp::LoadModel(
	const std::string &resource_desc_file,
	std::vector<PancySubModel*> &model_resource,
	std::unordered_map<pancy_object_id, std::unordered_map<TexType, pancy_object_id>> &material_list,
	std::vector<pancy_object_id> &texture_use
) {
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
	std::unordered_map<pancy_object_id, pancy_object_id> real_material_list;//舍弃不合理材质后的材质编号与之前的编号对比
	int32_t real_material_num = 0;
	for (unsigned int i = 0; i < model_need->mNumMaterials; ++i)
	{
		bool chekc_material = false;//检验是否是无效材质
		std::unordered_map<TexType, pancy_object_id> mat_tex_list;
		const aiMaterial* pMaterial = model_need->mMaterials[i];
		aiString Path;
		//漫反射纹理
		if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0 && pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
		{
			chekc_material = true;
			pancy_object_id id_need;
			check_error = BuildTextureRes(Path.C_Str(), 0, id_need);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			//根据漫反射纹理的位置标记材质的名称
			int32_t length_mat = Path.length;
			for (int i = Path.length - 1; i >= 0; --i)
			{
				length_mat -= 1;
				if (Path.C_Str()[i] == '_')
				{
					break;
				}
			}
			std::string texture_file_name = Path.C_Str();
			string mat_file_root_name = model_root_path + texture_file_name.substr(0, length_mat);
			material_name_list.insert(std::pair<pancy_object_id, std::string>(real_material_num, mat_file_root_name));
			//将纹理数据加载到材质表
			mat_tex_list.insert(std::pair<TexType, pancy_object_id>(TexType::tex_diffuse, texture_use.size()));
			texture_use.push_back(id_need);
		}
		//舍弃不含基础颜色贴图的无效材质
		if (chekc_material)
		{
			//法线纹理
			if (pMaterial->GetTextureCount(aiTextureType_HEIGHT) > 0 && pMaterial->GetTexture(aiTextureType_HEIGHT, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
			{
				pancy_object_id id_need;
				std::string texture_file_name;
				check_error = BuildTextureRes(Path.C_Str(), 0, id_need);
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
				check_error = BuildTextureRes(Path.C_Str(), 0, id_need);
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
				check_error = BuildTextureRes(Path.C_Str(), 0, id_need);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
				//将纹理数据加载到材质表
				mat_tex_list.insert(std::pair<TexType, pancy_object_id>(TexType::tex_ambient, texture_use.size()));
				texture_use.push_back(id_need);
			}
			material_list.insert(std::pair<pancy_object_id, std::unordered_map<TexType, pancy_object_id>>(real_material_num, mat_tex_list));
			real_material_list.insert(std::pair<pancy_object_id, pancy_object_id>(i, real_material_num));
			real_material_num += 1;
		}
	}
	for (int i = 0; i < model_need->mNumMeshes; i++)
	{
		//获取模型的第i个模块
		const aiMesh* paiMesh = model_need->mMeshes[i];
		//获取模型的材质编号
		auto real_material_find = real_material_list.find(paiMesh->mMaterialIndex);
		if (real_material_find == real_material_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "the material id: " + std::to_string(paiMesh->mMaterialIndex) + " of model " + resource_desc_file + " have been delete(do not have diffuse map)");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load model from Assimp", error_message);
			return error_message;
		}
		pancy_object_id material_use = real_material_find->second;
		auto mat_list_now = material_list.find(material_use);
		if (mat_list_now == material_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "havn't load the material id: " + std::to_string(material_use) + "in model:" + resource_desc_file);
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
				//更新AABB包围盒
				if (point_need[j].position.x > model_size.max_box_pos.x)
				{
					model_size.max_box_pos.x = point_need[j].position.x;
				}
				if (point_need[j].position.x < model_size.min_box_pos.x)
				{
					model_size.min_box_pos.x = point_need[j].position.x;
				}
				if (point_need[j].position.y > model_size.max_box_pos.y)
				{
					model_size.max_box_pos.y = point_need[j].position.y;
				}
				if (point_need[j].position.y < model_size.min_box_pos.y)
				{
					model_size.min_box_pos.y = point_need[j].position.y;
				}
				if (point_need[j].position.z > model_size.max_box_pos.z)
				{
					model_size.max_box_pos.z = point_need[j].position.z;
				}
				if (point_need[j].position.z < model_size.min_box_pos.z)
				{
					model_size.min_box_pos.z = point_need[j].position.z;
				}
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
				point_need[j].tex_id.x = material_use;
				point_need[j].tex_id.y = mat_list_now->second.size() + 2;//金属度及粗糙度
			}
			PancySubModel *new_submodel = new PancySubModel();
			check_error = new_submodel->Create(point_need, index_need, paiMesh->mNumVertices, paiMesh->mNumFaces * 3, material_use);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			model_resource.push_back(new_submodel);
			delete[] point_need;
		}
		delete[] index_need;
	}
	//创建包围盒顶点
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
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
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
		new_res_view.resource_view_offset_id = descriptor_use_data[0].table_offset[2] + i;
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
void PancyModelAssimp::update(DirectX::XMFLOAT4X4 world_matrix, DirectX::XMFLOAT4X4 uv_matrix, float delta_time)
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
	//创建两个离屏缓冲区，用于渲染以及数据回读
	std::vector<D3D12_HEAP_FLAGS> heap_flags;
	//创建屏幕空间uint4渲染目标格式
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
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	std::string RGB8uint_file_data = "screentarget\\screen_" + std::to_string(Scene_width) + "_" + std::to_string(Scene_height) + "_RGBA8UINT.json";
	if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(RGB8uint_file_data))
	{
		Json::Value json_data_out;
		//填充资源格式
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfFromFile", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFSRV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfRTV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFUAV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFDSV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "SubResourceFile", subres_name);
		//写入文件并标记为已创建
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, RGB8uint_file_data);
		//将文件标记为已经创建
		PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(RGB8uint_file_data);
	}
	//创建屏幕空间readback缓冲区
	uint64_t subresources_size;
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetCopyableFootprints(&uint_tex_desc, 0, 1, 0, nullptr, nullptr, nullptr, &subresources_size);
	if (subresources_size % 65536 != 0)
	{
		//65536对齐
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
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	std::string readback_data = "screentarget\\screen_" + std::to_string(Scene_width) + "_" + std::to_string(Scene_height) + "_RGBA8_readback.json";
	if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(readback_data))
	{
		Json::Value json_data_out;
		//填充资源格式
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfFromFile", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFSRV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfRTV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFUAV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFDSV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "SubResourceFile", subres_name);
		//写入文件并标记为已创建
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, readback_data);
		//将文件标记为已经创建
		PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(readback_data);
	}
	for (int i = 0; i < 2; ++i)
	{
		//根据新生成的格式创建两个离屏缓冲区
		if (if_readback_build)
		{
			//之前已经生成了离屏数据，删除之前的备份
			check_error = PancystarEngine::PancyTextureControl::GetInstance()->DeleteResurceReference(tex_uint_save[i]);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			check_error = PancystarEngine::PancyTextureControl::GetInstance()->DeleteResurceReference(read_back_buffer[i]);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			if_readback_build = false;
		}
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(RGB8uint_file_data, tex_uint_save[i]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(readback_data, read_back_buffer[i]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//创建一个额外的深度缓冲区
		std::string depth_stencil_use = "screentarget\\screen_" + std::to_string(Scene_width) + "_" + std::to_string(Scene_height) + "_DSV.json";
		auto check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(depth_stencil_use, depth_stencil_mask[i]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//创建深度模板缓冲区描述符
		SubMemoryPointer tex_resource_data;
		D3D12_DEPTH_STENCIL_VIEW_DESC DSV_desc;
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(depth_stencil_mask[i], tex_resource_data);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->GetDSVDesc(depth_stencil_mask[i], DSV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		std::string dsv_descriptor_name = "json\\descriptor_heap\\DSV_1_descriptor_heap.json";
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(dsv_descriptor_name, dsv_mask[i].resource_view_pack_id);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		dsv_mask[i].resource_view_offset_id = 0;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildDSV(dsv_mask[i], tex_resource_data, DSV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//创建渲染目标
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_uint_save[i], tex_resource_data);
		D3D12_RENDER_TARGET_VIEW_DESC RTV_desc;
		PancystarEngine::PancyTextureControl::GetInstance()->GetRTVDesc(tex_uint_save[i], RTV_desc);
		RTV_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		std::string rtv_descriptor_name = "json\\descriptor_heap\\RTV_1_descriptor_heap.json";
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(rtv_descriptor_name, rtv_mask[i].resource_view_pack_id);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		rtv_mask[i].resource_view_offset_id = 0;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildRTV(rtv_mask[i], tex_resource_data, RTV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//创建渲染回读数据的拷贝格式
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
	table_offset_model[3].resource_view_pack_id = globel_var;
	table_offset_model[3].resource_view_offset_id = descriptor_use_data[0].table_offset[3];
	/*
	//模型自身的纹理
	new_rvp = table_offset_model[3];
	for (int i = 0; i < model_deal->GetSubModelNum(); ++i)
	{
		pancy_object_id now_tex_id = model_deal->GetSubModelTexture(i, TexType::tex_diffuse);
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
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_metallic_id[i], texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_metallic_id[i], SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_rvp.resource_view_offset_id += 1;
		//粗糙度纹理
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_roughness_id[i], texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_roughness_id[i], SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	*/
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::UpdatePbrDescriptor()
{
	PancystarEngine::EngineFailReason check_error;
	ResourceViewPointer new_rvp = table_offset_model[3];
	SubMemoryPointer texture_need;
	D3D12_SHADER_RESOURCE_VIEW_DESC SRV_desc;
	for (int i = 0; i < model_deal->GetMaterialNum(); ++i)
	{
		pancy_object_id now_tex_id;
		//漫反射纹理
		check_error = model_deal->GetMateriaTexture(i, TexType::tex_diffuse, now_tex_id);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(now_tex_id, texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(now_tex_id, SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_rvp.resource_view_offset_id += 1;
		//法线纹理
		check_error = model_deal->GetMateriaTexture(i, TexType::tex_normal, now_tex_id);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(now_tex_id, texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(now_tex_id, SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_rvp.resource_view_offset_id += 1;
		//金属度纹理
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_metallic_id[i], texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_metallic_id[i], SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_rvp.resource_view_offset_id += 1;
		//粗糙度纹理
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_roughness_id[i], texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_roughness_id[i], SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_rvp.resource_view_offset_id += 1;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::LoadDealModel(std::string file_name)
{
	PancystarEngine::EngineFailReason check_error;
	if (if_load_model)
	{
		//有已经加载的模型，先删除模型的备份
		delete model_deal;
		model_deal = NULL;
		for (int i = 0; i < tex_metallic_id.size(); ++i)
		{
			if (tex_metallic_id[i] != pic_empty_white_id)
			{
				PancystarEngine::PancyTextureControl::GetInstance()->DeleteResurceReference(tex_metallic_id[i]);
			}
		}
		tex_metallic_id.clear();
		for (int i = 0; i < tex_roughness_id.size(); ++i)
		{
			if (tex_roughness_id[i] != pic_empty_white_id)
			{
				PancystarEngine::PancyTextureControl::GetInstance()->DeleteResurceReference(tex_roughness_id[i]);
			}
		}
		tex_roughness_id.clear();
		if_load_model = false;
	}
	//加载模型
	model_deal = new PancyModelAssimp(file_name, "json\\pipline_state_object\\pso_pbr.json");
	check_error = model_deal->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//预加载金属度/粗糙度数据
	for (int i = 0; i < model_deal->GetMaterialNum(); ++i)
	{
		auto assimp_pointer = dynamic_cast<PancyModelAssimp*>(model_deal);
		std::string material_name = assimp_pointer->GetMaterialName(i);
		std::string metallic_pre_name = material_name + "_Metallic";
		std::string roughness_pre_name = material_name + "_Roughness";
		std::ifstream file_check;
		//先检验金属度纹理
		file_check.open(metallic_pre_name + ".dds");
		if (file_check.is_open())
		{
			file_check.close();
			//检验成功，为纹理创建json文件
			std::string json_file_metallic = metallic_pre_name + ".json";
			if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(json_file_metallic))
			{
				std::string dds_metallic_name = metallic_pre_name + ".dds";
				int32_t copy_length = 0;
				for (int i = dds_metallic_name.size() - 1; i >= 0; --i)
				{
					if (dds_metallic_name[i] == '\\' || dds_metallic_name[i] == '/')
					{
						break;
					}
					else
					{
						copy_length += 1;
					}
				}
				dds_metallic_name = dds_metallic_name.substr(dds_metallic_name.size() - copy_length, copy_length);
				//为非json纹理创建一个纹理格式符
				Json::Value json_data_out;
				PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfFromFile", 1);
				PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "FileName", dds_metallic_name);
				PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfAutoBuildMipMap", 0);
				PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfForceSrgb", 0);
				PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "MaxSize", 0);
				check_error = PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, json_file_metallic);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
				//将文件标记为已经创建
				PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(json_file_metallic);
			}
			//json格式创建完毕，创建纹理资源
			pancy_object_id texture_id_metallic;
			check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(json_file_metallic, texture_id_metallic);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			tex_metallic_id.push_back(texture_id_metallic);
		}
		else
		{
			tex_metallic_id.push_back(pic_empty_white_id);
		}
		//检验粗糙度纹理
		file_check.open(roughness_pre_name + ".dds");
		if (file_check.is_open())
		{
			file_check.close();
			//检验成功，为纹理创建json文件
			std::string json_file_roughness = roughness_pre_name + ".json";
			if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(json_file_roughness))
			{
				std::string dds_roughness_name = roughness_pre_name + ".dds";
				int32_t copy_length = 0;
				for (int i = dds_roughness_name.size() - 1; i >= 0; --i)
				{
					if (dds_roughness_name[i] == '\\' || dds_roughness_name[i] == '/')
					{
						break;
					}
					else
					{
						copy_length += 1;
					}
				}
				dds_roughness_name = dds_roughness_name.substr(dds_roughness_name.size() - copy_length, copy_length);
				//为非json纹理创建一个纹理格式符
				Json::Value json_data_out;
				PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfFromFile", 1);
				PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "FileName", dds_roughness_name);
				PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfAutoBuildMipMap", 0);
				PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfForceSrgb", 0);
				PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "MaxSize", 0);
				check_error = PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, json_file_roughness);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
				//将文件标记为已经创建
				PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(json_file_roughness);
			}
			//json格式创建完毕，创建纹理资源
			pancy_object_id texture_id_roughness;
			check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(json_file_roughness, texture_id_roughness);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			tex_roughness_id.push_back(texture_id_roughness);
		}
		else
		{
			tex_roughness_id.push_back(pic_empty_white_id);
		}
		//tex_metallic_id.push_back(pic_empty_white_id);
		//tex_roughness_id.push_back(pic_empty_white_id);
	}
	//更新模型的纹理数据到descriptor_heap
	check_error = UpdatePbrDescriptor();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if_load_model = true;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::Init()
{
	PancystarEngine::EngineFailReason check_error;
	//创建临时的d3d11设备用于纹理压缩

	UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, 0, 0, D3D11_SDK_VERSION, &device_pancy, &featureLevel, &contex_pancy);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "D3D11CreateDevice Failed.");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Scene", error_message);
		return error_message;
	}
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
	UINT index[] = { 0,1,2 ,0,2,3 };
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
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_screenmask.json");
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_boundbox.json");
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	/*
	model_deal = new PancyModelAssimp("model\\ball2\\ball.obj", "json\\pipline_state_object\\pso_pbr.json");
	check_error = model_deal->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	*/
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
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\white.json", pic_empty_white_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	/*
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\Sphere002_roughness.json", tex_roughness_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	*/
	//为pbr模型的渲染创建descriptor
	check_error = PretreatPbrDescriptor();
	if (!check_error.CheckIfSucceed())
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
		PopulateCommandListModelDealBound();
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
	//回读GPU数据
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
	screen_rendertarget->SetName(PancystarEngine::PancyString("back_buffer" + std::to_string(now_render_num)).GetUnicodeString().c_str());
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
	//设置描述符堆
	ID3D12DescriptorHeap *heap_pointer;
	heap_pointer = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(table_offset_model[0].resource_view_pack_id.descriptor_heap_type_id).Get();
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &heap_pointer);
	//设置描述符堆的偏移
	for (int i = 0; i < 2; ++i)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
		auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(table_offset_model[i], srvHandle);
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(i, srvHandle);
	}
	//bindless texture
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
	auto heap_offset_bindless = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(table_offset_model[3], srvHandle);
	m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(2, srvHandle);
	//渲染目标
	int64_t per_memory_size;
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	SubMemoryPointer sub_res_rtv;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_uint_save[now_render_num], sub_res_rtv);
	auto memory_rtv_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_rtv, per_memory_size);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_rtv_data->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(rtv_mask[now_render_num], rtvHandle);
	//修改资源格式为dsv

	SubMemoryPointer sub_res_dsv;

	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(depth_stencil_mask[now_render_num], sub_res_dsv);
	auto memory_dsv_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_dsv, per_memory_size);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_dsv_data->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	//获取深度缓冲区
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
	//设置描述符堆
	ID3D12DescriptorHeap *heap_pointer;
	heap_pointer = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(table_offset_model[0].resource_view_pack_id.descriptor_heap_type_id).Get();
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &heap_pointer);
	//设置描述符堆的偏移
	for (int i = 0; i < 2; ++i)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
		auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(table_offset_model[i], srvHandle);
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(i, srvHandle);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ComPtr<ID3D12Resource> screen_rendertarget = PancyDx12DeviceBasic::GetInstance()->GetBackBuffer(rtvHandle);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));


	//修改资源格式为dsv

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
	//填充天空cbuffer
	DirectX::XMMATRIX proj_mat = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 1000.0f);
	DirectX::XMStoreFloat4x4(&sky_world_mat[0], DirectX::XMMatrixTranspose(DirectX::XMMatrixScaling(100, 100, 100)));
	DirectX::XMStoreFloat4x4(&sky_world_mat[1], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&sky_world_mat[0]) * DirectX::XMLoadFloat4x4(&view_mat) * proj_mat));
	DirectX::XMStoreFloat4x4(&sky_world_mat[2], DirectX::XMMatrixIdentity());
	DirectX::XMVECTOR x_delta;
	DirectX::XMStoreFloat4x4(&sky_world_mat[3], DirectX::XMMatrixInverse(&x_delta, DirectX::XMLoadFloat4x4(&sky_world_mat[0])));
	check_error = data_submemory->WriteFromCpuToBuffer(cbuffer[0].offset* per_memory_size, sky_world_mat, sizeof(sky_world_mat));
	//填充处理模型的cbuffer
	data_submemory = SubresourceControl::GetInstance()->GetResourceData(cbuffer_model[0], per_memory_size);
	DirectX::XMFLOAT4X4 pbr_world_mat[4];
	DirectX::XMMATRIX pbr_pre_world_mat = DirectX::XMMatrixScaling(scale_size, scale_size, scale_size) * DirectX::XMMatrixRotationX((DirectX::XM_PI * rotation_angle.x) / 180.0f) * DirectX::XMMatrixRotationY((DirectX::XM_PI * rotation_angle.y) / 180.0f) *  DirectX::XMMatrixRotationZ((DirectX::XM_PI * rotation_angle.z) / 180.0f) * DirectX::XMMatrixTranslation(translation_pos.x, translation_pos.y, translation_pos.z);
	DirectX::XMStoreFloat4x4(&pbr_world_mat[0], DirectX::XMMatrixTranspose(pbr_pre_world_mat));
	DirectX::XMStoreFloat4x4(&pbr_world_mat[1], DirectX::XMMatrixTranspose(pbr_pre_world_mat * DirectX::XMLoadFloat4x4(&view_mat) * proj_mat));
	DirectX::XMStoreFloat4x4(&pbr_world_mat[2], DirectX::XMMatrixIdentity());
	//先计算3*3矩阵的逆转置
	DirectX::XMMATRIX normal_need = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(&x_delta, pbr_pre_world_mat));
	DirectX::XMFLOAT4X4 mat_normal;
	DirectX::XMStoreFloat4x4(&mat_normal, normal_need);
	mat_normal._41 = 0.0f;
	mat_normal._42 = 0.0f;
	mat_normal._43 = 0.0f;
	mat_normal._44 = 0.0f;
	DirectX::XMStoreFloat4x4(&pbr_world_mat[3], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mat_normal)));
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
	if (if_load_model)
	{
		delete model_deal;
	}
	device_pancy->Release();
	contex_pancy->Release();
}