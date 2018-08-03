#pragma once
#include"PancyDx12Basic.h"
class PancyShaderBasic 
{
	PancystarEngine::PancyString shader_file_name;
	PancystarEngine::PancyString shader_entry_point_name;
	PancystarEngine::PancyString shader_type_name;
	ComPtr<ID3DBlob> shader_memory_pointer;
	
public:
	PancyShaderBasic
	(
		const std::string &shader_file_in,
		const std::string &main_func_name,
		const std::string &shader_type
	);
	ComPtr<ID3DBlob> GetShader() 
	{
		return shader_memory_pointer;
	}
	PancystarEngine::EngineFailReason Create();

};


