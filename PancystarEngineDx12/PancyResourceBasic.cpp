#include"PancyResourceBasic.h"
using namespace PancystarEngine;
//基础资源
PancyBasicVirtualResource::PancyBasicVirtualResource(const std::string &resource_name_in, Json::Value root_value_in)
{
	now_res_state = ResourceStateType::resource_state_not_init;
	resource_name = resource_name_in;
	root_value = root_value_in;
	reference_count.store(0);
}
PancyBasicVirtualResource::~PancyBasicVirtualResource()
{
}
void PancyBasicVirtualResource::AddReference()
{
	reference_count.fetch_add(1);
}
void PancyBasicVirtualResource::DeleteReference()
{
	if (reference_count > 0)
	{
		reference_count.fetch_sub(1);
	}
	else
	{
		reference_count.store(0);
	}
}
PancystarEngine::EngineFailReason PancyBasicVirtualResource::Create()
{
	auto check_error = InitResource(root_value, resource_name, now_res_state);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
//基础资源管理器
PancyBasicResourceControl::PancyBasicResourceControl(const std::string &resource_type_name_in)
{
	resource_type_name = resource_type_name_in;
}
PancyBasicResourceControl::~PancyBasicResourceControl()
{
	for (auto data_resource = basic_resource_array.begin(); data_resource != basic_resource_array.end(); ++data_resource)
	{
		delete data_resource->second;
	}
	basic_resource_array.clear();
	resource_name_list.clear();
	free_id_list.clear();
}
PancystarEngine::EngineFailReason PancyBasicResourceControl::LoadResource(const std::string &name_resource_in, const Json::Value &root_value, pancy_object_id &id_need)
{
	PancystarEngine::EngineFailReason check_error;
	//资源加载判断重复
	auto check_data = resource_name_list.find(name_resource_in);
	if (check_data != resource_name_list.end())
	{
		id_need = check_data->second;
		PancystarEngine::EngineFailReason error_message(E_FAIL, "repeat load resource : " + name_resource_in, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Resource", error_message);
		return error_message;
	}
	//创建一个新的资源
	PancyBasicVirtualResource *new_data;
	check_error = BuildResource(root_value, name_resource_in, &new_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = new_data->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	int id_now;
	//判断是否有空闲的id编号
	if (free_id_list.size() > 0)
	{
		id_now = *free_id_list.begin();
		free_id_list.erase(id_now);
	}
	else
	{
		id_now = basic_resource_array.size();
	}
	//添加名称-id表用于判重
	resource_name_list.insert(std::pair<std::string, pancy_object_id>(name_resource_in, id_now));
	//插入到资源列表
	basic_resource_array.insert(std::pair<pancy_object_id, PancyBasicVirtualResource*>(id_now, new_data));
	id_need = id_now;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicResourceControl::LoadResource(const std::string &desc_file_in, pancy_object_id &id_need)
{
	PancystarEngine::EngineFailReason check_error;
	Json::Value root_value;
	check_error = PancyJsonTool::GetInstance()->LoadJsonFile(desc_file_in, root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = LoadResource(desc_file_in, root_value, id_need);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicResourceControl::AddResurceReference(const pancy_object_id &resource_id)
{
	auto data_now = basic_resource_array.find(resource_id);
	if (data_now == basic_resource_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource: " + resource_id, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add resource reference in resource control " + resource_type_name, error_message);
		return error_message;
	}
	data_now->second->AddReference();
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicResourceControl::DeleteResurceReference(const pancy_object_id &resource_id)
{
	auto data_now = basic_resource_array.find(resource_id);
	if (data_now == basic_resource_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource: " + resource_id, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete resource reference in resource control " + resource_type_name, error_message);
		return error_message;
	}
	data_now->second->DeleteReference();
	//引用计数为0,删除该资源
	if (data_now->second->GetReferenceCount() == 0)
	{
		//删除资源对应名称
		resource_name_list.erase(data_now->second->GetResourceName());
		//添加到空闲资源
		free_id_list.insert(data_now->first);
		//删除资源
		delete data_now->second;
		basic_resource_array.erase(data_now);
	}
	return PancystarEngine::succeed;
}
PancyBasicVirtualResource* PancyBasicResourceControl::GetResource(const pancy_object_id &resource_id)
{
	auto data_now = basic_resource_array.find(resource_id);
	if (data_now == basic_resource_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource: " + resource_id, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get resource reference in resource control " + resource_type_name, error_message);
		return NULL;
	}
	return data_now->second;
}