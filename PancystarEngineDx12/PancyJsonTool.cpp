#include"PancyJsonTool.h"
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
	std::string name_shader_need[] =
	{
		"VertexShader",
		"PixelShader",
		"GeometryShader",
		"HullShader" ,
		"DominShader",
		"ComputeShader",
	};
	for (int i = 0; i < 6; ++i)
	{
		name_shader_type[i] = name_shader_need[i];
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
PancystarEngine::EngineFailReason PancyJsonTool::SetGlobelVraiable(const std::string &variable_name, const int32_t &variable_value,std::string enum_type)
{
	auto check_if_have = globel_variables.find(variable_name);
	if (check_if_have != globel_variables.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "Add Json Variable " + variable_name + " repeated", PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Globel Vraiable", error_message);
		return PancystarEngine::succeed;
	}
	globel_variables.insert(std::pair<std::string, int32_t>(variable_name, variable_value));
	
	auto enum_list_type = enum_name_list.find(enum_type);
	if (enum_list_type == enum_name_list.end())
	{
		std::unordered_map<int32_t, std::string> new_map;
		enum_name_list.insert(std::pair<std::string, std::unordered_map<int32_t, std::string>>(enum_type, new_map));
		enum_list_type = enum_name_list.find(enum_type);
	}
	enum_list_type->second.insert(std::pair<int32_t, std::string>(variable_value, variable_name));
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
	//枚举数据
	else if (json_type == pancy_json_data_type::json_data_enum)
	{
		if (enum_type_value.type() != Json::ValueType::stringValue)
		{
			int now_type_name = static_cast<int32_t>(enum_type_value.type());
			//json数据对应的类型不是枚举类型
			PancystarEngine::EngineFailReason error_mesage(E_FAIL, "the value of variable NumDescriptors need enum but find " + name_value_type[now_type_name]);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Root Signature json file " + file_name + " error", error_mesage);
			return error_mesage;
		}
		std::string value_enum = enum_type_value.asString();
		variable_value.int_value = GetGlobelVariable(value_enum);
		if (variable_value.int_value == -1)
		{
			//无法将枚举名称转换为有效的枚举值
			PancystarEngine::EngineFailReason error_mesage(E_FAIL, "the enum " + value_enum + " of the variable " + member_name + "can't recognize");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Root Signature json file " + file_name + " error", error_mesage);
			return error_mesage;
		}
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
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonTool::GetJsonShader(
	const std::string &file_name,
	const Json::Value &root_value,
	const Pancy_json_shader_type &json_type,
	std::string &shader_file_name,
	std::string &shader_func_name
)
{
	int32_t shader_type_num = static_cast<uint32_t>(json_type);
	std::string shader_file_param = name_shader_type[shader_type_num];
	std::string shader_func_param = name_shader_type[shader_type_num] + "Func";
	auto shader_file_value = root_value.get(shader_file_param, Json::Value::null);
	auto shader_func_value = root_value.get(shader_func_param, Json::Value::null);
	PancystarEngine::EngineFailReason check_error;
	pancy_json_value new_value;
	check_error = GetJsonMemberData(file_name, shader_file_value, shader_file_param, pancy_json_data_type::json_data_string, new_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	shader_file_name = new_value.string_value;
	check_error = GetJsonMemberData(file_name, shader_func_value, shader_func_param, pancy_json_data_type::json_data_string, new_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	shader_func_name = new_value.string_value;
	return PancystarEngine::succeed;
}
int32_t PancyJsonTool::GetGlobelVariable(const std::string &variable_name)
{
	auto check_data = globel_variables.find(variable_name);
	if (check_data != globel_variables.end())
	{
		return check_data->second;
	}
	else
	{
		return -1;
	}
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
		return "";
	}
	return enum_value->second;
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