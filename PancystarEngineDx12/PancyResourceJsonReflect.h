#pragma once
#include"PancystarEngineBasicDx12.h"
#include <typeinfo>
#include<type_traits>
#include"PancyJsonTool.h"
#define Init_Json_Data_Vatriable(var_name) AddAnyVariable(#var_name, var_name)
#define Bind_Json_Data_Array_Size(array_value,size_value) BindArraySizeValue<decltype(array_value)>(#array_value, #size_value)
//这里使用#define创建资源可以在编译期检查出类型不匹配的错误
#define InitJsonReflectParseClass(ReflectStructType,ReflectClassType) PancyJsonReflectTemplate<ReflectStructType> *pointer_##ReflectStructType = new ReflectClassType();\
																	  PancyJsonReflectControl::GetInstance()->InitJsonReflect<ReflectStructType,ReflectClassType>(pointer_##ReflectStructType);
//基本数据的反射保留内容
struct JsonReflectData
{
	PancyJsonMemberType data_type;  //标识数据的基本格式
	std::string data_name;          //标识数据的名称
	std::string parent_name;        //标识数据的父结构的名称
	std::string data_type_name;     //标识数据的细节类型名称
	pancy_object_id array_size = 0; //标识数据的最大可容纳大小(兼容数组)
	void* data_pointer = NULL;             //数据的内容
};
//基本反射类
class PancyJsonReflect
{
	//将json数据绑定到普通结构体的数据结构
	std::unordered_map<std::string, JsonReflectData> value_map;                //每个成员变量对应的值
	std::unordered_map<std::string, std::string> parent_list;                  //每个成员变量的父节点
	std::unordered_map<std::string, std::vector<std::string>> child_value_list;//每个成员变量的子变量
	std::unordered_map<std::string, std::string> array_real_size_map;          //每个数组的真实大小对应的变量
public:
	PancyJsonReflect();
	virtual ~PancyJsonReflect();
	void Create();
	//从json文件中加载数据
	PancystarEngine::EngineFailReason LoadFromJsonFile(const std::string &Json_file);
	//从json内存中加载数据
	PancystarEngine::EngineFailReason LoadFromJsonMemory(const std::string &value_name, const Json::Value &root_value);
	//将类成员数据存储到json文件
	PancystarEngine::EngineFailReason SaveToJsonFile(const std::string &json_name);
	//将类成员数据存储到json内存
	PancystarEngine::EngineFailReason SaveToJsonMemory(Json::Value &root_value);
	//将类成员数据拷贝到指定指针
	virtual PancystarEngine::EngineFailReason CopyMemberData(void *dst_pointer, const std::string &data_type_name, const pancy_resource_size &size) = 0;
	//将类成员数据添加到vector中
	virtual PancystarEngine::EngineFailReason CopyVectorData(void *vector_pointer, const std::string &data_type_name, const pancy_resource_size &index, const pancy_resource_size &size) = 0;
	//将指定指针的数据拷贝到类成员数据
	virtual PancystarEngine::EngineFailReason ResetMemoryByMemberData(void *memory_pointer, const std::string &data_type_name, const pancy_resource_size &size) = 0;
	//将指定数组的数据拷贝到类成员数据
	virtual PancystarEngine::EngineFailReason ResetMemoryByArrayData(void *array_pointer, const std::string &data_type_name, const pancy_object_id &index, const pancy_resource_size &size) = 0;
	//将指定vector的数据拷贝到类成员数据
	virtual PancystarEngine::EngineFailReason ResetMemoryByVectorData(void *vector_pointer, const std::string &data_type_name, const pancy_object_id &index, const pancy_resource_size &size) = 0;
	//获取vector变量的容量
	virtual PancystarEngine::EngineFailReason GetVectorDataSize(void *vector_pointer, const std::string &data_type_name, pancy_object_id &size) = 0;
private:
	//创建子节点映射
	void BuildChildValueMap();
	//从json节点中加载数据
	PancystarEngine::EngineFailReason LoadFromJsonNode(const std::string &parent_name, const std::string &value_name, const Json::Value &root_value);
	//从json数组中加载数据
	PancystarEngine::EngineFailReason LoadFromJsonArray(const std::string &value_name, const Json::Value &root_value);
	//将数据写入到json节点
	PancystarEngine::EngineFailReason SaveToJsonNode(const std::string &parent_name, Json::Value &root_value);
	//注册所有的反射变量
	virtual void InitBasicVariable() = 0;
	//获取节点的父节点名称
	const std::string GetParentName(const std::string &name_in);
	//设置基础变量
	PancystarEngine::EngineFailReason SetIntValue(JsonReflectData &reflect_data, const int64_t &int_value);
	PancystarEngine::EngineFailReason SetDoubleValue(JsonReflectData &reflect_data, const double &double_value);
	PancystarEngine::EngineFailReason SetBoolValue(JsonReflectData &reflect_data, const bool &bool_value);
	PancystarEngine::EngineFailReason SetStringValue(JsonReflectData &reflect_data, const std::string &string_value);
	//设置数组变量
	PancystarEngine::EngineFailReason SetArrayValue(JsonReflectData &reflect_data, const Json::Value &now_child_value);
	PancystarEngine::EngineFailReason SetIntArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const int64_t &int_value);
	PancystarEngine::EngineFailReason SetDoubleArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const double &double_value);
	PancystarEngine::EngineFailReason SetBoolArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const bool &bool_value);
	PancystarEngine::EngineFailReason SetStringArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const std::string &string_value);
	PancystarEngine::EngineFailReason SetNodeArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const Json::Value &now_child_value);
	//插入vector数据
	template<class T1, class T2>
	PancystarEngine::EngineFailReason SetVectorValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const T2 &input_value);
	//获取基础变量
	PancystarEngine::EngineFailReason SaveSingleValueMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value);
	//获取数组变量
	template<typename ArrayType, typename JsonType>
	PancystarEngine::EngineFailReason SaveArrayValueMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value);
	PancystarEngine::EngineFailReason SaveArrayEnumMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value);
	//获取vector变量
	template<typename ArrayType, typename JsonType>
	PancystarEngine::EngineFailReason SaveVectorValueMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value);
	PancystarEngine::EngineFailReason SaveVectorEnumMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value);
	PancystarEngine::EngineFailReason TranslateStringToEnum(const std::string &basic_string, std::string &enum_type, std::string &enum_value_string, int32_t &enum_value_data);
	//将变量的完整名称转化为基础名称
	std::string TranslateFullNameToRealName(const std::string &full_name);
	//获取数组数据的真实大小
	PancystarEngine::EngineFailReason GetArrayDataSize(const JsonReflectData &reflect_data,pancy_object_id &size_out);
