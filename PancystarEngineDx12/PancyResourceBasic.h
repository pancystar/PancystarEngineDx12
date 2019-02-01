#pragma once
#include"PancyMemoryBasic.h"
namespace PancystarEngine
{
	enum ResourceStateType 
	{
		resource_state_not_init = 0,
		resource_state_load_CPU_memory_finish,
		resource_state_load_GPU_memory_finish
	};
	class PancyBasicVirtualResource
	{
		ResourceStateType now_res_state;
		Json::Value root_value;
	protected:
		std::string resource_name;
		std::atomic<pancy_object_id> reference_count;
	public:
		PancyBasicVirtualResource(const std::string &resource_name_in, Json::Value root_value_in);
		virtual ~PancyBasicVirtualResource();
		PancystarEngine::EngineFailReason Create();
		void AddReference();
		void DeleteReference();
		inline int32_t GetReferenceCount()
		{
			return reference_count;
		}
		inline std::string GetResourceName()
		{
			return resource_name;
		}
	private:
		//virtual PancystarEngine::EngineFailReason InitResource(const std::string &resource_desc_file) = 0;
		virtual PancystarEngine::EngineFailReason InitResource(
			const Json::Value &root_value, 
			const std::string &resource_name, 
			ResourceStateType &now_res_state
		) = 0;
	};
	class PancyBasicResourceControl
	{
		std::string resource_type_name;
	private:
		std::unordered_map<std::string, pancy_object_id> resource_name_list;
		std::unordered_map<pancy_object_id, PancyBasicVirtualResource*> basic_resource_array;
		std::unordered_set<pancy_object_id> free_id_list;
	public:
		PancyBasicResourceControl(const std::string &resource_type_name_in);
		virtual ~PancyBasicResourceControl();
		PancystarEngine::EngineFailReason AddResurceReference(const pancy_object_id &resource_id);
		PancystarEngine::EngineFailReason DeleteResurceReference(const pancy_object_id &resource_id);
		PancyBasicVirtualResource* GetResource(const pancy_object_id &desc_file_name);
		PancystarEngine::EngineFailReason LoadResource(const std::string &desc_file_in, pancy_object_id &id_need);
		PancystarEngine::EngineFailReason LoadResource(const std::string &name_resource_in, const Json::Value &root_value, pancy_object_id &id_need);
	private:
		//virtual PancystarEngine::EngineFailReason BuildResource(const std::string &desc_file_in, PancyBasicVirtualResource** resource_out) = 0;
		virtual PancystarEngine::EngineFailReason BuildResource(
			const Json::Value &root_value, 
			const std::string &name_resource_in, 
			PancyBasicVirtualResource** resource_out
		) = 0;
	};
	
	

}