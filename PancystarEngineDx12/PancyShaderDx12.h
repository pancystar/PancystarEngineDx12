#pragma once
#include"PancyDx12Basic.h"
class PancyShaderBasic 
{
	PancystarEngine::PancyString shader_file_name;
	ComPtr<ID3DBlob> shader_memory_pointer;
	ComPtr<ID3D12ShaderReflection> shader_reflection;
public:
	PancyShaderBasic(const std::string &shader_file_in);
	PancystarEngine::EngineFailReason Create();

};
PancyShaderBasic::PancyShaderBasic(const std::string &shader_file_in) 
{
	shader_file_name = shader_file_in;
}
PancystarEngine::EngineFailReason PancyShaderBasic::Create()
{
#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif
	//编译shader文件
	D3DCompileFromFile(shader_file_name.GetUnicodeString().c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &shader_memory_pointer, nullptr);
	//获取shader反射
	D3DReflect(shader_memory_pointer->GetBufferPointer(), shader_memory_pointer->GetBufferSize(),IID_ID3D12ShaderReflection, &shader_reflection);
}
