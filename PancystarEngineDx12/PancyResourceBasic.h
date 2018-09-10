#pragma once
#include"PancystarEngineBasicDx12.h"
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
		std::string resource_name;
		std::atomic<int32_t> reference_count;
	public:
		PancyBasicVirtualResource();
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
	class PancyBasicResourceControl
	{
		std::string resource_type_name;
	private:
		std::unordered_map<std::string, PancyBasicVirtualResource*> basic_resource_array;
	public:
		PancyBasicResourceControl(const std::string &resource_type_name_in);
		virtual ~PancyBasicResourceControl();
		PancystarEngine::EngineFailReason AddResurceReference(const std::string &resource_id);
		PancystarEngine::EngineFailReason DeleteResurceReference(const std::string &resource_id);
		PancystarEngine::EngineFailReason GetResource(const std::string &resource_id, PancyBasicVirtualResource** resource_out = NULL);
		PancystarEngine::EngineFailReason LoadResource(std::string desc_file_in);
	private:
		virtual void BuildResource(PancyBasicVirtualResource** resource_out = NULL) = 0;
	};
}