#pragma once
#include"PancyResourceBasic.h"
#include"PancyThreadBasic.h"

namespace PancystarEngine 
{
	class PancyBasicBuffer : public PancyBasicVirtualResource
	{
		
	private:
		PancystarEngine::EngineFailReason InitResource(const std::string &resource_desc_file);
	};
}