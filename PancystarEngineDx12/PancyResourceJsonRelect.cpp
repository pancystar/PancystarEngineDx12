#include"PancyResourceJsonReflect.h"
PancyJsonReflect::PancyJsonReflect()
{
}
PancyJsonReflect::~PancyJsonReflect()
{
}
const std::string PancyJsonReflect::GetParentName(const std::string &name_in)
{
	int32_t parent_value_tail = static_cast<int32_t>(name_in.find_last_of('.'));
	int32_t parent_pointer_tail = static_cast<int32_t>(name_in.rfind("->"));
	auto max_value = max(parent_value_tail, parent_pointer_tail);
	if (max_value <= 0)
	{
		return "";
	}
	return name_in.substr(0, max_value);
}
PancystarEngine::EngineFailReason PancyJsonReflect::AddReflectData(const PancyJsonMemberType &type_data, const std::string &name, const std::string &variable_type_name, void*variable_data, const pancy_object_id &array_size)
{
	JsonReflectData new_data;
	new_data.array_size = array_size;
	new_data.data_type = type_data;
	new_data.data_name = name;
	new_data.parent_name = GetParentName(name);
	new_data.data_pointer = variable_data;
	new_data.data_type_name = variable_type_name;
	//检测节点是否存在重复添加的情况
	if (parent_list.find(name) != parent_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + name + " has been inited before, do not repeat init");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("init Variable of json reflect class", error_message);
		return error_message;
	}
	if (value_map.find(name) != value_map.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + name + " has been inited before, do not repeat init");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("init Variable of json reflect class", error_message);
		return error_message;
	}
	//智能处理父节点为node类型
	std::string next_parent = new_data.parent_name;
	while (next_parent != "")
	{
		std::string parent_next_parent = GetParentName(next_parent);
		if (value_map.find(next_parent) == value_map.end())
		{
			JsonReflectData new_parent_data;
			new_parent_data.data_type = PancyJsonMemberType::json_member_node;
			new_parent_data.data_name = next_parent;
			new_parent_data.parent_name = parent_next_parent;
			new_parent_data.data_pointer = variable_data;
			value_map.insert(std::pair<std::string, JsonReflectData>(next_parent, new_parent_data));
			parent_list.insert(std::pair<std::string, std::string>(next_parent, parent_next_parent));
		}
		next_parent = parent_next_parent;
	}
	parent_list.insert(std::pair<std::string, std::string>(name, new_data.parent_name));
	value_map.insert(std::pair<std::string, JsonReflectData>(name, new_data));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::AddVariable(const std::string &name, void*variable_data, const size_t &variable_type, const std::string &variable_type_name)
{
	PancystarEngine::EngineFailReason check_error;
	PancyJsonMemberType now_variable_type;
	now_variable_type = PancyJsonTool::GetInstance()->GetVariableJsonType(variable_type);
	if (now_variable_type == PancyJsonMemberType::json_member_unknown)
	{
		if (PancyJsonReflectControl::GetInstance()->CheckIfStructMemberInit(variable_type_name))
		{
			now_variable_type = PancyJsonMemberType::json_member_node;
		}
		else if (PancyJsonReflectControl::GetInstance()->CheckIfStructArrayInit(variable_type_name))
		{
			now_variable_type = PancyJsonMemberType::json_member_node_list;
		}
		else
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not recognize type: " + variable_type_name + " check for init or try AddArray()");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::AddVariable", error_message);
			return error_message;
		}
	}
	check_error = AddReflectData(now_variable_type, name, variable_type_name, variable_data,999999999);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::AddArray(const std::string &name, void*variable_data, const size_t &variable_type, const std::string &variable_type_name, const pancy_object_id &array_size)
{
	PancystarEngine::EngineFailReason check_error;
	PancyJsonMemberType now_variable_type;
	now_variable_type = PancyJsonTool::GetInstance()->GetVariableJsonType(variable_type);
	if (now_variable_type == PancyJsonMemberType::json_member_unknown)
	{
		if (PancyJsonReflectControl::GetInstance()->CheckIfStructArrayInit(variable_type_name))
		{
			now_variable_type = PancyJsonMemberType::json_member_node_array;
		}
		else
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not recognize array type: " + variable_type_name + " check for init or try AddVariable()");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::AddArray", error_message);
			return error_message;
		}
	}
	check_error = AddReflectData(now_variable_type, name, variable_type_name, variable_data, array_size);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
void PancyJsonReflect::Create()
{
	InitBasicVariable();
	BuildChildValueMap();
}
void PancyJsonReflect::BuildChildValueMap()
{
	for (auto now_member_value = parent_list.begin(); now_member_value != parent_list.end(); ++now_member_value)
	{
		child_value_list.emplace(now_member_value->first, std::vector<std::string>());
	}
	for (auto now_member_value = parent_list.begin(); now_member_value != parent_list.end(); ++now_member_value)
	{
		if (now_member_value->second != "")
		{
			child_value_list[now_member_value->second].push_back(now_member_value->first);
		}
	}
}
PancystarEngine::EngineFailReason PancyJsonReflect::TranslateStringToEnum(const std::string &basic_string, std::string &enum_type, std::string &enum_value_string, int32_t &enum_value_data)
{
	auto divide_part = basic_string.find("@-->");
	if (divide_part < 0)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not translate enum variable: " + basic_string);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::TranslateStringToEnum", error_message);
		return error_message;
	}
	enum_type = basic_string.substr(0, divide_part);
	enum_value_string = basic_string.substr(divide_part + 4);
	enum_value_data = PancyJsonTool::GetInstance()->GetEnumValue(enum_type, enum_value_string);
	if (enum_value_data == -1)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "haven't init enum variable: " + enum_type + " :: " + enum_value_string);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::TranslateStringToEnum", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
