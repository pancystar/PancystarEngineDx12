#pragma once
#include <json.h>
#include"PancystarEngineBasicDx12.h"
using namespace std;
enum pancy_json_data_type
{
	json_data_int = 0,
	json_data_float,
	json_data_enum,
	json_data_string,
	json_data_bool
};
enum Pancy_json_shader_type
{
	json_shader_vertex = 0,
	json_shader_pixel,
	json_shader_geometry,
	json_shader_hull,
	json_shader_domin,
	json_shader_compute
};
#define JSON_REFLECT_INIT_ENUM(enum_variable) PancyJsonTool::GetInstance()->SetGlobelVraiable(#enum_variable, static_cast<int32_t>(enum_variable), typeid(enum_variable).name());
struct pancy_json_value
{
	pancy_json_data_type value_type;
	int32_t int_value;
	float float_value;
	bool bool_value;
	std::string string_value;
};
class PancyJsonTool
{
	Json::StreamWriterBuilder Jwriter;
	Json::CharReaderBuilder builder;
	ifstream FileOpen;
	ofstream FileWrite;
	std::unordered_map<std::string, int32_t> globel_variables;
	std::unordered_map<std::string, std::unordered_map<int32_t, std::string>> enum_name_list;
	std::string name_value_type[7];
	std::string name_shader_type[6];
private:
	PancyJsonTool();
public:
	static PancyJsonTool* GetInstance()
	{
		static PancyJsonTool* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new PancyJsonTool();
		}
		return this_instance;
	}
	//读取json数据
	PancystarEngine::EngineFailReason LoadJsonFile(const std::string &file_name, Json::Value &root_value);
	PancystarEngine::EngineFailReason SetGlobelVraiable(const std::string &variable_name, const int32_t &variable_value,std::string enum_type);
	PancystarEngine::EngineFailReason GetJsonData
	(
		const std::string &file_name,
		const Json::Value &root_value,
		const std::string &member_name,
		const pancy_json_data_type &json_type,
		pancy_json_value &variable_value
	);
	PancystarEngine::EngineFailReason GetJsonData
	(
		const std::string &file_name,
		const Json::Value &root_value,
		const int32_t &member_num,
		const pancy_json_data_type &json_type,
		pancy_json_value &variable_value
	);
	PancystarEngine::EngineFailReason GetJsonShader(
		const std::string &file_name,
		const Json::Value &root_value,
		const Pancy_json_shader_type &json_type,
		std::string &shader_file_name,
		std::string &shader_func_name
	);
	//更改及输出json数据
	template<class T>
	void SetJsonValue(
		Json::Value &insert_value,
		const std::string &value_name,
		const T &value
	)
	{
		insert_value[value_name] = value;
	}
	template<class T>
	void AddJsonArrayValue(
		Json::Value &insert_value,
		const std::string &value_name,
		const T &value
	)
	{
		insert_value[value_name].append(value);
	}
	PancystarEngine::EngineFailReason WriteValueToJson(
		const Json::Value &insert_value,
		const std::string &Json_name
	);
	std::string GetEnumName(const std::string &enum_type, int32_t enum_num);
private:
	PancystarEngine::EngineFailReason GetJsonMemberData
	(
		const std::string &file_name,
		const Json::Value &enum_type_value,
		const std::string &member_name,
		const pancy_json_data_type &json_type,
		pancy_json_value &variable_value
	);
	int32_t GetGlobelVariable(const std::string &variable_name);
	
};

