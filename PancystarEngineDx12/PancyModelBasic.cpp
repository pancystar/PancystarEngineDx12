#include"PancyModelBasic.h"
using namespace PancystarEngine;
//模型部件
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
//模型类
PancyBasicModel::PancyBasicModel(const std::string &resource_name, const Json::Value &root_value) : PancyBasicVirtualResource(resource_name, root_value)
{
	//根据交换帧的数量决定创建多少组渲染描述符链
	pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	descriptor_map.resize(Frame_num);
}
PancyBasicModel::~PancyBasicModel()
{
	if (if_skinmesh)
	{
		FreeBoneTree(root_skin);
	}
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
PancystarEngine::EngineFailReason PancyBasicModel::LoadSkinTree(string filename)
{
	instream.open(filename, ios::binary);
	if (!instream.is_open())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "open file " + filename + " error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Model From File", error_message);
		return error_message;
	}
	//读取偏移矩阵
	int bone_num_need;
	instream.read(reinterpret_cast<char*>(&bone_num), sizeof(bone_num));
	instream.read(reinterpret_cast<char*>(offset_matrix_array), bone_num * sizeof(DirectX::XMFLOAT4X4));
	//先读取第一个入栈符
	char data[11];
	instream.read(reinterpret_cast<char*>(data), sizeof(data));
	root_skin = new skin_tree();
	//递归重建骨骼树
	ReadBoneTree(root_skin);
	//关闭文件
	instream.close();
	return PancystarEngine::succeed;
}
void PancyBasicModel::ReadBoneTree(skin_tree *now)
{
	char data[11];
	instream.read(reinterpret_cast<char*>(now), sizeof(*now));
	now->brother = NULL;
	now->son = NULL;
	instream.read(data, sizeof(data));
	while (strcmp(data, "*heaphead*") == 0)
	{
		//入栈符号，代表子节点
		skin_tree *now_point = new skin_tree();
		ReadBoneTree(now_point);
		now_point->brother = now->son;
		now->son = now_point;
		instream.read(data, sizeof(data));
	}

}
void PancyBasicModel::FreeBoneTree(skin_tree *now)
{
	if (now->brother != NULL)
	{
		FreeBoneTree(now->brother);
	}
	if (now->son != NULL)
	{
		FreeBoneTree(now->son);
	}
	if (now != NULL)
	{
		free(now);
	}
}
PancystarEngine::EngineFailReason PancyBasicModel::InitResource(const Json::Value &root_value, const std::string &resource_name, ResourceStateType &now_res_state)
{
	PancystarEngine::EngineFailReason check_error;
	std::string path_name = "";
	std::string file_name = "";
	std::string tile_name = "";
	DivideFilePath(resource_name, path_name, file_name, tile_name);
	pancy_json_value rec_value;
	/*
	Json::Value root_value;
	check_error = PancyJsonTool::GetInstance()->LoadJsonFile(resource_desc_file, root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	*/
	//是否包含骨骼动画
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, root_value, "IfHaveSkinAnimation", pancy_json_data_type::json_data_bool, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if_skinmesh = rec_value.bool_value;
	//是否包含顶点动画
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, root_value, "IfHavePoinAnimation", pancy_json_data_type::json_data_bool, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if_pointmesh = rec_value.bool_value;
	//模型的pbr类型
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, root_value, "PbrType", pancy_json_data_type::json_data_enum, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	model_pbr_type = static_cast<PbrMaterialType>(rec_value.int_value);
	//读取模型的网格数据
	int32_t model_part_num;
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, root_value, "model_num", pancy_json_data_type::json_data_int, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	model_part_num = rec_value.int_value;
	for (int i = 0; i < model_part_num; ++i)
	{
		std::string model_vertex_data_name = path_name + file_name + std::to_string(i) + ".vertex";
		std::string model_index_data_name = path_name + file_name + std::to_string(i) + ".index";
		if (if_skinmesh)
		{
			check_error = LoadMeshData<PancystarEngine::PointSkinCommon8>(model_vertex_data_name, model_index_data_name);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		else if (if_pointmesh)
		{
			check_error = LoadMeshData<PancystarEngine::PointCatchCommon>(model_vertex_data_name, model_index_data_name);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		else
		{
			check_error = LoadMeshData<PancystarEngine::PointCommon>(model_vertex_data_name, model_index_data_name);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
	}
	//读取模型的纹理数据
	int32_t model_texture_num;
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, root_value, "texture_num", pancy_json_data_type::json_data_int, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	model_texture_num = rec_value.int_value;
	for (int i = 0; i < model_texture_num; ++i)
	{
		std::string texture_name_now = path_name + file_name + "_tex" + std::to_string(i) + ".json";
		pancy_object_id texture_id;
		//加载纹理并添加引用计数
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(texture_name_now, texture_id);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		/*
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->AddResurceReference(texture_id);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		*/
		texture_list.push_back(texture_id);
	}
	//读取模型的材质数据
	Json::Value material_value = root_value.get("material", Json::Value::null);
	for (int i = 0; i < material_value.size(); ++i)
	{
		std::unordered_map<TexType, pancy_object_id> now_material_need;
		int32_t material_id;
		//材质id
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "materialID", pancy_json_data_type::json_data_int, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		material_id = rec_value.int_value;
		//漫反射纹理
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "Albedotex", pancy_json_data_type::json_data_int, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_diffuse, rec_value.int_value));
		//法线纹理
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "Normaltex", pancy_json_data_type::json_data_int, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_normal, rec_value.int_value));
		//AO纹理
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "Ambienttex", pancy_json_data_type::json_data_int, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_ambient, rec_value.int_value));
		//PBR纹理
		if (model_pbr_type == PbrMaterialType::PbrType_MetallicRoughness)
		{
			//金属度纹理
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "MetallicTex", pancy_json_data_type::json_data_int, rec_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_metallic, rec_value.int_value));
			//粗糙度纹理
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "RoughnessTex", pancy_json_data_type::json_data_int, rec_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_roughness, rec_value.int_value));
		}
		else if (model_pbr_type == PbrMaterialType::PbrType_SpecularSmoothness)
		{
			//镜面光&平滑度纹理
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "SpecularSmoothTex", pancy_json_data_type::json_data_int, rec_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_specular_smoothness, rec_value.int_value));
		}
		material_list.insert(std::pair<pancy_object_id, std::unordered_map<TexType, pancy_object_id>>(material_id, now_material_need));
	}
	//读取骨骼动画
	if (if_skinmesh)
	{
		std::string bone_data_name = path_name + file_name + ".bone";
		//读取骨骼信息
		check_error = LoadSkinTree(bone_data_name);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//读取动画信息
		Json::Value skin_animation_value = root_value.get("SkinAnimation", Json::Value::null);
		for (int i = 0; i < skin_animation_value.size(); ++i)
		{
			animation_set new_animation;
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, skin_animation_value, i, pancy_json_data_type::json_data_string, rec_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			std::string now_animation_name = path_name + rec_value.string_value;
			instream.open(now_animation_name, ios::binary);
			int32_t animation_bone_num;
			instream.read(reinterpret_cast<char*>(&animation_bone_num), sizeof(animation_bone_num));
			for (int l = 0; l < animation_bone_num; ++l)
			{
				animation_data new_bone_data;
				//骨骼信息
				int32_t bone_name_size = -1;
				instream.read(reinterpret_cast<char*>(&bone_name_size), sizeof(bone_name_size));
				char *name = new char[bone_name_size];
				instream.read(name, bone_name_size * sizeof(char));
				new_bone_data.bone_name += name;
				delete[] name;
				//旋转向量
				int32_t rotation_key_num = 0;
				instream.read(reinterpret_cast<char*>(&rotation_key_num), sizeof(rotation_key_num));
				quaternion_animation *new_rotation_key = new quaternion_animation[rotation_key_num];
				int32_t rotation_key_size = sizeof(new_rotation_key[0]) * rotation_key_num;

				instream.read(reinterpret_cast<char*>(new_rotation_key), rotation_key_size);
				for (int j = 0; j < rotation_key_num; ++j)
				{
					new_bone_data.rotation_key.push_back(new_rotation_key[j]);
				}
				//平移向量
				int32_t translation_key_num = 0;
				instream.read(reinterpret_cast<char*>(&translation_key_num), sizeof(translation_key_num));
				vector_animation *new_translation_key = new vector_animation[translation_key_num];
				int32_t translation_key_size = sizeof(new_translation_key[0]) * translation_key_num;
				instream.read(reinterpret_cast<char*>(new_translation_key), translation_key_size);
				for (int j = 0; j < translation_key_num; ++j)
				{
					new_bone_data.translation_key.push_back(new_translation_key[j]);
				}

				//缩放向量
				int32_t scaling_key_num = 0;
				instream.read(reinterpret_cast<char*>(&scaling_key_num), sizeof(scaling_key_num));
				vector_animation *new_scaling_key = new vector_animation[scaling_key_num];
				int32_t scaling_key_size = sizeof(new_scaling_key[0]) * scaling_key_num;
				instream.read(reinterpret_cast<char*>(new_scaling_key), scaling_key_size);
				for (int j = 0; j < scaling_key_num; ++j)
				{
					new_bone_data.scaling_key.push_back(new_scaling_key[j]);
				}
				new_animation.data_animition.push_back(new_bone_data);
				//删除临时变量
				delete[] new_rotation_key;
				delete[] new_translation_key;
				delete[] new_scaling_key;
			}
			//将动画信息加入表单
			skin_animation_name.insert(std::pair<std::string, pancy_resource_id>(now_animation_name, i));
			skin_animation_map.insert(std::pair<pancy_resource_id, animation_set>(i, new_animation));
			instream.close();
		}

	}
	//读取顶点动画
	if (if_pointmesh)
	{
		Json::Value point_animation_value = root_value.get("PointAnimation", Json::Value::null);
		for (int i = 0; i < point_animation_value.size(); ++i)
		{
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, point_animation_value, i, pancy_json_data_type::json_data_string, rec_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			std::string now_animation_name = path_name + rec_value.string_value;
			instream.open(now_animation_name, ios::binary);
			instream.read(reinterpret_cast<char*>(&all_frame_num), sizeof(all_frame_num));
			instream.read(reinterpret_cast<char*>(&perframe_size), sizeof(perframe_size));
			instream.read(reinterpret_cast<char*>(&buffer_size), sizeof(buffer_size));
			instream.read(reinterpret_cast<char*>(&fps_point_catch), sizeof(fps_point_catch));
			int32_t size_need = buffer_size * sizeof(mesh_animation_data);
			mesh_animation_data *new_point_catch_data = new mesh_animation_data[buffer_size];
			instream.read(reinterpret_cast<char*>(new_point_catch_data), size_need);
			instream.close();
			/*
			加载数据
			*/
			delete[] new_point_catch_data;
		}
	}
	now_res_state = ResourceStateType::resource_state_load_CPU_memory_finish;
	return PancystarEngine::succeed;
}
void PancyBasicModel::CheckIfResourceLoadToGpu(ResourceStateType &now_res_state)
{
	PancystarEngine::EngineFailReason check_error;
	if (now_res_state == ResourceStateType::resource_state_load_CPU_memory_finish)
	{
		//检测所有的纹理资源是否已经加载完毕
		for (int i = 0; i < texture_list.size(); ++i)
		{
			ResourceStateType now_texture_state;
			check_error = PancyTextureControl::GetInstance()->GetResourceState(texture_list[i], now_texture_state);
			if (!check_error.CheckIfSucceed() || now_texture_state == ResourceStateType::resource_state_not_init)
			{
				now_res_state = ResourceStateType::resource_state_not_init;
				return;
			}
			else if (now_texture_state == ResourceStateType::resource_state_load_CPU_memory_finish)
			{
				now_res_state = ResourceStateType::resource_state_load_CPU_memory_finish;
				return;
			}
		}
		//检测所有的几何体资源是否已经加载完毕
		for (int i = 0; i < model_resource_list.size(); ++i)
		{
			ResourceStateType now_texture_state;
			check_error = model_resource_list[i]->GetLoadState(now_texture_state);
			if (!check_error.CheckIfSucceed() || now_texture_state == ResourceStateType::resource_state_not_init)
			{
				now_res_state = ResourceStateType::resource_state_not_init;
				return;
			}
			else if (now_texture_state == ResourceStateType::resource_state_load_CPU_memory_finish)
			{
				now_res_state = ResourceStateType::resource_state_load_CPU_memory_finish;
				return;
			}
		}
		now_res_state = ResourceStateType::resource_state_load_GPU_memory_finish;
	}
}
PancystarEngine::EngineFailReason PancyBasicModel::GetRenderDescriptor(
	pancy_object_id PSO_id,
	const std::vector<std::string> &cbuffer_name_per_object_in,
	const std::vector<PancystarEngine::PancyConstantBuffer *> &cbuffer_per_frame_in,
	const std::vector<SubMemoryPointer> &resource_data_per_frame_in,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_in,
	DescriptorObject **descriptor_out
)
{
	pancy_object_id now_render_frame = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	PancystarEngine::EngineFailReason check_error;
	auto descriptor_list = descriptor_map[now_render_frame].find(PSO_id);
	if (descriptor_list == descriptor_map[now_render_frame].end())
	{
		//根据PSO的ID号获取PSO的名称和描述符的格式名称
		std::string PSO_name;
		std::string descriptor_name;
		check_error = PancyEffectGraphic::GetInstance()->GetPSOName(PSO_id, PSO_name);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = PancyEffectGraphic::GetInstance()->GetPSODescriptorName(PSO_id, descriptor_name);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//将所有的模型资源整理
		std::vector<SubMemoryPointer> res_pack;
		std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> SRV_pack;
		if (if_pointmesh)
		{

		}
		
		for (int i = 0; i < texture_list.size(); ++i)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC new_SRV_desc;
			SubMemoryPointer now_texture;
			PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(texture_list[i], now_texture);
			res_pack.push_back(now_texture);
			PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(texture_list[i], new_SRV_desc);
			SRV_pack.push_back(new_SRV_desc);
		}
		//创建一个新的描述符队列
		DescriptorObjectList *new_descriptor_List;
		new_descriptor_List = new DescriptorObjectList(PSO_name, descriptor_name);
		check_error = new_descriptor_List->Create(
			cbuffer_name_per_object_in,
			cbuffer_per_frame_in,
			resource_data_per_frame_in,
			resource_desc_per_frame_in,
			res_pack,
			SRV_pack
		);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		descriptor_map[now_render_frame].insert(std::pair<pancy_object_id, DescriptorObjectList*>(PSO_id, new_descriptor_List));
		descriptor_list = descriptor_map[now_render_frame].find(PSO_id);
	}
	check_error = descriptor_list->second->GetEmptyList(descriptor_out);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
