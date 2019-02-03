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
PancyBasicModel::PancyBasicModel(const std::string &resource_name, const Json::Value &root_value) : PancyBasicVirtualResource(resource_name,root_value)
{
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
PancystarEngine::EngineFailReason PancyBasicModel::InitResource(const Json::Value &root_value, const std::string &resource_name)
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
			instream.open(now_animation_name,ios::binary);
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
					new_bone_data.rotation_key.push_back(new_rotation_key[i]);
				}
				//平移向量
				int32_t translation_key_num = 0;
				instream.read(reinterpret_cast<char*>(&translation_key_num), sizeof(translation_key_num));
				vector_animation *new_translation_key = new vector_animation[translation_key_num];
				int32_t translation_key_size = sizeof(new_translation_key[0]) * translation_key_num;
				instream.read(reinterpret_cast<char*>(new_translation_key), translation_key_size);
				for (int j = 0; j < translation_key_num; ++j)
				{
					new_bone_data.translation_key.push_back(new_translation_key[i]);
				}
				
				//缩放向量
				int32_t scaling_key_num = 0;
				instream.read(reinterpret_cast<char*>(&scaling_key_num), sizeof(scaling_key_num));
				vector_animation *new_scaling_key = new vector_animation[scaling_key_num];
				int32_t scaling_key_size = sizeof(new_scaling_key[0]) * scaling_key_num;
				instream.read(reinterpret_cast<char*>(new_scaling_key), scaling_key_size);
				for (int j = 0; j < scaling_key_num; ++j)
				{
					new_bone_data.scaling_key.push_back(new_scaling_key[i]);
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
			instream.open(now_animation_name,ios::binary);
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
	return PancystarEngine::succeed;
}
//模型管理器
static PancyModelControl* this_instance = NULL;
PancyModelControl::PancyModelControl(const std::string &resource_type_name_in) :PancyBasicResourceControl(resource_type_name_in)
{

}
PancystarEngine::EngineFailReason PancyModelControl::BuildResource(
	const Json::Value &root_value,
	const std::string &name_resource_in,
	PancyBasicVirtualResource** resource_out
)
{
	*resource_out = new PancyBasicModel(name_resource_in,root_value);
	return PancystarEngine::succeed;
}