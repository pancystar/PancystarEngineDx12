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
class CommonEnumValueParser 
{
public:
	CommonEnumValueParser();
	virtual PancystarEngine::EngineFailReason SetEnumValue(void*data_pointer,const int32_t enum_data) = 0;
	virtual PancystarEngine::EngineFailReason SetEnumArrayValue(void*data_pointer, const int32_t &enum_offset, const int32_t &enum_data) = 0;
	virtual PancystarEngine::EngineFailReason SetEnumVectorValue(void*data_pointer, const int32_t enum_offsetdata,const int32_t enum_data) = 0;
	virtual PancystarEngine::EngineFailReason GetEnumValue(void*data_pointer,int32_t &enum_data_out) = 0;
	virtual PancystarEngine::EngineFailReason GetEnumArrayValue(void*data_pointer, const int32_t &enum_offsetdata,int32_t &enum_data_out) = 0;
	virtual PancystarEngine::EngineFailReason GetEnumVectorValue(void*data_pointer, const int32_t &enum_offsetdata, int32_t &enum_data_out) = 0;
	virtual PancystarEngine::EngineFailReason GetEnumVectorSize(void*data_pointer, pancy_object_id &vector_size) = 0;
};

template<class T>
class PancyEnumValueParser : public CommonEnumValueParser
{
public:
	PancyEnumValueParser();
	PancystarEngine::EngineFailReason SetEnumValue(void*data_pointer, const int32_t enum_data) override;
	PancystarEngine::EngineFailReason SetEnumArrayValue(void*data_pointer,const int32_t &enum_offset, const int32_t &enum_data) override;
	PancystarEngine::EngineFailReason SetEnumVectorValue(void*data_pointer, const int32_t enum_offsetdata, const int32_t enum_data) override;
	PancystarEngine::EngineFailReason GetEnumValue(void*data_pointer, int32_t &enum_data_out) override;
	PancystarEngine::EngineFailReason GetEnumArrayValue(void*data_pointer, const int32_t &enum_offsetdata, int32_t &enum_data_out) override;
	PancystarEngine::EngineFailReason GetEnumVectorValue(void*data_pointer, const int32_t &enum_offsetdata, int32_t &enum_data_out) override;
	PancystarEngine::EngineFailReason GetEnumVectorSize(void*data_pointer,pancy_object_id &vector_size) override;
};
template<class T>
PancyEnumValueParser<T>::PancyEnumValueParser() 
{

}
template<class T>
PancystarEngine::EngineFailReason PancyEnumValueParser<T>::SetEnumValue(void*data_pointer, const int32_t enum_data)
{
	//将int32类型转换为枚举类型
	T* real_data_pointer = reinterpret_cast<T*>(data_pointer);
	*real_data_pointer = static_cast<T>(enum_data);
	return PancystarEngine::succeed;
}
template<class T>
PancystarEngine::EngineFailReason PancyEnumValueParser<T>::SetEnumArrayValue(void*data_pointer, const int32_t &enum_offset, const int32_t &enum_data)
{
	//将int32类型转换为枚举类型
	T* real_data_pointer = reinterpret_cast<T*>(data_pointer);
	real_data_pointer[enum_offset] = static_cast<T>(enum_data);
	return PancystarEngine::succeed;
};
template<class T>
PancystarEngine::EngineFailReason PancyEnumValueParser<T>::SetEnumVectorValue(void*data_pointer, const int32_t enum_offsetdata, const int32_t enum_data)
{
	std::vector<T> *vec_pointer = reinterpret_cast<std::vector<T>*>(data_pointer);
	if (vec_pointer->size() != enum_offsetdata) 
	{
		//偏移量不正确，插入第i个元素要保证已经有了i个成员
		PancystarEngine::EngineFailReason error_message(E_FAIL, "reflect vector have wrong offset: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetEnumVectorValue", error_message);
		return error_message;
	}
	T new_value = static_cast<T>(enum_data);
	vec_pointer->push_back(new_value);
	return PancystarEngine::succeed;
}
template<class T>
PancystarEngine::EngineFailReason PancyEnumValueParser<T>::GetEnumValue(void*data_pointer, int32_t &enum_data_out)
{
	//将int32类型转换为枚举类型
	T* real_data_pointer = reinterpret_cast<T*>(data_pointer);
	enum_data_out = static_cast<int32_t>(*real_data_pointer);
	return PancystarEngine::succeed;
}
template<class T>
PancystarEngine::EngineFailReason PancyEnumValueParser<T>::GetEnumArrayValue(void*data_pointer, const int32_t &enum_offsetdata, int32_t &enum_data_out)
{
	//将int32类型转换为枚举类型
	T* real_data_pointer = reinterpret_cast<T*>(data_pointer);
	enum_data_out = static_cast<int32_t>(real_data_pointer[enum_offsetdata]);
	return PancystarEngine::succeed;
}
template<class T>
PancystarEngine::EngineFailReason PancyEnumValueParser<T>::GetEnumVectorValue(void*data_pointer, const int32_t &enum_offsetdata, int32_t &enum_data_out)
{
	std::vector<T> *vec_pointer = reinterpret_cast<std::vector<T>*>(data_pointer);
	if (vec_pointer->size() <= enum_offsetdata)
	{
		//偏移量不正确，成员数量不足
		PancystarEngine::EngineFailReason error_message(E_FAIL, "reflect vector have wrong offset: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::GetEnumVectorValue", error_message);
		return error_message;
	}
	enum_data_out = static_cast<int32_t>((*vec_pointer)[enum_offsetdata]);
	return PancystarEngine::succeed;
}
template<class T>
PancystarEngine::EngineFailReason PancyEnumValueParser<T>::GetEnumVectorSize(void*data_pointer, pancy_object_id &vector_size)
{
	std::vector<T> *vec_pointer = reinterpret_cast<std::vector<T>*>(data_pointer);
	vector_size = static_cast<pancy_object_id>(vec_pointer->size());
	return PancystarEngine::succeed;
}
//注册枚举变量到json工具
#define JSON_REFLECT_INIT_ENUM(enum_variable) auto new_enum_variable_##enum_variable = enum_variable;\
											  PancyJsonTool::GetInstance()->SetGlobelVraiable(#enum_variable,\
																							  static_cast<pancy_object_id>(enum_variable), \
																							  typeid(enum_variable).name(),\
																							  typeid(enum_variable),\
																							  typeid(&new_enum_variable_##enum_variable),\
																							  typeid(std::vector<decltype(enum_variable)>));\
                                              PancyJsonTool::GetInstance()->AddEnumParseClass<decltype(enum_variable)>(typeid(enum_variable).name())\
//获取枚举变量的名字
#define JSON_GET_ENUM_NAME(enum_variable) PancyJsonTool::GetInstance()->GetEnumName(typeid(enum_variable).name(),static_cast<int32_t>(enum_num))
struct pancy_json_value
{
	pancy_json_data_type value_type;
	int32_t int_value;
	float float_value;
	bool bool_value;
	std::string string_value;
};
enum PancyJsonMemberType
{
	//未识别的类型
	json_member_unknown = 0,
	//普通变量
	json_member_int8,
	json_member_int16,
	json_member_int32,
	json_member_int64,
	json_member_uint8,
	json_member_uint16,
	json_member_uint32,
	json_member_uint64,
	json_member_float,
	json_member_double,
	json_member_enum,
	json_member_string,
	json_member_bool,
	//链表变量
	json_member_int8_list,
	json_member_int16_list,
	json_member_int32_list,
	json_member_int64_list,
	json_member_uint8_list,
	json_member_uint16_list,
	json_member_uint32_list,
	json_member_uint64_list,
	json_member_float_list,
	json_member_double_list,
	json_member_enum_list,
	json_member_string_list,
	json_member_bool_list,
	//数组变量
	json_member_int8_array,
	json_member_int16_array,
	json_member_int32_array,
	json_member_int64_array,
	json_member_uint8_array,
	json_member_uint16_array,
	json_member_uint32_array,
	json_member_uint64_array,
	json_member_float_array,
	json_member_double_array,
	json_member_enum_array,
	json_member_string_array,
	json_member_bool_array,
	//节点变量
	json_member_node,
	json_member_node_list,
	json_member_node_array
};
class PancyJsonTool
{
	Json::StreamWriterBuilder Jwriter;
	Json::CharReaderBuilder builder;
	ifstream FileOpen;
	ofstream FileWrite;
	//通过枚举名得到枚举变量
	std::unordered_map<std::string, std::unordered_map<std::string, int32_t>> enum_variable_list;
	//通过枚举变量得到枚举名
	std::unordered_map<std::string, std::unordered_map<int32_t, std::string>> enum_name_list;
	//通过枚举名设置枚举变量
	std::unordered_map<std::string, CommonEnumValueParser*> enum_parse_list;
	//todo:删掉关于shader的特殊反射
	std::string name_value_type[7];
	//json反射相关......
	std::unordered_map<size_t, PancyJsonMemberType> json_type_map;//所有已经被识别的json类型
	std::unordered_map<std::string, std::string> enum_pointer_value_map;//枚举指针对应的基本类型
private:
	PancyJsonTool();
public:
	~PancyJsonTool();
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
	PancystarEngine::EngineFailReason SetGlobelVraiable(
		const std::string &variable_name,
		const int32_t &variable_value,
		const std::string &enum_type,
		const type_info &enmu_self_type,
		const type_info &enmu_array_type,
		const type_info &enmu_list_type
	);
	template<class T>
	PancystarEngine::EngineFailReason AddEnumParseClass(const std::string &enum_type);
	PancystarEngine::EngineFailReason SetEnumMemberValue(const std::string &variable_type, void*value_pointer, const int32_t value_data);
	PancystarEngine::EngineFailReason SetEnumArrayValue(const std::string &variable_type, void*value_pointer, const int32_t &enum_offset, const int32_t &enum_data);
	PancystarEngine::EngineFailReason SetEnumVectorValue(const std::string &variable_type, void*value_pointer, const int32_t enum_offsetdata, const int32_t value_data);
	PancystarEngine::EngineFailReason GetEnumMemberValue(const std::string &variable_type, void*value_pointer, int32_t &value_data);
	PancystarEngine::EngineFailReason GetEnumArrayValue(const std::string &variable_type, void*value_pointer, const int32_t &enum_offsetdata, int32_t &enum_data_out);
	PancystarEngine::EngineFailReason GetEnumVectorValue(const std::string &variable_type, void*value_pointer, const int32_t &enum_offsetdata, int32_t &enum_data_out);
	PancystarEngine::EngineFailReason GetEnumVectorSize(const std::string &variable_type, void*value_pointer, pancy_object_id &enum_size_out);
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
	int32_t GetEnumValue(const std::string &enum_type, const std::string &enum_name);
	PancystarEngine::EngineFailReason GetEnumNameByPointerName(const std::string &enum_pointer_type, std::string &enum_type);
	PancyJsonMemberType GetVariableJsonType(const size_t &variable_type);
private:
	std::string DevideEnumValue(const int32_t &enum_number, const std::unordered_map<int32_t, std::string> all_enum_member);
	void InitBasicType();
	PancystarEngine::EngineFailReason GetJsonMemberData
	(
		const std::string &file_name,
		const Json::Value &enum_type_value,
		const std::string &member_name,
		const pancy_json_data_type &json_type,
		pancy_json_value &variable_value
	);
	void SplitString(std::string str, const std::string &pattern,std::vector<std::string> &result);
};
template<class T>
PancystarEngine::EngineFailReason PancyJsonTool::AddEnumParseClass(const std::string &enum_type)
{
	auto enum_parse_member = enum_parse_list.find(enum_type);
	if (enum_parse_member != enum_parse_list.end())
	{
		return PancystarEngine::succeed;
	}
	CommonEnumValueParser* new_enum_parse = new PancyEnumValueParser<T>();
	enum_parse_list.insert(std::pair<std::string, CommonEnumValueParser*>(enum_type, new_enum_parse));
	return PancystarEngine::succeed;
}

