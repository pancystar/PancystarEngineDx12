#include"PancyResourceBasic.h"
using namespace PancystarEngine;
//基础资源
PancyBasicVirtualResource::PancyBasicVirtualResource(const uint32_t &resource_id_in)
{
	resource_id = resource_id_in;
	reference_count.store(0);
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
	auto check_error = InitResource();
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
	resource_id_self_add = 0;
}
PancystarEngine::EngineFailReason PancyBasicResourceControl::BuildResource(PancyBasicVirtualResource* data_input, uint32_t &resource_id_out)
{
	//创建一个新的资源
	if (data_input == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not build resource with NULL data ID: " + std::to_string(resource_id_self_add), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build resource data in resource control " + resource_type_name, error_message);
		return error_message;
	}
	//插入到资源列表
	basic_resource_array.insert(std::pair<uint32_t, PancyBasicVirtualResource*>(resource_id_self_add, data_input));
	resource_id_out = resource_id_self_add;
	//自增ID号+1
	resource_id_self_add += 1;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicResourceControl::AddResurceReference(const uint32_t &resource_id)
{
	auto data_now = basic_resource_array.find(resource_id);
	if (data_now == basic_resource_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource ID: " + std::to_string(resource_id), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add resource reference in resource control " + resource_type_name, error_message);
		return error_message;
	}
	data_now->second->AddReference();
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicResourceControl::DeleteResurceReference(const uint32_t &resource_id)
{
	auto data_now = basic_resource_array.find(resource_id);
	if (data_now == basic_resource_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource ID: " + std::to_string(resource_id), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete resource reference in resource control " + resource_type_name, error_message);
		return error_message;
	}
	data_now->second->DeleteReference();
	//引用计数为0,删除该资源
	if (data_now->second->GetReference() == 0)
	{
		delete data_now->second;
		basic_resource_array.erase(data_now);
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicResourceControl::GetResource(const uint32_t &resource_id, PancyBasicVirtualResource** resource_out)
{
	auto data_now = basic_resource_array.find(resource_id);
	if (data_now == basic_resource_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource ID: " + std::to_string(resource_id), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get resource reference in resource control " + resource_type_name, error_message);
		return error_message;
	}
	if (resource_out != NULL)
	{
		*resource_out = data_now->second;
	}
	return PancystarEngine::succeed;
}
//基础资源访问器
PancyBasicResourceView::PancyBasicResourceView()
{
}
PancystarEngine::EngineFailReason PancyBasicResourceView::Create(
	const uint32_t &resource_id_in,
	const uint32_t &resource_view_id_in,
	PancyBasicResourceControl *resource_control_use_in
)
{
	//检查资源是否存在
	PancystarEngine::EngineFailReason check_error;
	check_error = resource_control_use_in->GetResource(resource_id_in);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//保存资源访问信息
	resource_used_id = resource_id_in;
	resource_view_id = resource_view_id_in;
	resource_control_use = resource_control_use_in;
	//添加资源引用计数
	check_error = resource_control_use_in->AddResurceReference(resource_used_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//创建资源view
	InitResourceView();
}
PancyBasicResourceView::~PancyBasicResourceView()
{
	resource_control_use->DeleteResurceReference(resource_used_id);
}
//基础资源访问器管理器
PancyBasicResourceViewControl::PancyBasicResourceViewControl(const std::string &resource_type_name_in, PancyBasicResourceControl *resource_control_use_in)
{
	resource_view_id_self_add = 0;
	resource_type_name = resource_type_name_in;
	resource_control_use = resource_control_use_in;
}
PancystarEngine::EngineFailReason PancyBasicResourceViewControl::DeleteResourceView(const uint32_t &resource_view_id)
{
	auto data_now = resource_view_array.find(resource_view_id);
	if (data_now == resource_view_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource view ID: " + std::to_string(resource_view_id), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete resource view in resource view control " + resource_type_name, error_message);
		return error_message;
	}
	delete data_now->second;
	resource_view_array.erase(data_now);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicResourceViewControl::BuildResourceView(PancyBasicResourceView* data_input, uint32_t &resource_view_id_out)
{
	//创建一个新的资源
	if (data_input == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not build resource with NULL data ID: " + std::to_string(resource_view_id_self_add), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build resource data in resource control " + resource_type_name, error_message);
		return error_message;
	}
	//插入到资源列表
	resource_view_array.insert(std::pair<uint32_t, PancyBasicResourceView*>(resource_view_id_self_add, data_input));
	resource_view_id_out = resource_view_id_self_add;
	//自增ID号+1
	resource_view_id_self_add += 1;
	return PancystarEngine::succeed;
}