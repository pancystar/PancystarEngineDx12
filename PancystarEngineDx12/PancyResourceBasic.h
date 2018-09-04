#pragma once
#include"PancystarEngineBasicDx12.h"
//渲染资源处理模板(资源resource，资源管理器resource control，资源访问器resource view，资源访问器管理器resource view control)
/*
继承于控制器的类可以创建任意多个加载函数进行资源加载，
但是必须要通过BuildResource函数将加载完毕的资源加入到队列。
*/
/*
64位int作为id号，不做id回收
*/
namespace PancystarEngine
{
	class reference_basic 
	{
		std::atomic<uint64_t> reference_count;
	public:
		reference_basic() 
		{
			reference_count = 0;
		}
		inline void AddReference() 
		{
			reference_count.fetch_add(1);
		}
		inline void DeleteReference() 
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
		inline uint64_t GetReference() 
		{
			return reference_count.load();
		}
	};
	class PancyBasicVirtualResource
	{
	protected:
		uint32_t resource_id;
		std::atomic<int32_t> reference_count;
	public:
		PancyBasicVirtualResource(const uint32_t &resource_id_in);
		virtual ~PancyBasicVirtualResource();
		PancystarEngine::EngineFailReason Create(std::string desc_file_in);
		void AddReference();
		void DeleteReference();
		inline int32_t GetReference()
		{
			return reference_count;
		}
	private:
		virtual PancystarEngine::EngineFailReason InitResource(std::string resource_desc_file) = 0;
	};
	template<class T>
	class PancyBasicResourceControl
	{
		std::string resource_type_name;
	private:
		//资源列表，设为私有，仅允许buildresource进行访问
		uint64_t resource_id_self_add;
		std::unordered_map<uint64_t, PancyBasicVirtualResource*> basic_resource_array;
	public:
		PancyBasicResourceControl(const std::string &resource_type_name_in);
		virtual ~PancyBasicResourceControl();
		PancystarEngine::EngineFailReason AddResurceReference(const uint64_t &resource_id);
		PancystarEngine::EngineFailReason DeleteResurceReference(const uint64_t &resource_id);
		PancystarEngine::EngineFailReason GetResource(const uint64_t &resource_id, PancyBasicVirtualResource** resource_out = NULL);
		PancystarEngine::EngineFailReason LoadResource(std::string desc_file_in, uint64_t &resource_id_out);
	};
	//基础资源管理器
	template<class T>
	PancyBasicResourceControl<T>::PancyBasicResourceControl(const std::string &resource_type_name_in)
	{
		resource_type_name = resource_type_name_in;
		resource_id_self_add = 0;
	}
	template<class T>
	PancyBasicResourceControl<T>::~PancyBasicResourceControl()
	{
		for (auto data_resource = basic_resource_array.begin(); data_resource != basic_resource_array.end(); ++data_resource)
		{
			delete data_resource->second;
		}
		basic_resource_array.clear();
	}
	template<class T>
	PancystarEngine::EngineFailReason PancyBasicResourceControl<T>::LoadResource(std::string desc_file_in, uint64_t &resource_id_out)
	{
		//创建一个新的资源
		PancyBasicVirtualResource *new_data = new T();
		PancystarEngine::EngineFailReason check_error = new_data->Create(desc_file_in);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		//插入到资源列表
		basic_resource_array.insert(std::pair<uint32_t, PancyBasicVirtualResource*>(resource_id_self_add, new_data));
		resource_id_out = resource_id_self_add;
		//自增ID号+1
		resource_id_self_add += 1;
		return PancystarEngine::succeed;
	}
	/*
	PancystarEngine::EngineFailReason PancyBasicResourceControl::BuildResource(PancyBasicVirtualResource* data_input, uint64_t &resource_id_out)
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
	*/
	template<class T>
	PancystarEngine::EngineFailReason PancyBasicResourceControl<T>::AddResurceReference(const uint64_t &resource_id)
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
	template<class T>
	PancystarEngine::EngineFailReason PancyBasicResourceControl<T>::DeleteResurceReference(const uint64_t &resource_id)
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
	template<class T>
	PancystarEngine::EngineFailReason PancyBasicResourceControl<T>::GetResource(const uint64_t &resource_id, PancyBasicVirtualResource** resource_out)
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
}