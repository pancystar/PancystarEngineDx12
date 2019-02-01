#pragma once
#include"PancyResourceBasic.h"
#include"PancyThreadBasic.h"

namespace PancystarEngine 
{
	enum PancyBufferType 
	{
		Buffer_ShaderResource = 0,
		Buffer_CommondBUffer = 1,
		Buffer_VertexIndex = 2
	};
	class PancyBasicBuffer : public PancyBasicVirtualResource
	{
		PancyBufferType  buffer_type;
		SubMemoryPointer buffer_data;     //buffer数据指针
		SubMemoryPointer update_tex_data; //buffer上传数据指针
	private:
		PancystarEngine::EngineFailReason InitResource(const Json::Value &root_value, const std::string &resource_name);
	};
}