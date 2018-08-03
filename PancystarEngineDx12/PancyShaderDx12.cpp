#include"PancyShaderDx12.h"
PancyShaderBasic::PancyShaderBasic(
	const std::string &shader_file_in,
	const std::string &main_func_name,
	const std::string &shader_type
)
{
	shader_file_name = shader_file_in;
	shader_entry_point_name = main_func_name;
	shader_type_name = shader_type;
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
	HRESULT hr = D3DCompileFromFile(shader_file_name.GetUnicodeString().c_str(), nullptr, nullptr, shader_entry_point_name.GetAsciiString().c_str(), shader_type_name.GetAsciiString().c_str(), compileFlags, 0, &shader_memory_pointer, nullptr);
	if (FAILED(hr)) 
	{
		PancystarEngine::EngineFailReason error_message(hr, "Compile shader : " + shader_file_name.GetAsciiString() +":: " + shader_entry_point_name.GetAsciiString() +" error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("load shader from file", error_message);
		return error_message;
	}
	//获取shader反射
	ComPtr<ID3D12ShaderReflection> shader_reflection;
	hr = D3DReflect(shader_memory_pointer->GetBufferPointer(), shader_memory_pointer->GetBufferSize(), IID_ID3D12ShaderReflection, &shader_reflection);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "get shader reflect message : " + shader_file_name.GetAsciiString() + ":: " + shader_entry_point_name.GetAsciiString() + " error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("load shader from file", error_message);
		return error_message;
	}
	//获取输入格式
	D3D12_SHADER_DESC shader_desc;
	shader_reflection->GetDesc(&shader_desc);
	for (UINT i = 0; i < shader_desc.InputParameters; ++i)
	{
		D3D12_SIGNATURE_PARAMETER_DESC now_param_desc;
		shader_reflection->GetInputParameterDesc(i, &now_param_desc);
	}
	//获取常量缓冲区
	for (UINT i = 0; i < shader_desc.ConstantBuffers; ++i)
	{
		auto now_constant_buffer = shader_reflection->GetConstantBufferByIndex(i);
		D3D12_SHADER_BUFFER_DESC buffer_shader;
		now_constant_buffer->GetDesc(&buffer_shader);
		int a = 0;
	}
	for (UINT i = 0; i < shader_desc.BoundResources; ++i)
	{
		D3D12_SHADER_INPUT_BIND_DESC now_bind;
		shader_reflection->GetResourceBindingDesc(i, &now_bind);
		int a = 0;
	}
	return PancystarEngine::succeed;
}