//设置普通变量
PancystarEngine::EngineFailReason PancyJsonReflect::SetIntValue(JsonReflectData &reflect_data, const int64_t &int_value)
{
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_uint8:
	{
		uint8_t* data_pointer = reinterpret_cast<uint8_t*>(reflect_data.data_pointer);
		*data_pointer = static_cast<uint8_t>(int_value);
	}
	break;
	case PancyJsonMemberType::json_member_uint16:
	{
		uint16_t* data_pointer = reinterpret_cast<uint16_t*>(reflect_data.data_pointer);
		*data_pointer = static_cast<uint16_t>(int_value);
	}
	break;
	case PancyJsonMemberType::json_member_uint32:
	{
		uint32_t* data_pointer = reinterpret_cast<uint32_t*>(reflect_data.data_pointer);
		*data_pointer = static_cast<uint32_t>(int_value);
	}
	break;
	case PancyJsonMemberType::json_member_uint64:
	{
		uint64_t* data_pointer = reinterpret_cast<uint64_t*>(reflect_data.data_pointer);
		*data_pointer = static_cast<uint64_t>(int_value);
	}
	break;
	case PancyJsonMemberType::json_member_int8:
	{
		int8_t* data_pointer = reinterpret_cast<int8_t*>(reflect_data.data_pointer);
		*data_pointer = static_cast<int8_t>(int_value);
	}
	break;
	case PancyJsonMemberType::json_member_int16:
	{
		int16_t* data_pointer = reinterpret_cast<int16_t*>(reflect_data.data_pointer);
		*data_pointer = static_cast<int16_t>(int_value);
	}
	break;
	case PancyJsonMemberType::json_member_int32:
	{
		int32_t* data_pointer = reinterpret_cast<int32_t*>(reflect_data.data_pointer);
		*data_pointer = static_cast<int32_t>(int_value);
	}
	break;
	case PancyJsonMemberType::json_member_int64:
	{
		int64_t* data_pointer = reinterpret_cast<int64_t*>(reflect_data.data_pointer);
		*data_pointer = static_cast<int64_t>(int_value);
	}
	break;
	default:
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a int value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntValue", error_message);
		return error_message;
	}
	break;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SetDoubleValue(JsonReflectData &reflect_data, const double &double_value)
{
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_float:
	{
		float* data_pointer = reinterpret_cast<float*>(reflect_data.data_pointer);
		*data_pointer = static_cast<float>(double_value);
	}
	break;
	case PancyJsonMemberType::json_member_double:
	{
		double* data_pointer = reinterpret_cast<double*>(reflect_data.data_pointer);
		*data_pointer = static_cast<double>(double_value);
	}
	break;
	default:
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a double value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetDoubleValue", error_message);
		return error_message;
	}
	break;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SetBoolValue(JsonReflectData &reflect_data, const bool &bool_value)
{
	if (reflect_data.data_type != PancyJsonMemberType::json_member_bool)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a bool value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetBoolValue", error_message);
		return error_message;
	}
	bool* data_pointer = reinterpret_cast<bool*>(reflect_data.data_pointer);
	*data_pointer = bool_value;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SetStringValue(JsonReflectData &reflect_data, const std::string &string_value)
{
	PancystarEngine::EngineFailReason check_error;
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_string:
	{
		//普通的字符串
		std::string* data_pointer = reinterpret_cast<std::string*>(reflect_data.data_pointer);
		*data_pointer = static_cast<std::string>(string_value);
	}
	break;
	case PancyJsonMemberType::json_member_enum:
	{
		//枚举变量字符串
		std::string value_enum_type;
		std::string value_enum_value_name;
		//检测枚举变量是否填写正确
		if (string_value.find(reflect_data.data_type_name) < 0)
		{
			//枚举类型不匹配
			PancystarEngine::EngineFailReason error_message(E_FAIL, "enum member : " + reflect_data.data_name + " write wrong, need type: " + reflect_data.data_type_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetStringValue", error_message);
			return error_message;
		}
		//根据枚举变量的类型，将枚举变量赋值
		int32_t enum_value;
		check_error = TranslateStringToEnum(string_value, value_enum_type, value_enum_value_name, enum_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		auto check_error = PancyJsonTool::GetInstance()->SetEnumMemberValue(value_enum_type, reflect_data.data_pointer, enum_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//int32_t* data_pointer = reinterpret_cast<int32_t*>(reflect_data.data_pointer);
		//*data_pointer = enum_value;
	}
	break;
	default:
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a string value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetStringValue", error_message);
		return error_message;
		break;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SetIntArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const int64_t &int_value)
{
	PancystarEngine::EngineFailReason check_error;
	JsonReflectData single_member;
	//检查数据是不是vector类型的数组
	bool if_vector_value = true;
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_int8_list:
	{
		check_error = SetVectorValue<int8_t>(reflect_data, offset_value, int_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int16_list:
	{
		check_error = SetVectorValue<int16_t>(reflect_data, offset_value, int_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int32_list:
	{
		check_error = SetVectorValue<int32_t>(reflect_data, offset_value, int_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int64_list:
	{
		check_error = SetVectorValue<int64_t>(reflect_data, offset_value, int_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint8_list:
	{
		check_error = SetVectorValue<uint8_t>(reflect_data, offset_value, int_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint16_list:
	{
		check_error = SetVectorValue<uint16_t>(reflect_data, offset_value, int_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint32_list:
	{
		check_error = SetVectorValue<uint32_t>(reflect_data, offset_value, int_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint64_list:
	{
		check_error = SetVectorValue<uint64_t>(reflect_data, offset_value, int_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	default:
	{
		if_vector_value = false;
	}
	}
	if (if_vector_value)
	{
		//已经成功当作vector处理，退出
		return PancystarEngine::succeed;
	}
	//先检查数组是否越界
	if (offset_value >= reflect_data.array_size)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the array variable: " + reflect_data.data_name + " size only have " + std::to_string(reflect_data.array_size) + " could not get index" + std::to_string(offset_value));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
		return error_message;
	}
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_int8_array:
		single_member.data_type = PancyJsonMemberType::json_member_int8;
		single_member.data_pointer = reinterpret_cast<int8_t*>(reflect_data.data_pointer) + offset_value;
		break;
	case PancyJsonMemberType::json_member_int16_array:
		single_member.data_type = PancyJsonMemberType::json_member_int16;
		single_member.data_pointer = reinterpret_cast<int16_t*>(reflect_data.data_pointer) + offset_value;
		break;
	case PancyJsonMemberType::json_member_int32_array:
		single_member.data_type = PancyJsonMemberType::json_member_int32;
		single_member.data_pointer = reinterpret_cast<int32_t*>(reflect_data.data_pointer) + offset_value;
		break;
	case PancyJsonMemberType::json_member_int64_array:
		single_member.data_type = PancyJsonMemberType::json_member_int64;
		single_member.data_pointer = reinterpret_cast<int64_t*>(reflect_data.data_pointer) + offset_value;
		break;
	case PancyJsonMemberType::json_member_uint8_array:
		single_member.data_type = PancyJsonMemberType::json_member_uint8;
		single_member.data_pointer = reinterpret_cast<uint8_t*>(reflect_data.data_pointer) + offset_value;
		break;
	case PancyJsonMemberType::json_member_uint16_array:
		single_member.data_type = PancyJsonMemberType::json_member_uint16;
		single_member.data_pointer = reinterpret_cast<uint16_t*>(reflect_data.data_pointer) + offset_value;
		break;
	case PancyJsonMemberType::json_member_uint32_array:
		single_member.data_type = PancyJsonMemberType::json_member_uint32;
		single_member.data_pointer = reinterpret_cast<uint32_t*>(reflect_data.data_pointer) + offset_value;
		break;
	case PancyJsonMemberType::json_member_uint64_array:
		single_member.data_type = PancyJsonMemberType::json_member_uint64;
		single_member.data_pointer = reinterpret_cast<uint64_t*>(reflect_data.data_pointer) + offset_value;
		break;
	default:
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a int array value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
		return error_message;
		break;
	}

	check_error = SetIntValue(single_member, int_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SetDoubleArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const double &double_value)
{
	PancystarEngine::EngineFailReason check_error;
	JsonReflectData single_member;
	//检查数据是不是vector类型的数组
	bool if_vector_value = true;
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_double_list:
	{
		check_error = SetVectorValue<double>(reflect_data, offset_value, double_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_float_list:
	{
		check_error = SetVectorValue<float>(reflect_data, offset_value, double_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	default:
	{
		if_vector_value = false;
	}
	}
	if (if_vector_value)
	{
		//已经成功当作vector处理，退出
		return PancystarEngine::succeed;
	}
	//先检查数组是否越界
	if (offset_value >= reflect_data.array_size)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the array variable: " + reflect_data.data_name + " size only have " + std::to_string(reflect_data.array_size) + " could not get index" + std::to_string(offset_value));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
		return error_message;
	}
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_double_array:
		single_member.data_type = PancyJsonMemberType::json_member_double;
		single_member.data_pointer = reinterpret_cast<double*>(reflect_data.data_pointer) + offset_value;
		break;
	case PancyJsonMemberType::json_member_float_array:
		single_member.data_type = PancyJsonMemberType::json_member_float;
		single_member.data_pointer = reinterpret_cast<float*>(reflect_data.data_pointer) + offset_value;
		break;
	default:
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a double array value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
		return error_message;
		break;
	}
	check_error = SetDoubleValue(single_member, double_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SetBoolArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const bool &bool_value)
{
	JsonReflectData single_member;
	//检查数据是不是vector类型的数组
	if (reflect_data.data_type == PancyJsonMemberType::json_member_bool_list)
	{
		return SetVectorValue<bool>(reflect_data, offset_value, bool_value);
	}
	if (reflect_data.data_type != PancyJsonMemberType::json_member_bool_array)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a bool array value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
		return error_message;
	}
	//先检查数组是否越界
	if (offset_value >= reflect_data.array_size)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the array variable: " + reflect_data.data_name + " size only have " + std::to_string(reflect_data.array_size) + " could not get index" + std::to_string(offset_value));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
		return error_message;
	}
	single_member.data_type = PancyJsonMemberType::json_member_bool;
	single_member.data_pointer = reinterpret_cast<bool*>(reflect_data.data_pointer) + offset_value;
	PancystarEngine::EngineFailReason check_error;
	check_error = SetBoolValue(single_member, bool_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SetStringArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const std::string &string_value)
{
	PancystarEngine::EngineFailReason check_error;
	JsonReflectData single_member;
	//检查数据是不是vector类型的数组
	if (reflect_data.data_type == PancyJsonMemberType::json_member_string_list)
	{
		return SetVectorValue<std::string>(reflect_data, offset_value, string_value);
	}
	else if (reflect_data.data_type == PancyJsonMemberType::json_member_enum_list)
	{
		//todo:enum或运算
		std::string value_enum_type;
		std::string value_enum_value_name;
		int32_t enum_value;
		check_error = TranslateStringToEnum(string_value, value_enum_type, value_enum_value_name, enum_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		std::string now_dealed_enum_type;
		PancyJsonTool::GetInstance()->GetEnumNameByPointerName(reflect_data.data_type_name, now_dealed_enum_type);
		if (now_dealed_enum_type != value_enum_type)
		{
			//枚举类型不匹配
			PancystarEngine::EngineFailReason error_message(E_FAIL, "enum member: " + reflect_data.data_name + " :type dismatch: " + now_dealed_enum_type + " Dismatch： " + value_enum_type);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetStringArrayValue", error_message);
			return error_message;
		}
		return PancyJsonTool::GetInstance()->SetEnumVectorValue(value_enum_type, reflect_data.data_pointer, offset_value, enum_value);
	}
	//先检查数组是否越界
	if (offset_value >= reflect_data.array_size)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the array variable: " + reflect_data.data_name + " size only have " + std::to_string(reflect_data.array_size) + " could not get index" + std::to_string(offset_value));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
		return error_message;
	}
	if (reflect_data.data_type == PancyJsonMemberType::json_member_string_array)
	{
		single_member.data_type = PancyJsonMemberType::json_member_string;
		single_member.data_pointer = reinterpret_cast<std::string*>(reflect_data.data_pointer) + offset_value;
		PancystarEngine::EngineFailReason check_error;
		check_error = SetStringValue(single_member, string_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	else if (reflect_data.data_type == PancyJsonMemberType::json_member_enum_array)
	{
		std::string value_enum_type, value_enum_value_name;
		int32_t enum_value;
		//先将字符串数据解析为枚举数据
		check_error = TranslateStringToEnum(string_value, value_enum_type, value_enum_value_name, enum_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//获取数组枚举变量的真实变量类型
		std::string member_enum_type_name;
		check_error = PancyJsonTool::GetInstance()->GetEnumNameByPointerName(reflect_data.data_type_name, member_enum_type_name);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//检测类型是否匹配
		if (value_enum_type != member_enum_type_name)
		{
			//枚举类型不匹配
			PancystarEngine::EngineFailReason error_message(E_FAIL, "enum member: " + reflect_data.data_name + " :type dismatch: " + value_enum_type + " Dismatch： " + reflect_data.data_type_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetStringArrayValue", error_message);
			return error_message;
		}
		auto check_error = PancyJsonTool::GetInstance()->SetEnumArrayValue(value_enum_type, reflect_data.data_pointer, offset_value, enum_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	else
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a string array value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetStringArrayValue", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SetNodeArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const Json::Value &now_child_value)
{
	PancystarEngine::EngineFailReason check_error;
	//根据节点的类型创建一个新的处理类来解析节点数据
	;
	auto child_reflect_pointer = PancyJsonReflectControl::GetInstance()->GetJsonReflectByArray(reflect_data.data_type_name);
	if (child_reflect_pointer == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find struct node reflect: " + reflect_data.data_type_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetNodeArrayValue", error_message);
		return error_message;
	}
	check_error = child_reflect_pointer->LoadFromJsonNode("reflect_data", reflect_data.data_name + "array:" + std::to_string(offset_value), now_child_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	pancy_resource_size now_data_type_size;
	check_error = PancyJsonReflectControl::GetInstance()->GetReflectDataSizeByArray(reflect_data.data_type_name, now_data_type_size);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	if (reflect_data.data_type == PancyJsonMemberType::json_member_node_list)
	{
		//vector类型的节点数组
		check_error = child_reflect_pointer->CopyVectorData(reflect_data.data_pointer, reflect_data.data_type_name, offset_value, now_data_type_size);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	else if (reflect_data.data_type == PancyJsonMemberType::json_member_node_array)
	{
		//先检查数组是否越界
		if (offset_value >= reflect_data.array_size)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "the array variable: " + reflect_data.data_name + " size only have " + std::to_string(reflect_data.array_size) + " could not get index" + std::to_string(offset_value));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetNodeArrayValue", error_message);
			return error_message;
		}
		//先根据偏移量将节点数组的指针偏移到正确的位置
		char* now_array_pointer = reinterpret_cast<char*>(reflect_data.data_pointer);
		now_array_pointer = now_array_pointer + offset_value * now_data_type_size;
		//将处理完的数据拷贝到指定位置
		child_reflect_pointer->CopyMemberData(now_array_pointer, reflect_data.data_type_name, now_data_type_size);
	}
	else
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a node array value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetNodeArrayValue", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
//设置数组变量
PancystarEngine::EngineFailReason PancyJsonReflect::SetArrayValue(JsonReflectData &reflect_data, const Json::Value &now_child_value)
{
	PancystarEngine::EngineFailReason check_error;
	//数组变量对应的大小变量不需要进行存储(json中会有额外的变量映射过来)
	for (Json::ArrayIndex array_index = 0; array_index < now_child_value.size(); ++array_index)
	{
		//先检查数组是否越界
		if (array_index >= reflect_data.array_size)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "the array variable: " + reflect_data.data_name + " size only have " + std::to_string(reflect_data.array_size) + " could not get index" + std::to_string(array_index));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
			return error_message;
		}
		switch (now_child_value[array_index].type())
		{
		case Json::intValue:
		{
			check_error = SetIntArrayValue(reflect_data, array_index, now_child_value[array_index].asInt64());
		}
		break;
		case Json::uintValue:
		{
			check_error = SetIntArrayValue(reflect_data, array_index, now_child_value[array_index].asInt64());
		}
		break;
		case Json::realValue:
		{
			check_error = SetDoubleArrayValue(reflect_data, array_index, now_child_value[array_index].asDouble());
		}
		break;
		case Json::booleanValue:
		{
			check_error = SetBoolArrayValue(reflect_data, array_index, now_child_value[array_index].asBool());
		}
		break;
		case Json::stringValue:
		{
			check_error = SetStringArrayValue(reflect_data, array_index, now_child_value[array_index].asString());
		}
		break;
		case Json::objectValue:
		{
			check_error = SetNodeArrayValue(reflect_data, array_index, now_child_value[array_index]);
		}
		break;
		//todo:节点数组
		default:
			break;
		}
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::LoadFromJsonFile(const std::string &Json_file)
{
	Json::Value jsonRoot;
	auto check_error = PancyJsonTool::GetInstance()->LoadJsonFile(Json_file, jsonRoot);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return LoadFromJsonNode("reflect_data", Json_file, jsonRoot);
}
PancystarEngine::EngineFailReason PancyJsonReflect::LoadFromJsonMemory(const std::string &value_name, const Json::Value &root_value)
{
	return LoadFromJsonNode("reflect_data", value_name, root_value);
}

//获取普通变量
PancystarEngine::EngineFailReason PancyJsonReflect::SaveSingleValueMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value)
{
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_int8:
	{
		int8_t* now_value_pointer = reinterpret_cast<int8_t*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<int64_t>(*now_value_pointer));
		break;
	}
	case PancyJsonMemberType::json_member_int16:
	{
		int16_t* now_value_pointer = reinterpret_cast<int16_t*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<int64_t>(*now_value_pointer));
		break;
	}
	case PancyJsonMemberType::json_member_int32:
	{
		int32_t* now_value_pointer = reinterpret_cast<int32_t*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<int64_t>(*now_value_pointer));
		break;
	}
	case PancyJsonMemberType::json_member_int64:
	{
		int64_t* now_value_pointer = reinterpret_cast<int64_t*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), *now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_uint8:
	{
		uint8_t* now_value_pointer = reinterpret_cast<uint8_t*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<uint64_t>(*now_value_pointer));
		break;
	}
	case PancyJsonMemberType::json_member_uint16:
	{
		uint16_t* now_value_pointer = reinterpret_cast<uint16_t*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<uint64_t>(*now_value_pointer));
		break;
	}
	case PancyJsonMemberType::json_member_uint32:
	{
		uint32_t* now_value_pointer = reinterpret_cast<uint32_t*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<uint64_t>(*now_value_pointer));
		break;
	}
	case PancyJsonMemberType::json_member_uint64:
	{
		uint64_t* now_value_pointer = reinterpret_cast<uint64_t*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), *now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_float:
	{
		float* now_value_pointer = reinterpret_cast<float*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<double>(*now_value_pointer));
		break;
	}
	case PancyJsonMemberType::json_member_double:
	{
		double* now_value_pointer = reinterpret_cast<double*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), *now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_bool:
	{
		bool* now_value_pointer = reinterpret_cast<bool*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), *now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_string:
	{
		string* now_value_pointer = reinterpret_cast<string*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), *now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_enum:
	{
		PancystarEngine::EngineFailReason check_error;
		int32_t enum_value_int = -1;
		check_error = PancyJsonTool::GetInstance()->GetEnumMemberValue(reflect_data.data_type_name, reflect_data.data_pointer, enum_value_int);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		std::string enum_name_final = PancyJsonTool::GetInstance()->GetEnumName(reflect_data.data_type_name, enum_value_int);
		if (enum_name_final == "")
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not parse enum value to string: " + reflect_data.data_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveSingleValueMemberToJson", error_message);
			return error_message;
		}
		enum_name_final = reflect_data.data_type_name + "@-->" + enum_name_final;
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), enum_name_final);
		break;
	}
	default:
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not recognize JSON reflect type: " + reflect_data.data_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveSingleValueMemberToJson", error_message);
		return error_message;
	}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SaveArrayEnumMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id array_used_size = 0;
	check_error = GetArrayDataSize(reflect_data, array_used_size);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	for (pancy_object_id now_enum_index = 0; now_enum_index < array_used_size; ++now_enum_index)
	{
		std::string member_enum_type_name;
		check_error = PancyJsonTool::GetInstance()->GetEnumNameByPointerName(reflect_data.data_type_name, member_enum_type_name);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		int32_t enum_value_int = -1;
		check_error = PancyJsonTool::GetInstance()->GetEnumArrayValue(member_enum_type_name, reflect_data.data_pointer, now_enum_index, enum_value_int);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		std::string enum_name_final = PancyJsonTool::GetInstance()->GetEnumName(member_enum_type_name, enum_value_int);
		if (enum_name_final == "")
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not parse enum value to string: " + reflect_data.data_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveArrayEnumMemberToJson", error_message);
			return error_message;
		}
		enum_name_final = member_enum_type_name + "@-->" + enum_name_final;
		PancyJsonTool::GetInstance()->AddJsonArrayValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), enum_name_final);
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SaveVectorEnumMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id vector_size = 0;
	check_error = PancyJsonTool::GetInstance()->GetEnumVectorSize(reflect_data.data_type_name, reflect_data.data_pointer, vector_size);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	for (pancy_object_id now_enum_index = 0; now_enum_index < vector_size; ++now_enum_index)
	{
		std::string member_enum_type_name;
		check_error = PancyJsonTool::GetInstance()->GetEnumNameByPointerName(reflect_data.data_type_name, member_enum_type_name);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		int32_t enum_value_int = -1;
		check_error = PancyJsonTool::GetInstance()->GetEnumVectorValue(reflect_data.data_type_name, reflect_data.data_pointer, now_enum_index, enum_value_int);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		std::string enum_name_final = PancyJsonTool::GetInstance()->GetEnumName(reflect_data.data_type_name, enum_value_int);
		if (enum_name_final == "")
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not parse enum value to string: " + reflect_data.data_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveVectorEnumMemberToJson", error_message);
			return error_message;
		}
		enum_name_final = member_enum_type_name + "@-->" + enum_name_final;
		PancyJsonTool::GetInstance()->AddJsonArrayValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), enum_name_final);
	}
	return PancystarEngine::succeed;
}

PancystarEngine::EngineFailReason PancyJsonReflect::SaveToJsonMemory(Json::Value &root_value)
{
	auto child_value_member = child_value_list.find("reflect_data");
	if (child_value_member == child_value_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find JSON reflect variable: reflect_data");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveToJsonMemory", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error;
	for (int32_t child_value_index = 0; child_value_index < child_value_member->second.size(); ++child_value_index)
	{
		std::string now_child_name = child_value_member->second[child_value_index];
		check_error = SaveToJsonNode(now_child_name, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SaveToJsonFile(const std::string &json_name)
{
	Json::Value file_value;
	auto check_error = SaveToJsonMemory(file_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancyJsonTool::GetInstance()->WriteValueToJson(file_value, json_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SaveToJsonNode(const std::string &parent_name, Json::Value &root_value)
{
	PancystarEngine::EngineFailReason check_error;
	//Json::Value root_value;
	auto now_reflect_data = value_map.find(parent_name);
	if (now_reflect_data == value_map.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find JSON reflect variable: " + parent_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveToJsonNode", error_message);
		return error_message;
	}
	switch (now_reflect_data->second.data_type)
	{
	case PancyJsonMemberType::json_member_int8:
	case PancyJsonMemberType::json_member_int16:
	case PancyJsonMemberType::json_member_int32:
	case PancyJsonMemberType::json_member_int64:
	case PancyJsonMemberType::json_member_uint8:
	case PancyJsonMemberType::json_member_uint16:
	case PancyJsonMemberType::json_member_uint32:
	case PancyJsonMemberType::json_member_uint64:
	case PancyJsonMemberType::json_member_float:
	case PancyJsonMemberType::json_member_double:
	case PancyJsonMemberType::json_member_bool:
	case PancyJsonMemberType::json_member_string:
	case PancyJsonMemberType::json_member_enum:
	{
		check_error = SaveSingleValueMemberToJson(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int8_array:
	{
		check_error = SaveArrayValueMemberToJson<int8_t, int64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int16_array:
	{
		check_error = SaveArrayValueMemberToJson<int16_t, int64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int32_array:
	{
		check_error = SaveArrayValueMemberToJson<int32_t, int64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int64_array:
	{
		check_error = SaveArrayValueMemberToJson<int64_t, int64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint8_array:
	{
		check_error = SaveArrayValueMemberToJson<uint8_t, uint64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint16_array:
	{
		check_error = SaveArrayValueMemberToJson<uint16_t, uint64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint32_array:
	{
		check_error = SaveArrayValueMemberToJson<uint32_t, uint64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint64_array:
	{
		check_error = SaveArrayValueMemberToJson<uint64_t, uint64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_float_array:
	{
		check_error = SaveArrayValueMemberToJson<float, double>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_double_array:
	{
		check_error = SaveArrayValueMemberToJson<double, double>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_bool_array:
	{
		check_error = SaveArrayValueMemberToJson<bool, bool>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_string_array:
	{
		check_error = SaveArrayValueMemberToJson<std::string, std::string>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_enum_array:
	{
		check_error = SaveArrayEnumMemberToJson(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int8_list:
	{
		check_error = SaveVectorValueMemberToJson<int8_t, int64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int16_list:
	{
		check_error = SaveVectorValueMemberToJson<int16_t, int64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int32_list:
	{
		check_error = SaveVectorValueMemberToJson<int32_t, int64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int64_list:
	{
		check_error = SaveVectorValueMemberToJson<int64_t, int64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint8_list:
	{
		check_error = SaveVectorValueMemberToJson<uint8_t, uint64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint16_list:
	{
		check_error = SaveVectorValueMemberToJson<uint16_t, uint64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint32_list:
	{
		check_error = SaveVectorValueMemberToJson<uint32_t, uint64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint64_list:
	{
		check_error = SaveVectorValueMemberToJson<uint64_t, uint64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_float_list:
	{
		check_error = SaveVectorValueMemberToJson<float, double>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_double_list:
	{
		check_error = SaveVectorValueMemberToJson<double, double>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_bool_list:
	{
		check_error = SaveVectorValueMemberToJson<bool, bool>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_string_list:
	{
		check_error = SaveVectorValueMemberToJson<std::string, std::string>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_enum_list:
	{
		check_error = SaveVectorEnumMemberToJson(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_node:
	{
		auto child_value_member = child_value_list.find(parent_name);
		if (child_value_member == child_value_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find JSON reflect variable: " + parent_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveToJsonNode", error_message);
			return error_message;
		}
		Json::Value struct_root_value;
		for (int32_t child_value_index = 0; child_value_index < child_value_member->second.size(); ++child_value_index)
		{
			std::string now_child_name = child_value_member->second[child_value_index];
			Json::Value child_root_value;
			check_error = SaveToJsonNode(now_child_name, struct_root_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(now_reflect_data->second.data_name), struct_root_value);
		break;
	}
	case PancyJsonMemberType::json_member_node_array:
	{
		//根据节点的类型获取对应的处理类来解析节点数据
		auto child_reflect_pointer = PancyJsonReflectControl::GetInstance()->GetJsonReflectByArray(now_reflect_data->second.data_type_name);
		if (child_reflect_pointer == NULL)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find struct node reflect: " + now_reflect_data->second.data_type_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveToJsonNode", error_message);
			return error_message;
		}
		pancy_resource_size child_size_pointer;
		check_error = PancyJsonReflectControl::GetInstance()->GetReflectDataSizeByArray(now_reflect_data->second.data_type_name, child_size_pointer);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		pancy_object_id array_size = 0;
		check_error = GetArrayDataSize(now_reflect_data->second, array_size);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		for (pancy_object_id node_index_array = 0; node_index_array < array_size; ++node_index_array)
		{
			Json::Value array_member_value;
			check_error = child_reflect_pointer->ResetMemoryByArrayData(now_reflect_data->second.data_pointer, now_reflect_data->second.data_type_name, node_index_array, child_size_pointer);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			check_error = child_reflect_pointer->SaveToJsonMemory(array_member_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			PancyJsonTool::GetInstance()->AddJsonArrayValue(root_value, TranslateFullNameToRealName(now_reflect_data->second.data_name), array_member_value);
		}
		break;
	}
	case PancyJsonMemberType::json_member_node_list:
	{
		auto child_reflect_pointer = PancyJsonReflectControl::GetInstance()->GetJsonReflectByArray(now_reflect_data->second.data_type_name);
		if (child_reflect_pointer == NULL)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find struct node reflect: " + now_reflect_data->second.data_type_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveToJsonNode", error_message);
			return error_message;
		}
		pancy_resource_size child_size_pointer;
		check_error = PancyJsonReflectControl::GetInstance()->GetReflectDataSizeByArray(now_reflect_data->second.data_type_name, child_size_pointer);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//根据节点的类型获取对应的处理类来解析节点数据
		pancy_object_id vector_size = 0;
		check_error = child_reflect_pointer->GetVectorDataSize(now_reflect_data->second.data_pointer, now_reflect_data->second.data_type_name, vector_size);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		for (pancy_object_id node_index_array = 0; node_index_array < vector_size; ++node_index_array)
		{
			Json::Value array_member_value;
			check_error = child_reflect_pointer->ResetMemoryByVectorData(now_reflect_data->second.data_pointer, now_reflect_data->second.data_type_name, node_index_array, child_size_pointer);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			check_error = child_reflect_pointer->SaveToJsonMemory(array_member_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			PancyJsonTool::GetInstance()->AddJsonArrayValue(root_value, TranslateFullNameToRealName(now_reflect_data->second.data_name), array_member_value);
		}
		break;
	}
	default:
		break;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::LoadFromJsonArray(const std::string &value_name, const Json::Value &root_value)
{
	auto now_reflect_data = value_map.find(value_name);
	if (now_reflect_data == value_map.end())
	{
		//未找到对应的反射数据
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find JSON reflect variable: " + value_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::LoadFromJson", error_message);
		return error_message;
	}
	return SetArrayValue(now_reflect_data->second, root_value);
}
PancystarEngine::EngineFailReason PancyJsonReflect::LoadFromJsonNode(const std::string &parent_name, const std::string &value_name, const Json::Value &root_value)
{
	PancystarEngine::EngineFailReason check_error;
	std::vector<std::string> all_member_name = root_value.getMemberNames();
	std::vector<Json::ValueType> check_value;
	for (int member_index = 0; member_index < all_member_name.size(); ++member_index)
	{
		//获取子节点的json数据
		Json::Value now_child_value = root_value.get(all_member_name[member_index], Json::Value::null);
		//查找对应节点的反射信息
		std::string now_combine_value_common = parent_name;//通过.链接的结构体信息
		std::string now_combine_value_pointer = parent_name;//通过->链接的结构体指针信息
		if (parent_name != "")
		{
			now_combine_value_common += ".";
			now_combine_value_pointer += "->";
		}
		now_combine_value_common += all_member_name[member_index];
		now_combine_value_pointer += all_member_name[member_index];
		//存储当前节点的真正的名称，由于有些node依靠.来标记子节点，而有些需要依靠->标记
		std::string now_node_name = now_combine_value_common;
		auto now_reflect_data = value_map.find(now_combine_value_common);
		if (now_reflect_data == value_map.end())
		{
			now_reflect_data = value_map.find(now_combine_value_pointer);
			if (now_reflect_data == value_map.end())
			{
				//未找到对应的反射数据
				PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find JSON reflect variable: " + now_combine_value_common + " while load json node " + value_name);
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::LoadFromJson", error_message);
				return error_message;
			}
			now_node_name = now_combine_value_pointer;
		}
		auto json_data_type = now_child_value.type();
		//根据反射数据的类型，决定数据的载入方式
		switch (json_data_type)
		{
		case Json::nullValue:
		{
			//损坏的数据
			PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + all_member_name[member_index] + " could not be recognized by json tool");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::LoadFromJson", error_message);
			return error_message;
		}
		break;
		case Json::intValue:
		{
			int64_t int_data = now_child_value.asInt64();
			check_error = SetIntValue(now_reflect_data->second, int_data);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		break;
		case Json::uintValue:
		{
			int64_t int_data = now_child_value.asInt64();
			check_error = SetIntValue(now_reflect_data->second, int_data);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		break;
		case Json::realValue:
		{
			double double_value = now_child_value.asDouble();
			check_error = SetDoubleValue(now_reflect_data->second, double_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		break;
		case Json::stringValue:
		{
			std::string string_value = now_child_value.asString();
			check_error = SetStringValue(now_reflect_data->second, string_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		break;
		case Json::booleanValue:
		{
			bool bool_value = now_child_value.asDouble();
			check_error = SetBoolValue(now_reflect_data->second, bool_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		break;
		case Json::arrayValue:
		{
			check_error = LoadFromJsonArray(now_node_name, now_child_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		break;
		case Json::objectValue:
		{
			//节点数据
			check_error = LoadFromJsonNode(now_node_name, value_name, now_child_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		break;
		default:
			break;
		}
	}
	return PancystarEngine::succeed;
}
std::string PancyJsonReflect::TranslateFullNameToRealName(const std::string &full_name)
{
	size_t self_offset = 1;
	auto offset_value = full_name.rfind(".");
	auto offset_pointer = full_name.rfind("->");
	if (offset_value < offset_pointer)
	{
		self_offset = 2;
		offset_value = offset_pointer;
	}
	std::string real_name;
	if (offset_value != -1)
	{
		real_name = full_name.substr(self_offset + offset_value, real_name.size() - (self_offset + offset_value));
	}
	else
	{
		real_name = full_name;
	}
	return real_name;
}
PancystarEngine::EngineFailReason PancyJsonReflect::GetArrayDataSize(const JsonReflectData &reflect_data, pancy_object_id &size_out)
{
	auto check_array_size_name = array_real_size_map.find(reflect_data.data_name);
	if (check_array_size_name == array_real_size_map.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "array: " + reflect_data.data_name +"do not have size variable");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::GetArrayDataSize", error_message);
		return error_message;
	}
	auto check_array_size_value = value_map.find(check_array_size_name->second);
	if (check_array_size_name == array_real_size_map.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find array size variable: " + check_array_size_name->second);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::GetArrayDataSize", error_message);
		return error_message;
	}
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_int8:
	{
		int8_t* now_value_pointer = reinterpret_cast<int8_t*>(reflect_data.data_pointer);
		size_out = static_cast<pancy_object_id>(*now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_int16:
	{
		int16_t* now_value_pointer = reinterpret_cast<int16_t*>(reflect_data.data_pointer);
		size_out = static_cast<pancy_object_id>(*now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_int32:
	{
		int32_t* now_value_pointer = reinterpret_cast<int32_t*>(reflect_data.data_pointer);
		size_out = static_cast<pancy_object_id>(*now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_int64:
	{
		int64_t* now_value_pointer = reinterpret_cast<int64_t*>(reflect_data.data_pointer);
		size_out = static_cast<pancy_object_id>(*now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_uint8:
	{
		uint8_t* now_value_pointer = reinterpret_cast<uint8_t*>(reflect_data.data_pointer);
		size_out = static_cast<pancy_object_id>(*now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_uint16:
	{
		uint16_t* now_value_pointer = reinterpret_cast<uint16_t*>(reflect_data.data_pointer);
		size_out = static_cast<pancy_object_id>(*now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_uint32:
	{
		uint32_t* now_value_pointer = reinterpret_cast<uint32_t*>(reflect_data.data_pointer);
		size_out = static_cast<pancy_object_id>(*now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_uint64:
	{
		uint64_t* now_value_pointer = reinterpret_cast<uint64_t*>(reflect_data.data_pointer);
		size_out = static_cast<pancy_object_id>(*now_value_pointer);
		break;
	}
	default:
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "array size variable: " + check_array_size_name->second +" is not an int value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::GetArrayDataSize", error_message);
		return error_message;
	}
	}
	if (size_out > reflect_data.array_size) 
	{
		//数组使用大小越界
		PancystarEngine::EngineFailReason error_message(E_FAIL, "array size out of range: " + check_array_size_name->second);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::GetArrayDataSize", error_message);
		size_out = 0;
		return error_message;
	}
	return PancystarEngine::succeed;
}

PancyJsonReflectControl::PancyJsonReflectControl()
{
}
PancyJsonReflectControl::~PancyJsonReflectControl()
{
	for (auto reflect_parse_object = refelct_map.begin(); reflect_parse_object != refelct_map.end(); ++reflect_parse_object)
	{
		delete reflect_parse_object->second;
	}
}
PancystarEngine::EngineFailReason PancyJsonReflectControl::GetReflectDataSizeByMember(const std::string &name, pancy_resource_size &size)
{
	auto now_reflect_data = refelct_data_desc_size_map.find(name);
	if (now_reflect_data == refelct_data_desc_size_map.end())
	{
		size = 0;
		PancystarEngine::EngineFailReason error_message(0, "class: " + name + " haven't init to reflect class");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflectControl::GetReflectDataSizeByMember", error_message);
		return error_message;
	}
	size = now_reflect_data->second;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflectControl::GetReflectDataSizeByArray(const std::string &name, pancy_resource_size &size) 
{
	auto now_reflect_array_name = refelct_array_map.find(name);
	if (now_reflect_array_name == refelct_array_map.end())
	{
		size = 0;
		PancystarEngine::EngineFailReason error_message(0, "class: " + name + " haven't init to reflect class");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflectControl::GetReflectDataSizeByArray", error_message);
		return error_message;
	}
	auto check_error = GetReflectDataSizeByMember(now_reflect_array_name->second, size);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
bool PancyJsonReflectControl::CheckIfStructArrayInit(const std::string &class_name)
{
	auto now_reflect_array_name = refelct_array_map.find(class_name);
	if (now_reflect_array_name == refelct_array_map.end())
	{
		return false;
	}
	return CheckIfStructMemberInit(now_reflect_array_name->second);
}
bool PancyJsonReflectControl::CheckIfStructMemberInit(const std::string &class_name)
{
	auto now_reflect_data = refelct_map.find(class_name);
	if (now_reflect_data == refelct_map.end())
	{
		return false;
	}
	return true;
}
PancyJsonReflect* PancyJsonReflectControl::GetJsonReflectByArray(const std::string &class_name)
{
	auto now_reflect_array_name = refelct_array_map.find(class_name);
	if (now_reflect_array_name == refelct_array_map.end())
	{
		PancystarEngine::EngineFailReason error_message(0, "class: " + class_name + " haven't init to reflect class");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflectControl::GetJsonReflectByArray", error_message);
		return NULL;
	}
	return GetJsonReflect(now_reflect_array_name->second);
}
PancyJsonReflect* PancyJsonReflectControl::GetJsonReflect(const std::string &class_name)
{
	auto now_reflect_data = refelct_map.find(class_name);
	if (now_reflect_data == refelct_map.end())
	{
		PancystarEngine::EngineFailReason error_message(0, "class: " + class_name + " haven't init to reflect class");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflectControl::GetJsonReflect", error_message);
		return NULL;
	}
	return now_reflect_data->second;
}