//描述符类
DescriptorObject::DescriptorObject()
{
}
DescriptorObject::~DescriptorObject()
{
	PancyDescriptorHeapControl::GetInstance()->FreeResourceView(descriptor_block_id);
	for (auto release_data = per_object_cbuffer.begin(); release_data != per_object_cbuffer.end(); ++release_data)
	{
		delete release_data->second;
	}
	per_object_cbuffer.clear();
}
PancystarEngine::EngineFailReason DescriptorObject::Create(
	const std::string &PSO_name,
	const std::string &descriptor_name,
	const std::vector<std::string> &cbuffer_name_per_object,
	const std::vector<PancystarEngine::PancyConstantBuffer *> &cbuffer_per_frame,
	const std::vector<SubMemoryPointer> &resource_data_per_frame,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_in,
	const std::vector<SubMemoryPointer> &resource_data_per_object,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_object_in
)
{
	PancystarEngine::EngineFailReason check_error;
	PSO_name_descriptor = PSO_name;
	//创建一个对应类型的描述符块
	ResourceViewPointer new_point;
	pancy_object_id globel_offset = 0;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(descriptor_name, descriptor_block_id, resource_view_num);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//将渲染需要的绑定资源指针一次性全部获取并保存
	pancy_object_id PSO_id_need;
	//PSO数据
	check_error = PancyEffectGraphic::GetInstance()->GetPSO(PSO_name, PSO_id_need);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancyEffectGraphic::GetInstance()->GetPSOResource(PSO_id_need,&PSO_pointer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//rootsignature数据
	check_error = PancyEffectGraphic::GetInstance()->GetRootSignatureResource(PSO_id_need, &rootsignature);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//绑定的描述符堆数据
	check_error = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(descriptor_block_id,&descriptor_heap_use);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//绑定的描述符堆的偏移
	std::vector<pancy_object_id> descriptor_distribute;
	check_error = PancyEffectGraphic::GetInstance()->GetDescriptorDistribute(PSO_id_need, descriptor_distribute);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	pancy_object_id now_start_offset = 0;
	for (int i = 0; i < descriptor_distribute.size(); ++i) 
	{
		ResourceViewPointer distribute_point;
		distribute_point.resource_view_pack_id = descriptor_block_id;
		CD3DX12_GPU_DESCRIPTOR_HANDLE now_gpu_handle;
		distribute_point.resource_view_offset_id = now_start_offset;
		check_error = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(distribute_point, now_gpu_handle);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		descriptor_offset.push_back(now_gpu_handle);
		now_start_offset += descriptor_distribute[i];
	}
	
	//填充描述符的信息
	new_point.resource_view_pack_id = descriptor_block_id;
	//检验传入的资源数量和描述符的数量是否匹配(如果有bindless texture则不做考虑)
	pancy_object_id check_descriptor_size = cbuffer_name_per_object.size() + cbuffer_per_frame.size() + resource_data_per_frame.size() + resource_data_per_object.size();
	if (resource_data_per_object.size() == 0 && check_descriptor_size != resource_view_num)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource num: " +
			std::to_string(check_descriptor_size) +
			" dismatch resource view num: " +
			std::to_string(resource_view_num) +
			" in PSO: " + PSO_name
		);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor object", error_message);
		return error_message;
	}
	//先根据常量缓冲区的名称，绑定object独有的常量缓冲区。
	for (int i = 0; i < cbuffer_name_per_object.size(); ++i)
	{
		auto cbuffer_check = per_object_cbuffer.find(cbuffer_name_per_object[i]);
		if (cbuffer_check == per_object_cbuffer.end()) 
		{
			std::string pso_divide_path;
			std::string pso_divide_name;
			std::string pso_divide_tail;
			PancystarEngine::DivideFilePath(PSO_name, pso_divide_path, pso_divide_name, pso_divide_tail);
			PancystarEngine::PancyConstantBuffer *new_cbuffer = new PancystarEngine::PancyConstantBuffer(cbuffer_name_per_object[i], pso_divide_name);
			check_error = new_cbuffer->Create();
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			SubMemoryPointer submemory;
			check_error = new_cbuffer->GetBufferSubResource(submemory);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			new_point.resource_view_offset_id = globel_offset + i;
			check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(new_point, submemory);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			per_object_cbuffer.insert(std::pair<std::string, PancystarEngine::PancyConstantBuffer*>(cbuffer_name_per_object[i], new_cbuffer));
		}
		
	}
	globel_offset += cbuffer_name_per_object.size();
	//绑定每帧独有的常量缓冲区
	for (int i = 0; i < cbuffer_per_frame.size(); ++i)
	{
		SubMemoryPointer submemory;
		check_error = cbuffer_per_frame[i]->GetBufferSubResource(submemory);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_point.resource_view_offset_id = globel_offset + i;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(new_point, submemory);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	globel_offset += cbuffer_per_frame.size();
	//绑定每帧独有的shader资源
	for (int i = 0; i < resource_data_per_frame.size(); ++i)
	{
		new_point.resource_view_offset_id = globel_offset + i;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_point, resource_data_per_frame[i], resource_desc_per_frame_in[i]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	globel_offset += resource_data_per_frame.size();
	//绑定每个object独有的shader资源
	for (int i = 0; i < resource_data_per_object.size(); ++i)
	{
		new_point.resource_view_offset_id = globel_offset + i;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_point, resource_data_per_object[i], resource_desc_per_object_in[i]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorObject::SetCbufferMatrix(
	const std::string &cbuffer_name,
	const std::string &variable_name,
	const DirectX::XMFLOAT4X4 &data_in,
	const pancy_resource_size &offset
) 
{
	PancystarEngine::EngineFailReason check_error;
	auto cbuffer_data = per_object_cbuffer.find(cbuffer_name);
	if (cbuffer_data == per_object_cbuffer.end()) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL,"Could not find Cbuffer: "+ cbuffer_name +" in DescriptorObject of PSO: " + PSO_name_descriptor);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer Matrix",error_message);
		return error_message;
	}
	check_error = cbuffer_data->second->SetMatrix(variable_name, data_in, offset);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorObject::SetCbufferFloat4(
	const std::string &cbuffer_name,
	const std::string &variable_name,
	const DirectX::XMFLOAT4 &data_in,
	const pancy_resource_size &offset
) 
{
	PancystarEngine::EngineFailReason check_error;
	auto cbuffer_data = per_object_cbuffer.find(cbuffer_name);
	if (cbuffer_data == per_object_cbuffer.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in DescriptorObject of PSO: " + PSO_name_descriptor);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer float4", error_message);
		return error_message;
	}
	check_error = cbuffer_data->second->SetFloat4(variable_name, data_in, offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorObject::SetCbufferUint4(
	const std::string &cbuffer_name,
	const std::string &variable_name,
	const DirectX::XMUINT4 &data_in,
	const pancy_resource_size &offset
) 
{
	PancystarEngine::EngineFailReason check_error;
	auto cbuffer_data = per_object_cbuffer.find(cbuffer_name);
	if (cbuffer_data == per_object_cbuffer.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in DescriptorObject of PSO: " + PSO_name_descriptor);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer uint4", error_message);
		return error_message;
	}
	check_error = cbuffer_data->second->SetUint4(variable_name, data_in, offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorObject::SetCbufferStructData(
	const std::string &cbuffer_name,
	const std::string &variable_name,
	const void* data_in,
	const pancy_resource_size &data_size,
	const pancy_resource_size &offset
) 
{
	PancystarEngine::EngineFailReason check_error;
	auto cbuffer_data = per_object_cbuffer.find(cbuffer_name);
	if (cbuffer_data == per_object_cbuffer.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in DescriptorObject of PSO: " + PSO_name_descriptor);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer struct", error_message);
		return error_message;
	}
	check_error = cbuffer_data->second->SetStruct(variable_name, data_in, data_size,offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
//描述符链
DescriptorObjectList::DescriptorObjectList(
	const std::string &PSO_name_in,
	const std::string &descriptor_name_in
)
{
	PSO_name = PSO_name_in;
	descriptor_name = descriptor_name_in;
}
DescriptorObjectList::~DescriptorObjectList()
{
	//删除所有的描述符备份
	while (!empty_list.empty())
	{
		auto data = empty_list.front();
		empty_list.pop();
		delete data;
	}
	while (!used_list.empty())
	{
		auto data = used_list.front();
		empty_list.pop();
		delete data;
	}
}
PancystarEngine::EngineFailReason DescriptorObjectList::Create(
	const std::vector<std::string> &cbuffer_name_per_object_in,
	const std::vector<PancystarEngine::PancyConstantBuffer *> &cbuffer_per_frame_in,
	const std::vector<SubMemoryPointer> &resource_data_per_frame_in,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_in,
	const std::vector<SubMemoryPointer> &resource_data_per_object_in,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_object_in
)
{
	PancystarEngine::EngineFailReason check_error;
	//将资源信息拷贝
	for (int i = 0; i < cbuffer_name_per_object_in.size(); ++i)
	{
		//检验传入的每个常量缓冲区名称是否合法
		pancy_object_id PSO_id;
		check_error = PancyEffectGraphic::GetInstance()->GetPSO(PSO_name, PSO_id);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		check_error = PancyEffectGraphic::GetInstance()->CheckCbuffer(PSO_id, cbuffer_name_per_object_in[i]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		cbuffer_name_per_object.push_back(cbuffer_name_per_object_in[i]);
	}
	for (int i = 0; i < cbuffer_per_frame_in.size(); ++i)
	{
		cbuffer_per_frame.push_back(cbuffer_per_frame_in[i]);
	}
	for (int i = 0; i < resource_data_per_frame_in.size(); ++i)
	{
		resource_data_per_frame.push_back(resource_data_per_frame_in[i]);
	}
	for (int i = 0; i < resource_data_per_object_in.size(); ++i)
	{
		resource_data_per_object.push_back(resource_data_per_object_in[i]);
	}
	for (int i = 0; i < resource_desc_per_frame_in.size(); ++i)
	{
		resource_desc_per_frame.push_back(resource_desc_per_frame_in[i]);
	}
	for (int i = 0; i < resource_desc_per_object_in.size(); ++i)
	{
		resource_desc_per_per_object.push_back(resource_desc_per_object_in[i]);
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorObjectList::GetEmptyList(DescriptorObject** descripto_res)
{
	if (empty_list.size() > 0)
	{
		auto empty_descriptor = empty_list.front();
		empty_list.pop();
		used_list.push(empty_descriptor);
		*descripto_res = empty_descriptor;
	}
	else
	{
		DescriptorObject *new_descriptor_obj;
		new_descriptor_obj = new DescriptorObject();
		auto check_error = new_descriptor_obj->Create(
			PSO_name,
			descriptor_name,
			cbuffer_name_per_object,
			cbuffer_per_frame,
			resource_data_per_frame,
			resource_desc_per_frame,
			resource_data_per_object,
			resource_desc_per_per_object
		);
		if (!check_error.CheckIfSucceed())
		{
			*descripto_res = NULL;
			return check_error;
		}
		used_list.push(new_descriptor_obj);
		*descripto_res = new_descriptor_obj;
	}
	return PancystarEngine::succeed;
}
void DescriptorObjectList::Reset()
{
	//将已经使用完毕的描述符还原
	while (!used_list.empty())
	{
		auto empty_descriptor = used_list.front();
		used_list.pop();
		empty_list.push(empty_descriptor);
	}
}
//模型管理器
static PancyModelControl* this_instance = NULL;
PancyModelControl::PancyModelControl(const std::string &resource_type_name_in) :PancyBasicResourceControl(resource_type_name_in)
{

}
PancystarEngine::EngineFailReason PancyModelControl::GetRenderDescriptor(
	pancy_object_id model_id,
	pancy_object_id PSO_id,
	const std::vector<std::string> &cbuffer_name_per_object_in,
	const std::vector<PancystarEngine::PancyConstantBuffer *> &cbuffer_per_frame_in,
	const std::vector<SubMemoryPointer> &resource_data_per_frame_in,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_in,
	DescriptorObject **descriptor_out
) 
{
	PancystarEngine::EngineFailReason check_error;
	auto resource_data = GetResource(model_id);
	if (resource_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL,"could not find resource ID: " + std::to_string(model_id));
		return error_message;
	}
	PancyBasicModel *model_pointer = dynamic_cast<PancyBasicModel*>(resource_data);
	check_error = model_pointer->GetRenderDescriptor(
		PSO_id, 
		cbuffer_name_per_object_in, 
		cbuffer_per_frame_in, 
		resource_data_per_frame_in, 
		resource_desc_per_frame_in, 
		descriptor_out
	);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyModelControl::BuildResource(
	const Json::Value &root_value,
	const std::string &name_resource_in,
	PancyBasicVirtualResource** resource_out
)
{
	*resource_out = new PancyBasicModel(name_resource_in, root_value);
	return PancystarEngine::succeed;
}