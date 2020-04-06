#include"PancyJsonTool.h"
CommonEnumValueParser::CommonEnumValueParser()
{

}
PancyJsonTool::PancyJsonTool()
{
	builder["collectComments"] = false;
	std::string name_value_need[] = {
		"nullValue"
		"intValue",
		"uintValue",
		"realValue",
		"stringValue",
		"booleanValue",
		"arrayValue",
		"objectValue"
	};
	for (int i = 0; i < 7; ++i)
	{
		name_value_type[i] = name_value_need[i];
	}
	InitBasicType();
}
PancyJsonTool::~PancyJsonTool() 
{
	for (auto release_data = enum_parse_list.begin(); release_data != enum_parse_list.end(); ++release_data) 
	{
		delete release_data->second;
	}
}
PancystarEngine::EngineFailReason PancyJsonTool::LoadJsonFile(const std::string &file_name, Json::Value &root_value)
{
	FileOpen.open(file_name);
	if (!FileOpen.is_open())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not open json file " + file_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Json File " + file_name, error_message);
		return error_message; 
	}
	root_value.clear();
	JSONCPP_STRING errs;
	if (!parseFromStream(builder, FileOpen, &root_value, &errs))
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, errs);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Json File " + file_name, error_message);
		return error_message;
	}
	FileOpen.close();
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonTool::SetGlobelVraiable(
	const std::string &variable_name,
	const int32_t &variable_value,
	const std::string &enum_type,
	const type_info &enmu_self_type,
	const type_info &enmu_array_type,
	const type_info &enmu_list_type
)
{
	//先检验是否存在对应类型的枚举
	auto enum_variable_type = enum_variable_list.find(enum_type);
	if (enum_variable_type == enum_variable_list.end())
	{
		std::unordered_map<std::string,int32_t> new_map;
		enum_variable_list.insert(std::pair<std::string, std::unordered_map<std::string,int32_t>>(enum_type, new_map));
		enum_variable_type = enum_variable_list.find(enum_type);
	}
	//在对应类型的枚举中添加变量
	auto check_if_have_enum_name = enum_variable_type->second.find(variable_name);
	if (check_if_have_enum_name != enum_variable_type->second.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "Add Json Variable name " + variable_name + " repeated", PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonTool::SetGlobelVraiable", error_message);
		return PancystarEngine::succeed;
	}
	enum_variable_type->second.insert(std::pair<std::string, int32_t>(variable_name, variable_value));
	//添加对应类型的枚举名称
	auto enum_list_type = enum_name_list.find(enum_type);
	if (enum_list_type == enum_name_list.end())
	{
		std::unordered_map<int32_t, std::string> new_map;
		enum_name_list.insert(std::pair<std::string, std::unordered_map<int32_t, std::string>>(enum_type, new_map));
		enum_list_type = enum_name_list.find(enum_type);
	}
	auto check_if_have_enum_value = enum_list_type->second.find(variable_value);
	if (check_if_have_enum_value != enum_list_type->second.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "Add Json Variable value " + check_if_have_enum_value->second + " repeated", PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonTool::SetGlobelVraiable", error_message);
		return PancystarEngine::succeed;
	}
	enum_list_type->second.insert(std::pair<int32_t, std::string>(variable_value, variable_name));
	//添加当前枚举变量的类型到类型表
	json_type_map[enmu_self_type.hash_code()] = json_member_enum;
	json_type_map[enmu_array_type.hash_code()] = json_member_enum_array;
	json_type_map[enmu_list_type.hash_code()] = json_member_enum_list;
	enum_pointer_value_map[enmu_array_type.name()] = enmu_self_type.name();
	enum_pointer_value_map[enmu_list_type.name()] = enmu_self_type.name();
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonTool::GetJsonData
(
	const std::string &file_name,
	const Json::Value &root_value,
	const std::string &member_name,
	const pancy_json_data_type &json_type,
	pancy_json_value &variable_value
)
{
	auto enum_type_value = root_value.get(member_name, Json::Value::null);
	return GetJsonMemberData(file_name, enum_type_value, member_name, json_type, variable_value);
}
PancystarEngine::EngineFailReason PancyJsonTool::GetJsonData
(
	const std::string &file_name,
	const Json::Value &root_value,
	const int32_t &member_num,
	const pancy_json_data_type &json_type,
	pancy_json_value &variable_value
)
{
	auto enum_type_value = root_value[member_num];
	return GetJsonMemberData(file_name, enum_type_value, "array::" + std::to_string(member_num), json_type, variable_value);
}
PancystarEngine::EngineFailReason PancyJsonTool::GetJsonMemberData
(
	const std::string &file_name,
	const Json::Value &enum_type_value,
	const std::string &member_name,
	const pancy_json_data_type &json_type,
	pancy_json_value &variable_value
)
{
	if (enum_type_value == Json::Value::null)
	{
		//未能获得json数据
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable " + member_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Root Signature json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	//整数数据
	else if (json_type == pancy_json_data_type::json_data_int)
	{
		if (enum_type_value.type() != Json::ValueType::intValue && enum_type_value.type() != Json::ValueType::uintValue)
		{
			int now_type_name = static_cast<int32_t>(enum_type_value.type());
			//json数据对应的类型不是整数类型
			PancystarEngine::EngineFailReason error_mesage(E_FAIL, "the value of variable " + member_name + " need int but find " + name_value_type[now_type_name]);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Root Signature json file " + file_name + " error", error_mesage);
			return error_mesage;
		}
		if (enum_type_value.type() == Json::ValueType::intValue)
		{
			variable_value.int_value = enum_type_value.asInt();
		}
		else if (enum_type_value.type() == Json::ValueType::uintValue)
		{
			variable_value.int_value = static_cast<int32_t>(enum_type_value.asUInt());
		}
	}
	//浮点数数据
	else if (json_type == pancy_json_data_type::json_data_float)
	{
		if (enum_type_value.type() != Json::ValueType::realValue)
		{
			int now_type_name = static_cast<int32_t>(enum_type_value.type());
			//json数据对应的类型不是浮点类型
			PancystarEngine::EngineFailReason error_mesage(E_FAIL, "the value of variable " + member_name + " need float but find " + name_value_type[now_type_name]);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Root Signature json file " + file_name + " error", error_mesage);
			return error_mesage;
		}
		variable_value.float_value = enum_type_value.asFloat();
	}
	//字符串数据
	else if (json_type == pancy_json_data_type::json_data_string)
	{
		if (enum_type_value.type() != Json::ValueType::stringValue)
		{
			int now_type_name = static_cast<int32_t>(enum_type_value.type());
			//json数据对应的类型不是浮点类型
			PancystarEngine::EngineFailReason error_mesage(E_FAIL, "the value of variable " + member_name + " need string but find " + name_value_type[now_type_name]);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Root Signature json file " + file_name + " error", error_mesage);
			return error_mesage;
		}
		variable_value.string_value = enum_type_value.asCString();
	}
	//bool数据
	else if (json_type == pancy_json_data_type::json_data_bool)
	{
		if (enum_type_value.type() != Json::ValueType::booleanValue)
		{
			int now_type_name = static_cast<int32_t>(enum_type_value.type());
			//json数据对应的类型不是浮点类型
			PancystarEngine::EngineFailReason error_mesage(E_FAIL, "the value of variable " + member_name + " need bool but find " + name_value_type[now_type_name]);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Root Signature json file " + file_name + " error", error_mesage);
			return error_mesage;
		}
		variable_value.bool_value = enum_type_value.asBool();
	}
	else 
	{
		//未能获得json数据
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not parse value of variable " + member_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Root Signature json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	return PancystarEngine::succeed;
}
void PancyJsonTool::SplitString(std::string str, const std::string &pattern, std::vector<std::string> &result)
{
	string::size_type pos;
	str += pattern;//扩展字符串以方便操作
	size_t size = str.size();
	for (size_t i = 0; i < size; i++) {
		pos = str.find(pattern, i);
		if (pos < size) {
			std::string s = str.substr(i, pos - i);
			result.push_back(s);
			i = pos + pattern.size() - 1;
		}
	}
}
int32_t PancyJsonTool::GetEnumValue(const std::string &enum_type, const std::string &enum_name)
{
	//解决或运算
	std::string now_string_deal = enum_name;
	std::vector<std::string> all_string_list;
	SplitString(now_string_deal,"|", all_string_list);
	auto enum_type_valuepack = enum_variable_list.find(enum_type);
	if (enum_type_valuepack == enum_variable_list.end())
	{
		return -1;
	}
	int32_t enum_final_value = -1;
	for (int32_t now_sub_enum_str_index = 0; now_sub_enum_str_index < all_string_list.size(); ++now_sub_enum_str_index) 
	{
		auto enum_value = enum_type_valuepack->second.find(all_string_list[now_sub_enum_str_index]);
		if (enum_value != enum_type_valuepack->second.end())
		{
			if(enum_final_value == -1)
			{
				enum_final_value = enum_value->second;
			}
			else 
			{
				enum_final_value |= enum_value->second;
			}
			
		}
		else
		{
			return -1;
		}
	}
	return enum_final_value;
}


std::string PancyJsonTool::GetEnumName(const std::string &enum_type, int32_t enum_num)
{
	auto enum_type_namepack = enum_name_list.find(enum_type);
	if (enum_type_namepack == enum_name_list.end()) 
	{
		return "";
	}
	auto enum_value = enum_type_namepack->second.find(enum_num);
	if (enum_value == enum_type_namepack->second.end()) 
	{
		//找不到对应的枚举类型，尝试拆解处理枚举或类型
		return DevideEnumValue(enum_num, enum_type_namepack->second);
	}
	return enum_value->second;
}
std::string PancyJsonTool::DevideEnumValue(const int32_t &enum_number, const std::unordered_map<int32_t, std::string> all_enum_member)
{
	std::string real_name = "";
	int check_if_or_same = 0;
	for (auto check_member : all_enum_member) 
	{
		//检查每一个枚举变量是否是可拆分的
		if (enum_number & check_member.first) 
		{
			check_if_or_same |= check_member.first;
			if (real_name == "") 
			{
				real_name = check_member.second;
			}
			else 
			{
				real_name += "|" + check_member.second;
			}
		}
	}
	if (check_if_or_same != enum_number) 
	{
		return "";
	}
	return real_name;
}
PancystarEngine::EngineFailReason PancyJsonTool::GetEnumNameByPointerName(const std::string &enum_pointer_type, std::string &enum_type)
{
	auto enum_member_type_data = enum_pointer_value_map.find(enum_pointer_type);
	if (enum_member_type_data == enum_pointer_value_map.end())
	{
		PancystarEngine::EngineFailReason error_message(0, "could not find enum type by pointer: " + enum_pointer_type);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonTool::GetEnumNameByPointerName", error_message);
		return error_message;
	}
	enum_type = enum_member_type_data->second;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonTool::WriteValueToJson(
	const Json::Value &insert_value,
	const std::string &Json_name
)
{
	std::string save_file_name = Json_name;
	bool if_json_tail = false;
	if (Json_name.size() >= 5)
	{
		std::string tail_name = Json_name.substr(Json_name.size() - 5,5);
		if (tail_name == ".json")
		{
			if_json_tail = true;
		}
	}
	if (!if_json_tail)
	{
		save_file_name += ".json";
	}
	std::unique_ptr<Json::StreamWriter> writer(Jwriter.newStreamWriter());
	FileWrite.open(save_file_name);
	writer->write(insert_value,&FileWrite);
	FileWrite.close();
	return PancystarEngine::succeed;
}
void PancyJsonTool::InitBasicType()
{
	//注册数组类型的数据
	json_type_map[typeid(int64_t*).hash_code()] = json_member_int64_array;
	json_type_map[typeid(uint64_t*).hash_code()] = json_member_uint64_array;
	json_type_map[typeid(int32_t*).hash_code()] = json_member_int32_array;
	json_type_map[typeid(uint32_t*).hash_code()] = json_member_uint32_array;
	json_type_map[typeid(int16_t*).hash_code()] = json_member_int16_array;
	json_type_map[typeid(uint16_t*).hash_code()] = json_member_uint16_array;
	json_type_map[typeid(int8_t*).hash_code()] = json_member_int8_array;
	json_type_map[typeid(uint8_t*).hash_code()] = json_member_uint8_array;
	json_type_map[typeid(float*).hash_code()] = json_member_float_array;
	json_type_map[typeid(double*).hash_code()] = json_member_double_array;
	json_type_map[typeid(bool*).hash_code()] = json_member_bool_array;
	json_type_map[typeid(std::string*).hash_code()] = json_member_string_array;
	//注册普通类型的数据
	json_type_map[typeid(int64_t).hash_code()] = json_member_int64;
	json_type_map[typeid(uint64_t).hash_code()] = json_member_uint64;
	json_type_map[typeid(int32_t).hash_code()] = json_member_int32;
	json_type_map[typeid(uint32_t).hash_code()] = json_member_uint32;
	json_type_map[typeid(int16_t).hash_code()] = json_member_int16;
	json_type_map[typeid(uint16_t).hash_code()] = json_member_uint16;
	json_type_map[typeid(int8_t).hash_code()] = json_member_int8;
	json_type_map[typeid(uint8_t).hash_code()] = json_member_uint8;
	json_type_map[typeid(float).hash_code()] = json_member_float;
	json_type_map[typeid(double).hash_code()] = json_member_double;
	json_type_map[typeid(bool).hash_code()] = json_member_bool;
	json_type_map[typeid(std::string).hash_code()] = json_member_string;
	//注册vector类型的数据
	json_type_map[typeid(std::vector<int64_t>).hash_code()] = json_member_int64_list;
	json_type_map[typeid(std::vector<uint64_t>).hash_code()] = json_member_uint64_list;
	json_type_map[typeid(std::vector<int32_t>).hash_code()] = json_member_int32_list;
	json_type_map[typeid(std::vector<uint32_t>).hash_code()] = json_member_uint32_list;
	json_type_map[typeid(std::vector<int16_t>).hash_code()] = json_member_int16_list;
	json_type_map[typeid(std::vector<uint16_t>).hash_code()] = json_member_uint16_list;
	json_type_map[typeid(std::vector<int8_t>).hash_code()] = json_member_int8_list;
	json_type_map[typeid(std::vector<uint8_t>).hash_code()] = json_member_uint8_list;
	json_type_map[typeid(std::vector<float>).hash_code()] = json_member_float_list;
	json_type_map[typeid(std::vector<double>).hash_code()] = json_member_double_list;
	json_type_map[typeid(std::vector<bool>).hash_code()] = json_member_bool_list;
	json_type_map[typeid(std::vector<std::string>).hash_code()] = json_member_string_list;
}
PancyJsonMemberType PancyJsonTool::GetVariableJsonType(const size_t &variable_type)
{
	auto check_variable_type = json_type_map.find(variable_type);
	if (check_variable_type == json_type_map.end())
	{
		return PancyJsonMemberType::json_member_unknown;
	}
	return check_variable_type->second;
}
PancystarEngine::EngineFailReason PancyJsonTool::SetEnumMemberValue(const std::string &variable_type, void*value_pointer, const int32_t value_data)
{
	auto enum_parse_member = enum_parse_list.find(variable_type);
	if (enum_parse_member == enum_parse_list.end())
	{
		PancystarEngine::EngineFailReason error_message(0, "could not parse enum variable with type: " + variable_type);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonTool::SetEnumMemberValue", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = enum_parse_member->second->SetEnumValue(value_pointer, value_data);
	return check_error;
}
PancystarEngine::EngineFailReason PancyJsonTool::SetEnumArrayValue(const std::string &variable_type, void*value_pointer, const int32_t &enum_offset, const int32_t &enum_data) 
{
	auto enum_parse_member = enum_parse_list.find(variable_type);
	if (enum_parse_member == enum_parse_list.end())
	{
		PancystarEngine::EngineFailReason error_message(0, "could not parse enum variable with type: " + variable_type);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonTool::SetEnumArrayValue", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = enum_parse_member->second->SetEnumArrayValue(value_pointer, enum_offset, enum_data);
	return check_error;
}
PancystarEngine::EngineFailReason PancyJsonTool::SetEnumVectorValue(const std::string &variable_type, void*value_pointer, const int32_t enum_offsetdata, const int32_t value_data)
{
	auto enum_parse_member = enum_parse_list.find(variable_type);
	if (enum_parse_member == enum_parse_list.end())
	{
		PancystarEngine::EngineFailReason error_message(0, "could not parse enum variable with type: " + variable_type);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonTool::SetEnumVectorValue", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = enum_parse_member->second->SetEnumVectorValue(value_pointer, enum_offsetdata, value_data);
	return check_error;
}
PancystarEngine::EngineFailReason PancyJsonTool::GetEnumMemberValue(const std::string &variable_type, void*value_pointer, int32_t &value_data)
{
	auto enum_parse_member = enum_parse_list.find(variable_type);
	if (enum_parse_member == enum_parse_list.end())
	{
		PancystarEngine::EngineFailReason error_message(0, "could not parse enum variable with type: " + variable_type);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonTool::GetEnumMemberValue", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = enum_parse_member->second->GetEnumValue(value_pointer, value_data);
	return check_error;
}
PancystarEngine::EngineFailReason PancyJsonTool::GetEnumArrayValue(const std::string &variable_type, void*value_pointer, const int32_t &enum_offsetdata, int32_t &enum_data_out) 
{
	auto enum_parse_member = enum_parse_list.find(variable_type);
	if (enum_parse_member == enum_parse_list.end())
	{
		PancystarEngine::EngineFailReason error_message(0, "could not parse enum variable with type: " + variable_type);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonTool::GetEnumMemberValue", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = enum_parse_member->second->GetEnumArrayValue(value_pointer, enum_offsetdata, enum_data_out);
	return check_error;
}
PancystarEngine::EngineFailReason PancyJsonTool::GetEnumVectorValue(const std::string &variable_type, void*value_pointer, const int32_t &enum_offsetdata, int32_t &enum_data_out) 
{
	auto enum_parse_member = enum_parse_list.find(variable_type);
	if (enum_parse_member == enum_parse_list.end())
	{
		PancystarEngine::EngineFailReason error_message(0, "could not parse enum variable with type: " + variable_type);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonTool::GetEnumMemberValue", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = enum_parse_member->second->GetEnumVectorValue(value_pointer, enum_offsetdata, enum_data_out);
	return check_error;
}
PancystarEngine::EngineFailReason PancyJsonTool::GetEnumVectorSize(const std::string &variable_type, void*value_pointer, pancy_object_id &enum_size_out)
{
	auto enum_parse_member = enum_parse_list.find(variable_type);
	if (enum_parse_member == enum_parse_list.end())
	{
		PancystarEngine::EngineFailReason error_message(0, "could not parse enum variable with type: " + variable_type);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonTool::GetEnumMemberValue", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = enum_parse_member->second->GetEnumVectorSize(value_pointer, enum_size_out);
	return check_error;
}