protected:
	template<typename T>
	PancystarEngine::EngineFailReason AddAnyVariable(
		const std::string &name,
		T &variable_data
	);
	PancystarEngine::EngineFailReason AddVariable(const std::string &name, void*variable_data, const size_t &variable_type, const std::string &variable_type_name);
	PancystarEngine::EngineFailReason AddArray(const std::string &name, void*variable_data, const size_t &variable_type, const std::string &variable_type_name, const pancy_object_id &array_size);
	template<typename ArrayType>
	PancystarEngine::EngineFailReason BindArraySizeValue(
		const std::string &array_name,
		const std::string &value_name
	);
	PancystarEngine::EngineFailReason AddReflectData(const PancyJsonMemberType &type_data, const std::string &name, const std::string &variable_type_name, void*variable_data, const pancy_object_id &array_size);
};
template<typename T>
PancystarEngine::EngineFailReason PancyJsonReflect::AddAnyVariable(
	const std::string &name,
	T &variable_data
)
{
	if constexpr (std::is_array<T>::value)
	{
		int32_t array_size = sizeof(variable_data) / sizeof(variable_data[0]);
		return AddArray(name, (void*)(variable_data), typeid(&variable_data[0]).hash_code(), typeid(&variable_data[0]).name(), array_size);
	}
	else
	{
		return AddVariable(name, (void*)(&variable_data), typeid(variable_data).hash_code(), typeid(variable_data).name());
	}
	return PancystarEngine::succeed;
}
template<typename ArrayType>
PancystarEngine::EngineFailReason PancyJsonReflect::BindArraySizeValue(
	const std::string &array_name,
	const std::string &value_name
) 
{
	if constexpr (std::is_array<ArrayType>::value)
	{
		array_real_size_map.insert(std::pair<std::string, std::string>(array_name, value_name));
	}
	else 
	{
		//未找到变量，无法绑定
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, array_name + " is not an array value, could not bind it's size to other member: ",error_message);
		
		return error_message;
	}
	return PancystarEngine::succeed;
}
template<class T1, class T2>
PancystarEngine::EngineFailReason PancyJsonReflect::SetVectorValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const T2 &input_value)
{
	std::vector<T1> *now_data_pointer = reinterpret_cast<std::vector<T1>*>(reflect_data.data_pointer);
	if (now_data_pointer->size() != offset_value)
	{
		//偏移量不正确，插入第i个元素要保证已经有了i个成员
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "reflect vector have wrong offset: " + reflect_data.data_name,error_message);
		
		return error_message;
	}
	now_data_pointer->push_back(static_cast<T1>(input_value));
	return PancystarEngine::succeed;
}
template<typename ArrayType, typename JsonType>
PancystarEngine::EngineFailReason PancyJsonReflect::SaveArrayValueMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value)
{
	if (typeid(ArrayType*).name() != reflect_data.data_type_name)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "array type dismatch: " + reflect_data.data_name,error_message);
		
		return error_message;
	}
	ArrayType *pointer = reinterpret_cast<ArrayType*>(reflect_data.data_pointer);
	//获取真实使用的数组大小
	pancy_object_id array_used_size = 0;
	auto check_error = GetArrayDataSize(reflect_data, array_used_size);
	if (!check_error.if_succeed) 
	{
		return check_error;
	}
	for (pancy_object_id data_offset_index = 0; data_offset_index < array_used_size; ++data_offset_index)
	{
		PancyJsonTool::GetInstance()->AddJsonArrayValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<JsonType>(pointer[data_offset_index]));
	}
	return PancystarEngine::succeed;
}
template<typename ArrayType, typename JsonType>
PancystarEngine::EngineFailReason PancyJsonReflect::SaveVectorValueMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value)
{
	if (typeid(std::vector<ArrayType>).name() != reflect_data.data_type_name)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "array type dismatch: " + reflect_data.data_name,error_message);
		
		return error_message;
	}
	std::vector<ArrayType> *pointer = reinterpret_cast<std::vector<ArrayType>*>(reflect_data.data_pointer);
	for (int32_t data_offset_index = 0; data_offset_index < pointer->size(); ++data_offset_index)
	{
		PancyJsonTool::GetInstance()->AddJsonArrayValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<JsonType>((*pointer)[data_offset_index]));
	}
	return PancystarEngine::succeed;
}
//模板反射类
template<typename ReflectDataType>
class PancyJsonReflectTemplate :public PancyJsonReflect
{
protected:
	ReflectDataType reflect_data;
public:
	PancyJsonReflectTemplate();
	virtual ~PancyJsonReflectTemplate();
	PancystarEngine::EngineFailReason CopyMemberData(void * dst_pointer, const std::string &data_type_name, const pancy_resource_size & size) override;
	PancystarEngine::EngineFailReason CopyVectorData(void *vector_pointer, const std::string &data_type_name, const pancy_resource_size &index, const pancy_resource_size &size) override;

