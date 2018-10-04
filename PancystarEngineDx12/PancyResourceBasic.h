#pragma once
#include"PancyMemoryBasic.h"
namespace PancystarEngine
{
	class PancyBasicVirtualResource
	{
	protected:
		std::string resource_name;
		std::atomic<pancy_object_id> reference_count;
	public:
		PancyBasicVirtualResource(std::string desc_file_in);
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
		virtual PancystarEngine::EngineFailReason InitResource(std::string resource_desc_file) = 0;
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
		PancystarEngine::EngineFailReason LoadResource(std::string desc_file_in);
	private:
		virtual PancystarEngine::EngineFailReason BuildResource(const std::string &desc_file_in, PancyBasicVirtualResource** resource_out) = 0;
	};
	
	

}