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
}
PancyBasicModel::~PancyBasicModel()
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
//骨骼动画处理函数
PancystarEngine::EngineFailReason PancyBasicModel::LoadSkinTree(const string &filename)
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
	//递归重建骨骼树
	bone_object_num = bone_num;
	auto check_error = ReadBoneTree(root_id);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	//关闭文件
	instream.close();
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicModel::ReadBoneTree(int32_t &now_build_id)
{
	char data[11];
	skin_tree now_bone_data;
	instream.read(reinterpret_cast<char*>(&now_bone_data), sizeof(now_bone_data));
	//根据读取到的骨骼节点创建一个骨骼结构并赋予编号
	bone_struct new_bone;
	bool if_used_skin = false;;
	if (now_bone_data.bone_number == NouseAssimpStruct) 
	{
		//当前骨骼并未用于蒙皮信息,重新生成一个ID号
		now_build_id = bone_object_num;
		bone_object_num += 1;
	}
	else 
	{
		if_used_skin = true;
		now_build_id = now_bone_data.bone_number;
	}
	new_bone.bone_name = now_bone_data.bone_ID;
	new_bone.bone_ID_son = NouseAssimpStruct;
	new_bone.bone_ID_brother = NouseAssimpStruct;
	new_bone.if_used_for_skin = if_used_skin;
	bone_tree_data.insert(std::pair<int32_t, bone_struct>(now_build_id, new_bone));
	bone_name_index.insert(std::pair<std::string, int32_t>(new_bone.bone_name, now_build_id));
	instream.read(data, sizeof(data));
	while (strcmp(data, "*heaphead*") == 0)
	{
		//入栈符号，代表子节点
		int32_t now_son_ID = NouseAssimpStruct;
		//递归创建子节点
		auto check_error = ReadBoneTree(now_son_ID);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		auto now_parent_data = bone_tree_data.find(now_build_id);
		auto now_data = bone_tree_data.find(now_son_ID);
		if (now_parent_data == bone_tree_data.end() )
		{
			//传入的父节点不合理
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find Parent bone ID "+std::to_string(now_build_id));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Model Bone Data From File", error_message);
			return error_message;
		}
		else if (now_data == bone_tree_data.end())
		{
			//生成的子节点不合理
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find Son Bone ID " + std::to_string(now_son_ID));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Model Bone Data From File", error_message);
			return error_message;
		}
		else 
		{
			//当前节点的兄弟节点存储之前父节点的子节点
			now_data->second.bone_ID_brother = now_parent_data->second.bone_ID_son;
			//之前父节点的子节点变为当前节点
			now_parent_data->second.bone_ID_son = now_son_ID;
			instream.read(data, sizeof(data));
		}
	}
	return PancystarEngine::succeed;
}
void PancyBasicModel::Interpolate(quaternion_animation& pOut, const quaternion_animation &pStart, const quaternion_animation &pEnd, const float &pFactor)
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
void PancyBasicModel::Interpolate(vector_animation& pOut, const vector_animation &pStart, const vector_animation &pEnd, const float &pFactor)
{
	for (int i = 0; i < 3; ++i)
	{
		pOut.main_key[i] = pStart.main_key[i] + pFactor * (pEnd.main_key[i] - pStart.main_key[i]);
	}
}
void PancyBasicModel::FindAnimStEd(const float &input_time, int &st, int &ed, const std::vector<quaternion_animation> &input)
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
void PancyBasicModel::FindAnimStEd(const float &input_time, int &st, int &ed, const std::vector<vector_animation> &input)
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
PancystarEngine::EngineFailReason PancyBasicModel::GetBoneByAnimation(
	const pancy_resource_id &animation_ID, 
	const float &animation_time, 
	std::vector<DirectX::XMFLOAT4X4> &bone_final_matrix
)
{
	PancystarEngine::EngineFailReason check_error;
	//先判断模型数据是否支持骨骼动画
	if (!if_skinmesh) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "model: " +resource_name +"don't have skin mesh message");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Update mesh animation", error_message);
		return error_message;
	}
	std::vector<DirectX::XMFLOAT4X4> matrix_animation_save;
	matrix_animation_save.resize(bone_object_num);
	for (int i = 0; i < bone_object_num; ++i)
	{
		//使用单位矩阵将动画矩阵清空
		DirectX::XMStoreFloat4x4(&matrix_animation_save[i],DirectX::XMMatrixIdentity());
	}
	check_error = UpdateAnimData(animation_ID, animation_time, matrix_animation_save);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	DirectX::XMFLOAT4X4 matrix_identi;
	DirectX::XMStoreFloat4x4(&matrix_identi, DirectX::XMMatrixIdentity());
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//开辟一个临时存储所有骨骼递归结果的矩阵数组。由于最终输出的数据仅包
	//含蒙皮骨骼，因而需要额外的空间存储非蒙皮骨骼的中间信息。
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::vector<DirectX::XMFLOAT4X4> matrix_combine_save;
	matrix_combine_save.resize(bone_object_num);
	for (int i = 0; i < bone_object_num; ++i)
	{
		DirectX::XMStoreFloat4x4(&matrix_combine_save[i], DirectX::XMMatrixIdentity());
	}
	std::vector<DirectX::XMFLOAT4X4> matrix_out_save;
	matrix_out_save.resize(bone_num);
	bone_final_matrix.resize(bone_num);
	UpdateRoot(root_id, matrix_identi, matrix_animation_save, matrix_combine_save, matrix_out_save);
	//将更新后的动画矩阵做偏移
	for (int i = 0; i < bone_num; ++i)
	{
		//使用单位矩阵将混合矩阵清空
		DirectX::XMStoreFloat4x4(&bone_final_matrix[i], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&offset_matrix_array[i]) * DirectX::XMLoadFloat4x4(&matrix_out_save[i])));
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicModel::UpdateAnimData(
	const pancy_resource_id &animation_ID, 
	const float &time_in,
	std::vector<DirectX::XMFLOAT4X4> &matrix_out
)
{
	//根据动画的ID号查找对应的动画数据
	auto skin_anim_data = skin_animation_map.find(animation_ID);
	if (skin_anim_data == skin_animation_map.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find skin_animation ID " + std::to_string(animation_ID));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Update mesh animation", error_message);
		return error_message;
	}
	animation_set now_animation_use = skin_anim_data->second;
	float input_time = time_in * now_animation_use.animation_length;
	for (int i = 0; i < now_animation_use.data_animition.size(); ++i)
	{
		animation_data now = now_animation_use.data_animition[i];
		DirectX::XMMATRIX rec_trans, rec_scal;
		DirectX::XMFLOAT4X4 rec_rot;

		int start_anim, end_anim;
		FindAnimStEd(input_time, start_anim, end_anim, now.rotation_key);
		//四元数插值并寻找变换矩阵
		quaternion_animation rotation_now;
		if (start_anim == end_anim || end_anim >= now.rotation_key.size())
		{
			rotation_now = now.rotation_key[start_anim];
		}
		else
		{
			Interpolate(rotation_now, now.rotation_key[start_anim], now.rotation_key[end_anim], (input_time - now.rotation_key[start_anim].time) / (now.rotation_key[end_anim].time - now.rotation_key[start_anim].time));
		}
		GetQuatMatrix(rec_rot, rotation_now);
		//缩放变换
		FindAnimStEd(input_time, start_anim, end_anim, now.scaling_key);
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
		//平移变换
		FindAnimStEd(input_time, start_anim, end_anim, now.translation_key);
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
		//检测当前动画节点对应的骨骼数据是否还正常
		auto bone_data = bone_tree_data.find(now.bone_ID);
		if (bone_data == bone_tree_data.end()) 
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find Bone ID " + std::to_string(now.bone_ID) +" in model: "+ resource_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Update mesh animation", error_message);
			return error_message;
		}
		XMStoreFloat4x4(&matrix_out[now.bone_ID], rec_scal * XMLoadFloat4x4(&rec_rot) * rec_trans);
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicModel::UpdateRoot(
	int32_t root_id, 
	const DirectX::XMFLOAT4X4 &matrix_parent,
	const std::vector<DirectX::XMFLOAT4X4> &matrix_animation,
	std::vector<DirectX::XMFLOAT4X4> &matrix_combine_save,
	std::vector<DirectX::XMFLOAT4X4> &matrix_out
)
{
	PancystarEngine::EngineFailReason check_error;
	//查找当前的父节点是否存在
	auto now_root_bone_data = bone_tree_data.find(root_id);
	if (now_root_bone_data == bone_tree_data.end()) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find bone ID"+ std::to_string(root_id) + " in model: " + resource_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Update Bone tree", error_message);
		return error_message;
	}
	//获取当前骨骼的动画矩阵
	DirectX::XMMATRIX rec = DirectX::XMLoadFloat4x4(&matrix_animation[root_id]);
	DirectX::XMStoreFloat4x4(&matrix_combine_save[root_id], rec * DirectX::XMLoadFloat4x4(&matrix_parent));
	//对于蒙皮需要的骨骼，将其导出
	if (now_root_bone_data->second.if_used_for_skin)
	{
		matrix_out[root_id] = matrix_combine_save[root_id];
	}
	//更新兄弟节点及子节点
	if (now_root_bone_data->second.bone_ID_brother != NouseAssimpStruct) 
	{
		check_error = UpdateRoot(now_root_bone_data->second.bone_ID_brother, matrix_parent, matrix_animation, matrix_combine_save, matrix_out);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	if (now_root_bone_data->second.bone_ID_son != NouseAssimpStruct) 
	{
		check_error = UpdateRoot(now_root_bone_data->second.bone_ID_son, matrix_combine_save[root_id], matrix_animation, matrix_combine_save, matrix_out);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
void PancyBasicModel::GetQuatMatrix(DirectX::XMFLOAT4X4 &resMatrix, const quaternion_animation& pOut)
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
//资源加载
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
		std::string texture_name_now = path_name + file_name + "_tex" + std::to_string(i) + ".dds";
		pancy_object_id texture_id;
		//加载纹理并添加引用计数
		Json::Value new_texture;
		PancyJsonTool::GetInstance()->SetJsonValue(new_texture, "FileName", texture_name_now);
		PancyJsonTool::GetInstance()->SetJsonValue(new_texture, "IfAutoBuildMipMap", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(new_texture, "IfForceSrgb", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(new_texture, "IfFromFile", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(new_texture, "MaxSize", 0);
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(texture_name_now, new_texture, texture_id);
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
		std::vector<pancy_object_id> now_material_id_need;
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
		now_material_id_need.push_back(rec_value.int_value);
		//法线纹理
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "Normaltex", pancy_json_data_type::json_data_int, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_normal, rec_value.int_value));
		now_material_id_need.push_back(rec_value.int_value);
		//AO纹理
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "Ambienttex", pancy_json_data_type::json_data_int, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_ambient, rec_value.int_value));
		now_material_id_need.push_back(rec_value.int_value);
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
			now_material_id_need.push_back(rec_value.int_value);
			//粗糙度纹理
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "RoughnessTex", pancy_json_data_type::json_data_int, rec_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_roughness, rec_value.int_value));
			now_material_id_need.push_back(rec_value.int_value);
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
			now_material_id_need.push_back(rec_value.int_value);
		}
		material_list.insert(std::pair<pancy_object_id, std::unordered_map<TexType, pancy_object_id>>(material_id, now_material_need));
		material_id_list.insert(std::pair<pancy_object_id, std::vector<pancy_object_id>>(material_id, now_material_id_need));
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
			float animation_length;
			instream.read(reinterpret_cast<char*>(&animation_length), sizeof(animation_length));
			instream.read(reinterpret_cast<char*>(&animation_bone_num), sizeof(animation_bone_num));
			new_animation.animation_length = animation_length;
			for (int l = 0; l < animation_bone_num; ++l)
			{
				animation_data new_bone_data;
				//骨骼信息
				int32_t bone_name_size = -1;
				instream.read(reinterpret_cast<char*>(&bone_name_size), sizeof(bone_name_size));
				char *name = new char[bone_name_size];
				instream.read(name, bone_name_size * sizeof(char));
				new_bone_data.bone_name += name;
				auto now_used_bone_ID = bone_name_index.find(new_bone_data.bone_name);
				if (now_used_bone_ID == bone_name_index.end()) 
				{
					PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find bone: " + new_bone_data.bone_name + " in model: " + resource_name);
					PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Skin Mesh Animation Data", error_message);
					return error_message;
				}
				new_bone_data.bone_ID = now_used_bone_ID->second;
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
PancystarEngine::EngineFailReason PancyBasicModel::GetShaderResourcePerObject(
	std::vector<SubMemoryPointer> &resource_data_per_frame_out,
	std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_out
)
{
	PancystarEngine::EngineFailReason check_error;
	//整理模型自带的顶点动画资源
	if (if_pointmesh)
	{

	}
	//整理模型的纹理资源
	bool if_need_resource_barrier = false;
	for (int i = 0; i < material_id_list.size(); ++i)
	{
		for (int j = 0; j < material_id_list[i].size(); ++j)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC new_SRV_desc;
			SubMemoryPointer now_texture;
			PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(texture_list[material_id_list[i][j]], now_texture);
			resource_data_per_frame_out.push_back(now_texture);
			D3D12_RESOURCE_STATES test_state;
			SubresourceControl::GetInstance()->GetResourceState(now_texture, test_state);
			if (test_state != D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
			{
				if_need_resource_barrier = true;
			}
			PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(texture_list[material_id_list[i][j]], new_SRV_desc);
			resource_desc_per_frame_out.push_back(new_SRV_desc);
		}
	}
	if (if_need_resource_barrier)
	{
		//有些渲染资源尚未转换为SRV格式，调用主线程统一转换
		PancyRenderCommandList *m_commandList;
		PancyThreadIdGPU commdlist_id_use;
		check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(NULL, &m_commandList, commdlist_id_use);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		for (int i = 0; i < material_id_list.size(); ++i)
		{
			for (int j = 0; j < material_id_list[i].size(); ++j)
			{
				SubMemoryPointer now_texture;
				PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(texture_list[material_id_list[i][j]], now_texture);
				D3D12_RESOURCE_STATES test_state;
				SubresourceControl::GetInstance()->GetResourceState(now_texture, test_state);
				if (test_state != D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
				{
					SubresourceControl::GetInstance()->ResourceBarrier(m_commandList, now_texture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				};
			}
		}
		m_commandList->UnlockPrepare();
		ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(1, &commdlist_id_use);
	}
	return PancystarEngine::succeed;
}
//模型管理器
static PancyModelControl* this_instance = NULL;
PancyModelControl::PancyModelControl(const std::string &resource_type_name_in) :PancyBasicResourceControl(resource_type_name_in)
{

}
PancystarEngine::EngineFailReason PancyModelControl::GetRenderMesh(const pancy_object_id &model_id, const pancy_object_id &submesh_id, PancySubModel **render_mesh)
{
	PancystarEngine::EngineFailReason check_error;
	auto resource_data = GetResource(model_id);
	if (resource_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource ID: " + std::to_string(model_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Render mesh From Model", error_message);
		return error_message;
	}
	PancyBasicModel *model_pointer = dynamic_cast<PancyBasicModel*>(resource_data);
	check_error = model_pointer->GetRenderMesh(submesh_id, render_mesh);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyModelControl::GetShaderResourcePerObject(
	const pancy_object_id &model_id,
	std::vector<SubMemoryPointer> &resource_data_per_frame_out,
	std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_out
) 
{
	PancystarEngine::EngineFailReason check_error;
	auto resource_data = GetResource(model_id);
	if (resource_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource ID: " + std::to_string(model_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Render mesh From Model", error_message);
		return error_message;
	}
	PancyBasicModel *model_pointer = dynamic_cast<PancyBasicModel*>(resource_data);
	check_error = model_pointer->GetShaderResourcePerObject(resource_data_per_frame_out, resource_desc_per_frame_out);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyModelControl::GetModelBoneMatrix(
	const pancy_object_id &model_id,
	const pancy_resource_id &animation_ID,
	const float &animation_time,
	std::vector<DirectX::XMFLOAT4X4> &matrix_out
) 
{
	PancystarEngine::EngineFailReason check_error;
	auto resource_data = GetResource(model_id);
	if (resource_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource ID: " + std::to_string(model_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Render Descriptor From Model", error_message);
		return error_message;
	}
	PancyBasicModel *model_pointer = dynamic_cast<PancyBasicModel*>(resource_data);
	check_error = model_pointer->GetBoneByAnimation(animation_ID, animation_time, matrix_out);
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