	PancystarEngine::EngineFailReason ResetMemoryByMemberData(void *array_pointer, const std::string &data_type_name, const pancy_resource_size &size) override;
	PancystarEngine::EngineFailReason ResetMemoryByArrayData(void *array_pointer, const std::string &data_type_name, const pancy_object_id &index, const pancy_resource_size &size) override;
	PancystarEngine::EngineFailReason ResetMemoryByVectorData(void *vector_pointer, const std::string &data_type_name, const pancy_object_id &index, const pancy_resource_size &size) override;
	PancystarEngine::EngineFailReason GetVectorDataSize(void *vector_pointer, const std::string &data_type_name, pancy_object_id &size) override;
};
template<typename ReflectDataType>
PancyJsonReflectTemplate<ReflectDataType>::PancyJsonReflectTemplate()
{

}
template<typename ReflectDataType>
PancyJsonReflectTemplate<ReflectDataType>::~PancyJsonReflectTemplate() 
{

}
template<typename ReflectDataType>
PancystarEngine::EngineFailReason PancyJsonReflectTemplate<ReflectDataType>::CopyMemberData(void *dst_pointer, const std::string &data_type_name, const pancy_resource_size &size)
{
	std::string check_data_type_name = typeid(ReflectDataType*).name();
	if (check_data_type_name != data_type_name)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "could not copy memory for json reflect,type dismatch",error_message);
		
		return error_message;
	}
	if (size != sizeof(ReflectDataType))
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "could not copy memory for json reflect,size dismatch",error_message);
		
		return error_message;
	}
	ReflectDataType *pointer = reinterpret_cast<ReflectDataType*>(dst_pointer);
	*pointer = reflect_data;
	return PancystarEngine::succeed;
}
template<typename ReflectDataType>
PancystarEngine::EngineFailReason PancyJsonReflectTemplate<ReflectDataType>::CopyVectorData(void *vector_pointer, const std::string &data_type_name, const pancy_resource_size &index, const pancy_resource_size &size)
{
	std::string check_data_type_name = typeid(std::vector<ReflectDataType>).name();
	if (check_data_type_name != data_type_name)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "could not push back vector for json reflect,type dismatch",error_message);
		
		return error_message;
	}
	if (size != sizeof(ReflectDataType))
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "could not push back vector for json reflect,size dismatch",error_message);
		
		return error_message;
	}
	std::vector<ReflectDataType> *pointer = reinterpret_cast<std::vector<ReflectDataType>*>(vector_pointer);
	if (pointer->size() != index)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "could not add vector memory for json reflect,index number dismatch",error_message);
		
		return error_message;
	}
	pointer->push_back(reflect_data);
	return PancystarEngine::succeed;
}
template<typename ReflectDataType>
PancystarEngine::EngineFailReason PancyJsonReflectTemplate<ReflectDataType>::ResetMemoryByMemberData(void *memory_pointer, const std::string &data_type_name, const pancy_resource_size &size)
{
	std::string check_data_type_name = typeid(ReflectDataType*).name();
	if (check_data_type_name != data_type_name)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "could not copy memory for json reflect,type dismatch",error_message);
		
		return error_message;
	}
	if (size != sizeof(ReflectDataType))
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "could not copy memory for json reflect,size dismatch",error_message);
		
		return error_message;
	}
	ReflectDataType *real_data_pointer = reinterpret_cast<ReflectDataType*>(memory_pointer);
	reflect_data = *real_data_pointer;
	return PancystarEngine::succeed;
}
template<typename ReflectDataType>
PancystarEngine::EngineFailReason PancyJsonReflectTemplate<ReflectDataType>::ResetMemoryByArrayData(void *array_pointer, const std::string &data_type_name, const pancy_object_id &index, const pancy_resource_size &size)
{
	std::string check_data_type_name = typeid(ReflectDataType*).name();
	if (check_data_type_name != data_type_name)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "could not copy memory for json reflect,type dismatch",error_message);
		
		return error_message;
	}
	if (size != sizeof(ReflectDataType))
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "could not copy memory for json reflect,size dismatch",error_message);
		
		return error_message;
	}
	ReflectDataType *real_data_pointer = reinterpret_cast<ReflectDataType*>(array_pointer);
	reflect_data = real_data_pointer[index];
	return PancystarEngine::succeed;
}
template<typename ReflectDataType>
PancystarEngine::EngineFailReason PancyJsonReflectTemplate<ReflectDataType>::ResetMemoryByVectorData(void *vector_pointer, const std::string &data_type_name, const pancy_object_id &index, const pancy_resource_size &size)
{
	std::string check_data_type_name = typeid(std::vector<ReflectDataType>).name();
	if (check_data_type_name != data_type_name)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "could not push back vector for json reflect,type dismatch",error_message);
		
		return error_message;
	}
	if (size != sizeof(ReflectDataType))
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "could not push back vector for json reflect,size dismatch",error_message);
		
		return error_message;
	}
	std::vector<ReflectDataType> *pointer = reinterpret_cast<std::vector<ReflectDataType>*>(vector_pointer);
	if (pointer->size() <= index)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "could not add vector memory for json reflect,index number dismatch",error_message);
		
		return error_message;
	}
	reflect_data = (*pointer)[index];
	return PancystarEngine::succeed;
}
template<typename ReflectDataType>
PancystarEngine::EngineFailReason PancyJsonReflectTemplate<ReflectDataType>::GetVectorDataSize(void *vector_pointer, const std::string &data_type_name, pancy_object_id &size)
{
	std::string check_data_type_name = typeid(std::vector<ReflectDataType>).name();
	if (check_data_type_name != data_type_name)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "could not push back vector for json reflect,type dismatch",error_message);
		
		return error_message;
	}
	std::vector<ReflectDataType> *pointer = reinterpret_cast<std::vector<ReflectDataType>*>(vector_pointer);
	size = static_cast<pancy_object_id>((*pointer).size());
	return PancystarEngine::succeed;
}
class PancyJsonReflectControl 
{
	std::unordered_map<std::string, PancyJsonReflect*> refelct_map;
	std::unordered_map<std::string, std::string> refelct_array_map;
	std::unordered_map<std::string, pancy_resource_size> refelct_data_desc_size_map;
	PancyJsonReflectControl();
public:
	~PancyJsonReflectControl();
	template<typename ReflectStructType,typename ReflectClassType>
	void InitJsonReflect(PancyJsonReflect* reflect_parse_class);
	PancyJsonReflect* GetJsonReflect(const std::string &class_name);
	PancyJsonReflect* GetJsonReflectByArray(const std::string &class_name);
	PancystarEngine::EngineFailReason GetReflectDataSizeByMember(const std::string &name, pancy_resource_size &size);
	PancystarEngine::EngineFailReason GetReflectDataSizeByArray(const std::string &name, pancy_resource_size &size);
	//通过vector或者array的方式检查结构体是否注册
	bool CheckIfStructArrayInit(const std::string &class_name);
	//通过变量的方式检查结构体是否注册
	bool CheckIfStructMemberInit(const std::string &class_name);
	static PancyJsonReflectControl* GetInstance()
	{
		static PancyJsonReflectControl* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new PancyJsonReflectControl();
		}
		return this_instance;
	}
};
template<typename ReflectStructType, typename ReflectClassType>
void PancyJsonReflectControl::InitJsonReflect(PancyJsonReflect* new_reflect_data)
{
	std::string class_name = typeid(ReflectStructType).name();
	auto now_reflect_data = refelct_map.find(class_name);
	if (now_reflect_data == refelct_map.end())
	{
		new_reflect_data->Create();
		refelct_map.insert(std::pair<std::string, PancyJsonReflect*>(class_name, new_reflect_data));
		std::string array_type_name = typeid(ReflectStructType*).name();
		std::string vector_type_name = typeid(std::vector<ReflectStructType>).name();
		refelct_array_map.insert(std::pair<std::string, std::string>(array_type_name, class_name));
		refelct_array_map.insert(std::pair<std::string, std::string>(vector_type_name, class_name));
		refelct_data_desc_size_map.insert(std::pair<std::string, pancy_resource_size>(class_name, sizeof(ReflectStructType)));
	}
	else if (new_reflect_data != NULL)
	{
		delete new_reflect_data;
	}
}
