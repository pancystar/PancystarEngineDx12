#include"PancyShaderDx12.h"
//输入顶点格式
InputLayoutDesc::InputLayoutDesc()
{
}
InputLayoutDesc::~InputLayoutDesc()
{
	for (auto now_vertex_desc = vertex_buffer_desc_map.begin(); now_vertex_desc != vertex_buffer_desc_map.end(); ++now_vertex_desc)
	{
		delete now_vertex_desc->second.inputElementDescs;
		now_vertex_desc->second.inputElementDescs = NULL;
	}
	vertex_buffer_desc_map.clear();
}
void InputLayoutDesc::AddVertexDesc(std::string vertex_desc_name_in, std::vector<D3D12_INPUT_ELEMENT_DESC> input_element_desc_list)
{
	PancyVertexBufferDesc new_vertex_desc;
	new_vertex_desc.vertex_desc_name = vertex_desc_name_in;
	new_vertex_desc.input_element_num = input_element_desc_list.size();
	new_vertex_desc.inputElementDescs = new D3D12_INPUT_ELEMENT_DESC[input_element_desc_list.size()];
	uint32_t count_element = 0;
	for (auto now_element = input_element_desc_list.begin(); now_element != input_element_desc_list.end(); ++now_element)
	{
		new_vertex_desc.inputElementDescs[count_element] = *now_element;
		count_element += 1;
	}
	vertex_buffer_desc_map.insert(std::pair<std::string, PancyVertexBufferDesc>(vertex_desc_name_in, new_vertex_desc));
}
//shader编译器
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
//shader管理器
PancyShaderControl::PancyShaderControl()
{

}
PancystarEngine::EngineFailReason PancyShaderControl::LoadShader(std::string shader_file, std::string shader_main_func, std::string shader_type)
{
	std::string shader_final_name = shader_file + "::" + shader_main_func;
	auto check_data = shader_list.find(shader_final_name);
	if (check_data != shader_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the shader " + shader_final_name + " had been insert to map before", PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build new shader", error_message);
		return error_message;
	}
	PancyShaderBasic *new_shader = new PancyShaderBasic(shader_file, shader_main_func, shader_type);
	auto check_error = new_shader->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	shader_list.insert(std::pair<std::string, PancyShaderBasic*>(shader_final_name, new_shader));
	return PancystarEngine::succeed;
}
ComPtr<ID3D12ShaderReflection> PancyShaderControl::GetShaderReflection(std::string shader_file, std::string shader_main_func)
{
	std::string shader_final_name = shader_file + "::" + shader_main_func;
	auto shader_data = shader_list.find(shader_final_name);
	if (shader_data == shader_list.end())
	{
		return NULL;
	}
	return shader_data->second->GetShaderReflect();
}
ComPtr<ID3DBlob> PancyShaderControl::GetShaderData(std::string shader_file, std::string shader_main_func)
{
	std::string shader_final_name = shader_file + "::" + shader_main_func;
	auto shader_data = shader_list.find(shader_final_name);
	if (shader_data == shader_list.end())
	{
		return NULL;
	}
	return shader_data->second->GetShader();
}
PancyShaderControl::~PancyShaderControl()
{
	for (auto data = shader_list.begin(); data != shader_list.end(); ++data)
	{
		delete data->second;
	}
	shader_list.clear();
}
//rootsignature
PancyRootSignature::PancyRootSignature(const std::string &file_name)
{
	root_signature_name = file_name;
}
PancystarEngine::EngineFailReason PancyRootSignature::Create(const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC &rootSignatureDesc)
{
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	HRESULT hr;
	hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "serial rootsignature " + root_signature_name.GetAsciiString() + " error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Create Root Signature", error_message);
		return error_message;
	}
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_signature_data));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create root signature " + root_signature_name.GetAsciiString() + " failed");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Create Root Signature", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
//rootsignature管理器
PancyRootSignatureControl::PancyRootSignatureControl()
{
	AddRootSignatureGlobelVariable();
	ranges = NULL;
	rootParameters = NULL;
	data_sampledesc = NULL;
}
void PancyRootSignatureControl::AddRootSignatureGlobelVariable()
{
	//descriptor range格式
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_DESCRIPTOR_RANGE_TYPE_SRV", static_cast<int32_t>(D3D12_DESCRIPTOR_RANGE_TYPE_SRV));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_DESCRIPTOR_RANGE_TYPE_UAV", static_cast<int32_t>(D3D12_DESCRIPTOR_RANGE_TYPE_UAV));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_DESCRIPTOR_RANGE_TYPE_CBV", static_cast<int32_t>(D3D12_DESCRIPTOR_RANGE_TYPE_CBV));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER", static_cast<int32_t>(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER));
	//descriptor flag格式
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_DESCRIPTOR_RANGE_FLAG_NONE", static_cast<int32_t>(D3D12_DESCRIPTOR_RANGE_FLAG_NONE));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE", static_cast<int32_t>(D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE", static_cast<int32_t>(D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE", static_cast<int32_t>(D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC", static_cast<int32_t>(D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC));
	//shader访问权限
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_SHADER_VISIBILITY_ALL", static_cast<int32_t>(D3D12_SHADER_VISIBILITY_ALL));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_SHADER_VISIBILITY_VERTEX", static_cast<int32_t>(D3D12_SHADER_VISIBILITY_VERTEX));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_SHADER_VISIBILITY_HULL", static_cast<int32_t>(D3D12_SHADER_VISIBILITY_HULL));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_SHADER_VISIBILITY_DOMAIN", static_cast<int32_t>(D3D12_SHADER_VISIBILITY_DOMAIN));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_SHADER_VISIBILITY_GEOMETRY", static_cast<int32_t>(D3D12_SHADER_VISIBILITY_GEOMETRY));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_SHADER_VISIBILITY_PIXEL", static_cast<int32_t>(D3D12_SHADER_VISIBILITY_PIXEL));
	//采样格式
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MIN_MAG_MIP_POINT", static_cast<int32_t>(D3D12_FILTER_MIN_MAG_MIP_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR", static_cast<int32_t>(D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT", static_cast<int32_t>(D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR", static_cast<int32_t>(D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT", static_cast<int32_t>(D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR", static_cast<int32_t>(D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT", static_cast<int32_t>(D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MIN_MAG_MIP_LINEAR", static_cast<int32_t>(D3D12_FILTER_MIN_MAG_MIP_LINEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_ANISOTROPIC", static_cast<int32_t>(D3D12_FILTER_ANISOTROPIC));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT", static_cast<int32_t>(D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR", static_cast<int32_t>(D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT", static_cast<int32_t>(D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR", static_cast<int32_t>(D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT", static_cast<int32_t>(D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR", static_cast<int32_t>(D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT", static_cast<int32_t>(D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR", static_cast<int32_t>(D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_COMPARISON_ANISOTROPIC", static_cast<int32_t>(D3D12_FILTER_COMPARISON_ANISOTROPIC));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT", static_cast<int32_t>(D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR", static_cast<int32_t>(D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT", static_cast<int32_t>(D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR", static_cast<int32_t>(D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT", static_cast<int32_t>(D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR", static_cast<int32_t>(D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT", static_cast<int32_t>(D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR", static_cast<int32_t>(D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MINIMUM_ANISOTROPIC", static_cast<int32_t>(D3D12_FILTER_MINIMUM_ANISOTROPIC));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT", static_cast<int32_t>(D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR", static_cast<int32_t>(D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT", static_cast<int32_t>(D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR", static_cast<int32_t>(D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT", static_cast<int32_t>(D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR", static_cast<int32_t>(D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT", static_cast<int32_t>(D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR", static_cast<int32_t>(D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILTER_MAXIMUM_ANISOTROPIC", static_cast<int32_t>(D3D12_FILTER_MAXIMUM_ANISOTROPIC));
	//寻址格式
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_TEXTURE_ADDRESS_MODE_WRAP", static_cast<int32_t>(D3D12_TEXTURE_ADDRESS_MODE_WRAP));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_TEXTURE_ADDRESS_MODE_MIRROR", static_cast<int32_t>(D3D12_TEXTURE_ADDRESS_MODE_MIRROR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_TEXTURE_ADDRESS_MODE_CLAMP", static_cast<int32_t>(D3D12_TEXTURE_ADDRESS_MODE_CLAMP));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_TEXTURE_ADDRESS_MODE_BORDER", static_cast<int32_t>(D3D12_TEXTURE_ADDRESS_MODE_BORDER));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE", static_cast<int32_t>(D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE));
	//比较函数格式
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COMPARISON_FUNC_NEVER", static_cast<int32_t>(D3D12_COMPARISON_FUNC_NEVER));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COMPARISON_FUNC_LESS", static_cast<int32_t>(D3D12_COMPARISON_FUNC_LESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COMPARISON_FUNC_EQUAL", static_cast<int32_t>(D3D12_COMPARISON_FUNC_EQUAL));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COMPARISON_FUNC_LESS_EQUAL", static_cast<int32_t>(D3D12_COMPARISON_FUNC_LESS_EQUAL));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COMPARISON_FUNC_GREATER", static_cast<int32_t>(D3D12_COMPARISON_FUNC_GREATER));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COMPARISON_FUNC_NOT_EQUAL", static_cast<int32_t>(D3D12_COMPARISON_FUNC_NOT_EQUAL));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COMPARISON_FUNC_GREATER_EQUAL", static_cast<int32_t>(D3D12_COMPARISON_FUNC_GREATER_EQUAL));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COMPARISON_FUNC_ALWAYS", static_cast<int32_t>(D3D12_COMPARISON_FUNC_ALWAYS));
	//超出边界的采样颜色
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK", static_cast<int32_t>(D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK", static_cast<int32_t>(D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE", static_cast<int32_t>(D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE));
	//root signature的访问权限
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_ROOT_SIGNATURE_FLAG_NONE", static_cast<int32_t>(D3D12_ROOT_SIGNATURE_FLAG_NONE));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT", static_cast<int32_t>(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS", static_cast<int32_t>(D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS", static_cast<int32_t>(D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS", static_cast<int32_t>(D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS", static_cast<int32_t>(D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS", static_cast<int32_t>(D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT", static_cast<int32_t>(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT));
}
PancystarEngine::EngineFailReason PancyRootSignatureControl::GetDesc(const std::string &file_name, CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC &desc_out)
{
	Json::Value jsonRoot;
	auto check_error = JsonLoader::GetInstance()->LoadJsonFile(file_name, jsonRoot);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}

	pancy_json_value now_value;
	//读取shader外部变量的数量
	check_error = JsonLoader::GetInstance()->GetJsonData(file_name, jsonRoot, "NumParameters", pancy_json_data_type::json_data_int, now_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	int num_parameter = now_value.int_value;
	//读取每个shader外部变量
	Json::Value value_parameters = jsonRoot.get("D3D12_ROOT_PARAMETER", Json::Value::null);
	if (value_parameters == Json::Value::null)
	{
		//无法找到存储parameter的数组
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable D3D12_ROOT_PARAMETER");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Root Signature json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	if (value_parameters.size() != num_parameter)
	{
		//parameter数组大小与json预设的不一致
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "variable D3D12_ROOT_PARAMETER only have " + std::to_string(value_parameters.size()) + " values, dismatch num specified " + std::to_string(num_parameter));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Root Signature json file " + file_name + " error", error_mesage);
		return error_mesage;
	}

	//CD3DX12_DESCRIPTOR_RANGE1 *ranges = NULL;
	//CD3DX12_ROOT_PARAMETER1 *rootParameters = NULL;
	if (num_parameter > 0)
	{
		ranges = new CD3DX12_DESCRIPTOR_RANGE1[num_parameter];
		rootParameters = new CD3DX12_ROOT_PARAMETER1[num_parameter];
	}
	for (int i = 0; i < num_parameter; ++i)
	{
		pancy_json_value data_param_value[7];
		std::string find_name[] =
		{
			"RangeType",
			"NumDescriptors",
			"BaseShaderRegister",
			"RegisterSpace",
			"flags",
			"numDescriptorRanges",
			"ShaderVisibility"
		};
		pancy_json_data_type out_data_type[] =
		{
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_int,
			pancy_json_data_type::json_data_int,
			pancy_json_data_type::json_data_int,
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_int,
			pancy_json_data_type::json_data_enum
		};
		for (int j = 0; j < 7; ++j)
		{
			check_error = JsonLoader::GetInstance()->GetJsonData(file_name, value_parameters[i], find_name[j], out_data_type[j], data_param_value[j]);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		//获取需要range_type(cbv/srv/......)
		D3D12_DESCRIPTOR_RANGE_TYPE range_type = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(data_param_value[0].int_value);
		//获取需要创建的descriptor(cbv/srv/......)的数量
		int32_t descriptor_num = data_param_value[1].int_value;
		//获取这组descriptor变量对应的首个寄存器的地址
		int32_t base_registor = data_param_value[2].int_value;
		//获取这组descriptor变量对应的寄存器域的地址
		int32_t base_registor_space = data_param_value[3].int_value;
		//获取这组descriptor变量对应的格式
		D3D12_DESCRIPTOR_RANGE_FLAGS flag_type = static_cast<D3D12_DESCRIPTOR_RANGE_FLAGS>(data_param_value[4].int_value);
		//注册descriptor range
		ranges[i].Init(range_type, descriptor_num, base_registor, base_registor_space, flag_type);


		//获取该descriptor range需要被创建的次数
		int32_t descriptor_range_num = data_param_value[5].int_value;
		//获取shader的访问权限
		D3D12_SHADER_VISIBILITY shader_visibility_type = static_cast<D3D12_SHADER_VISIBILITY>(data_param_value[6].int_value);
		//注册rootsignature格式
		rootParameters[i].InitAsDescriptorTable(descriptor_range_num, &ranges[i], shader_visibility_type);

	}

	//获取静态采样器的数量
	int num_static_sampler;
	check_error = JsonLoader::GetInstance()->GetJsonData(file_name, jsonRoot, "NumStaticSamplers", pancy_json_data_type::json_data_int, now_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	num_static_sampler = now_value.int_value;
	//获取每个静态采样器
	Json::Value value_staticsamplers = jsonRoot.get("D3D12_STATIC_SAMPLER_DESC", Json::Value::null);
	if (value_staticsamplers == Json::Value::null)
	{
		//无法找到存储static sampler的数组
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable D3D12_STATIC_SAMPLER_DESC");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Root Signature json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	if (value_staticsamplers.size() != num_static_sampler)
	{
		//staticsampler数组大小与json预设的不一致
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "variable D3D12_STATIC_SAMPLER_DESC only have " + std::to_string(value_staticsamplers.size()) + " values, dismatch num specified " + std::to_string(num_static_sampler));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Root Signature json file " + file_name + " error", error_mesage);
		return error_mesage;
	}

	//D3D12_STATIC_SAMPLER_DESC *data_sampledesc = NULL;
	if (num_static_sampler > 0)
	{
		data_sampledesc = new D3D12_STATIC_SAMPLER_DESC[num_static_sampler];
	}
	for (int i = 0; i < num_static_sampler; ++i)
	{
		pancy_json_value data_num[13];
		std::string find_name[] =
		{
			"Filter",
			"AddressU",
			"AddressV",
			"AddressW",
			"MipLODBias",
			"MaxAnisotropy",
			"ComparisonFunc",
			"BorderColor",
			"MinLOD",
			"MaxLOD",
			"ShaderRegister",
			"RegisterSpace",
			"ShaderVisibility"
		};
		pancy_json_data_type out_data_type[] =
		{
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_float,
			pancy_json_data_type::json_data_int,
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_float,
			pancy_json_data_type::json_data_float,
			pancy_json_data_type::json_data_int,
			pancy_json_data_type::json_data_int,
			pancy_json_data_type::json_data_enum
		};
		for (int j = 0; j < 13; ++j)
		{
			check_error = JsonLoader::GetInstance()->GetJsonData(file_name, value_staticsamplers[i], find_name[j], out_data_type[j], data_num[j]);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}

		data_sampledesc[i].Filter = static_cast<D3D12_FILTER>(data_num[0].int_value);
		data_sampledesc[i].AddressU = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(data_num[1].int_value);
		data_sampledesc[i].AddressV = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(data_num[2].int_value);
		data_sampledesc[i].AddressW = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(data_num[3].int_value);
		data_sampledesc[i].MipLODBias = data_num[4].float_value;
		data_sampledesc[i].MaxAnisotropy = data_num[5].int_value;
		data_sampledesc[i].ComparisonFunc = static_cast<D3D12_COMPARISON_FUNC>(data_num[6].int_value);
		data_sampledesc[i].BorderColor = static_cast<D3D12_STATIC_BORDER_COLOR>(data_num[7].int_value);
		data_sampledesc[i].MinLOD = data_num[8].float_value;
		data_sampledesc[i].MaxLOD = data_num[9].float_value;
		data_sampledesc[i].ShaderRegister = data_num[10].int_value;
		data_sampledesc[i].RegisterSpace = data_num[11].int_value;
		data_sampledesc[i].ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(data_num[12].int_value);
	}
	//获取rootsignature格式
	Json::Value value_root_signature_flag = jsonRoot.get("RootSignatureFlags", Json::Value::null);
	if (value_staticsamplers == Json::Value::null)
	{
		//无法找到存储root signature flag的数组
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable RootSignatureFlags");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Root Signature json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	D3D12_ROOT_SIGNATURE_FLAGS root_signature = static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(0);
	for (uint32_t i = 0; i < value_root_signature_flag.size(); ++i)
	{
		check_error = JsonLoader::GetInstance()->GetJsonData(file_name, value_root_signature_flag, i, pancy_json_data_type::json_data_enum, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		root_signature = root_signature | static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(now_value.int_value);
	}
	desc_out.Init_1_1(num_parameter, rootParameters, num_static_sampler, data_sampledesc, root_signature);
	
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyRootSignatureControl::BuildRootSignature(std::string rootsig_config_file)
{
	PancystarEngine::EngineFailReason check_error;
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc = {};
	//判断重复
	auto check_data = root_signature_array.find(rootsig_config_file);
	if (check_data != root_signature_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the root signature " + rootsig_config_file + " had been insert to map before", PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build new root signature", error_message);
		return error_message;
	}
	//创建RootSignature
	PancyRootSignature *data_root_signature = new PancyRootSignature(rootsig_config_file);
	check_error = GetDesc(rootsig_config_file, root_signature_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = data_root_signature->Create(root_signature_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	root_signature_array.insert(std::pair<std::string, PancyRootSignature*>(rootsig_config_file, data_root_signature));
	if (ranges != NULL)
	{
		delete[] ranges;
		ranges = NULL;
	}
	if (rootParameters != NULL)
	{
		delete[] rootParameters;
		rootParameters = NULL;
	}
	if (data_sampledesc != NULL)
	{
		delete[] data_sampledesc;
		data_sampledesc = NULL;
	}
	return PancystarEngine::succeed;
}
PancyRootSignature* PancyRootSignatureControl::GetRootSignature(std::string name_in)
{
	auto root_signature_find = root_signature_array.find(name_in);
	if (root_signature_find != root_signature_array.end())
	{
		return root_signature_find->second;
	}
	return NULL;
}
PancyRootSignatureControl::~PancyRootSignatureControl()
{
	for (auto data = root_signature_array.begin(); data != root_signature_array.end(); ++data)
	{
		delete data->second;
	}
	root_signature_array.clear();
}
//pipline state object graph
PancyPiplineStateObjectGraph::PancyPiplineStateObjectGraph(std::string pso_name_in)
{
	pso_name = pso_name_in;
}
PancystarEngine::EngineFailReason PancyPiplineStateObjectGraph::Create(const D3D12_GRAPHICS_PIPELINE_STATE_DESC &pso_desc_in)
{
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateGraphicsPipelineState(&pso_desc_in, IID_PPV_ARGS(&pso_data));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create PSO error name " + pso_name.GetAsciiString());
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build PSO", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
//effect
PancyEffectGraphic::PancyEffectGraphic() 
{
	AddPSOGlobelVariable();
}
void PancyEffectGraphic::AddPSOGlobelVariable()
{
	//几何体填充格式
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILL_MODE_WIREFRAME", static_cast<int32_t>(D3D12_FILL_MODE_WIREFRAME));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_FILL_MODE_SOLID", static_cast<int32_t>(D3D12_FILL_MODE_SOLID));
	//几何体消隐格式
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_CULL_MODE_NONE", static_cast<int32_t>(D3D12_CULL_MODE_NONE));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_CULL_MODE_FRONT", static_cast<int32_t>(D3D12_CULL_MODE_FRONT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_CULL_MODE_BACK", static_cast<int32_t>(D3D12_CULL_MODE_BACK));
	//alpha混合系数
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_ZERO", static_cast<int32_t>(D3D12_BLEND_ZERO));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_ONE", static_cast<int32_t>(D3D12_BLEND_ONE));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_SRC_COLOR", static_cast<int32_t>(D3D12_BLEND_SRC_COLOR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_INV_SRC_COLOR", static_cast<int32_t>(D3D12_BLEND_INV_SRC_COLOR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_SRC_ALPHA", static_cast<int32_t>(D3D12_BLEND_SRC_ALPHA));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_INV_SRC_ALPHA", static_cast<int32_t>(D3D12_BLEND_INV_SRC_ALPHA));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_DEST_ALPHA", static_cast<int32_t>(D3D12_BLEND_DEST_ALPHA));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_INV_DEST_ALPHA", static_cast<int32_t>(D3D12_BLEND_INV_DEST_ALPHA));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_DEST_COLOR", static_cast<int32_t>(D3D12_BLEND_DEST_COLOR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_INV_DEST_COLOR", static_cast<int32_t>(D3D12_BLEND_INV_DEST_COLOR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_SRC_ALPHA_SAT", static_cast<int32_t>(D3D12_BLEND_SRC_ALPHA_SAT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_BLEND_FACTOR", static_cast<int32_t>(D3D12_BLEND_BLEND_FACTOR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_INV_BLEND_FACTOR", static_cast<int32_t>(D3D12_BLEND_INV_BLEND_FACTOR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_SRC1_COLOR", static_cast<int32_t>(D3D12_BLEND_SRC1_COLOR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_INV_SRC1_COLOR", static_cast<int32_t>(D3D12_BLEND_INV_SRC1_COLOR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_SRC1_ALPHA", static_cast<int32_t>(D3D12_BLEND_SRC1_ALPHA));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_INV_SRC1_ALPHA", static_cast<int32_t>(D3D12_BLEND_INV_SRC1_ALPHA));
	//alpha混合操作
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_OP_ADD", static_cast<int32_t>(D3D12_BLEND_OP_ADD));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_OP_SUBTRACT", static_cast<int32_t>(D3D12_BLEND_OP_SUBTRACT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_OP_REV_SUBTRACT", static_cast<int32_t>(D3D12_BLEND_OP_REV_SUBTRACT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_OP_MIN", static_cast<int32_t>(D3D12_BLEND_OP_MIN));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_BLEND_OP_MAX", static_cast<int32_t>(D3D12_BLEND_OP_MAX));
	//alpha混合logic操作
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_LOGIC_OP_CLEAR", static_cast<int32_t>(D3D12_LOGIC_OP_CLEAR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_LOGIC_OP_SET", static_cast<int32_t>(D3D12_LOGIC_OP_SET));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_LOGIC_OP_COPY", static_cast<int32_t>(D3D12_LOGIC_OP_COPY));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_LOGIC_OP_COPY_INVERTED", static_cast<int32_t>(D3D12_LOGIC_OP_COPY_INVERTED));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_LOGIC_OP_NOOP", static_cast<int32_t>(D3D12_LOGIC_OP_NOOP));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_LOGIC_OP_INVERT", static_cast<int32_t>(D3D12_LOGIC_OP_INVERT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_LOGIC_OP_AND", static_cast<int32_t>(D3D12_LOGIC_OP_AND));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_LOGIC_OP_NAND", static_cast<int32_t>(D3D12_LOGIC_OP_NAND));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_LOGIC_OP_OR", static_cast<int32_t>(D3D12_LOGIC_OP_OR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_LOGIC_OP_NOR", static_cast<int32_t>(D3D12_LOGIC_OP_NOR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_LOGIC_OP_XOR", static_cast<int32_t>(D3D12_LOGIC_OP_XOR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_LOGIC_OP_EQUIV", static_cast<int32_t>(D3D12_LOGIC_OP_EQUIV));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_LOGIC_OP_AND_REVERSE", static_cast<int32_t>(D3D12_LOGIC_OP_AND_REVERSE));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_LOGIC_OP_AND_INVERTED", static_cast<int32_t>(D3D12_LOGIC_OP_AND_INVERTED));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_LOGIC_OP_OR_REVERSE", static_cast<int32_t>(D3D12_LOGIC_OP_OR_REVERSE));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_LOGIC_OP_OR_INVERTED", static_cast<int32_t>(D3D12_LOGIC_OP_OR_INVERTED));
	//alpha混合目标掩码
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COLOR_WRITE_ENABLE_RED", static_cast<int32_t>(D3D12_COLOR_WRITE_ENABLE_RED));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COLOR_WRITE_ENABLE_GREEN", static_cast<int32_t>(D3D12_COLOR_WRITE_ENABLE_GREEN));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COLOR_WRITE_ENABLE_BLUE", static_cast<int32_t>(D3D12_COLOR_WRITE_ENABLE_BLUE));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COLOR_WRITE_ENABLE_ALPHA", static_cast<int32_t>(D3D12_COLOR_WRITE_ENABLE_ALPHA));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COLOR_WRITE_ENABLE_ALL", static_cast<int32_t>(D3D12_COLOR_WRITE_ENABLE_ALL));
	//深度缓冲区写掩码
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_DEPTH_WRITE_MASK_ZERO", static_cast<int32_t>(D3D12_DEPTH_WRITE_MASK_ZERO));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_DEPTH_WRITE_MASK_ALL", static_cast<int32_t>(D3D12_DEPTH_WRITE_MASK_ALL));
	//深度缓冲区比较函数
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COMPARISON_FUNC_NEVER", static_cast<int32_t>(D3D12_COMPARISON_FUNC_NEVER));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COMPARISON_FUNC_LESS", static_cast<int32_t>(D3D12_COMPARISON_FUNC_LESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COMPARISON_FUNC_EQUAL", static_cast<int32_t>(D3D12_COMPARISON_FUNC_EQUAL));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COMPARISON_FUNC_LESS_EQUAL", static_cast<int32_t>(D3D12_COMPARISON_FUNC_LESS_EQUAL));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COMPARISON_FUNC_GREATER", static_cast<int32_t>(D3D12_COMPARISON_FUNC_GREATER));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COMPARISON_FUNC_NOT_EQUAL", static_cast<int32_t>(D3D12_COMPARISON_FUNC_NOT_EQUAL));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COMPARISON_FUNC_GREATER_EQUAL", static_cast<int32_t>(D3D12_COMPARISON_FUNC_GREATER_EQUAL));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_COMPARISON_FUNC_ALWAYS", static_cast<int32_t>(D3D12_COMPARISON_FUNC_ALWAYS));
	//模板缓冲区操作
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_STENCIL_OP_KEEP", static_cast<int32_t>(D3D12_STENCIL_OP_KEEP));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_STENCIL_OP_ZERO", static_cast<int32_t>(D3D12_STENCIL_OP_ZERO));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_STENCIL_OP_REPLACE", static_cast<int32_t>(D3D12_STENCIL_OP_REPLACE));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_STENCIL_OP_INCR_SAT", static_cast<int32_t>(D3D12_STENCIL_OP_INCR_SAT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_STENCIL_OP_DECR_SAT", static_cast<int32_t>(D3D12_STENCIL_OP_DECR_SAT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_STENCIL_OP_INVERT", static_cast<int32_t>(D3D12_STENCIL_OP_INVERT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_STENCIL_OP_INCR", static_cast<int32_t>(D3D12_STENCIL_OP_INCR));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_STENCIL_OP_DECR", static_cast<int32_t>(D3D12_STENCIL_OP_DECR));
	//渲染图元格式
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED", static_cast<int32_t>(D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT", static_cast<int32_t>(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE", static_cast<int32_t>(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE", static_cast<int32_t>(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH", static_cast<int32_t>(D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH));
	//采样格式
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_UNKNOWN", static_cast<int32_t>(DXGI_FORMAT_UNKNOWN));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32B32A32_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R32G32B32A32_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32B32A32_FLOAT", static_cast<int32_t>(DXGI_FORMAT_R32G32B32A32_FLOAT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32B32A32_UINT", static_cast<int32_t>(DXGI_FORMAT_R32G32B32A32_UINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32B32A32_SINT", static_cast<int32_t>(DXGI_FORMAT_R32G32B32A32_SINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32B32_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R32G32B32_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32B32_FLOAT", static_cast<int32_t>(DXGI_FORMAT_R32G32B32_FLOAT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32B32_UINT", static_cast<int32_t>(DXGI_FORMAT_R32G32B32_UINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32B32_SINT", static_cast<int32_t>(DXGI_FORMAT_R32G32B32_SINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16B16A16_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R16G16B16A16_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16B16A16_FLOAT", static_cast<int32_t>(DXGI_FORMAT_R16G16B16A16_FLOAT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16B16A16_UNORM", static_cast<int32_t>(DXGI_FORMAT_R16G16B16A16_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16B16A16_UINT", static_cast<int32_t>(DXGI_FORMAT_R16G16B16A16_UINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16B16A16_SNORM", static_cast<int32_t>(DXGI_FORMAT_R16G16B16A16_SNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16B16A16_SINT", static_cast<int32_t>(DXGI_FORMAT_R16G16B16A16_SINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R32G32_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32_FLOAT", static_cast<int32_t>(DXGI_FORMAT_R32G32_FLOAT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32_UINT", static_cast<int32_t>(DXGI_FORMAT_R32G32_UINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32_SINT", static_cast<int32_t>(DXGI_FORMAT_R32G32_SINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G8X24_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R32G8X24_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_D32_FLOAT_S8X24_UINT", static_cast<int32_t>(DXGI_FORMAT_D32_FLOAT_S8X24_UINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_X32_TYPELESS_G8X24_UINT", static_cast<int32_t>(DXGI_FORMAT_X32_TYPELESS_G8X24_UINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R10G10B10A2_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R10G10B10A2_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R10G10B10A2_UNORM", static_cast<int32_t>(DXGI_FORMAT_R10G10B10A2_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R10G10B10A2_UINT", static_cast<int32_t>(DXGI_FORMAT_R10G10B10A2_UINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R11G11B10_FLOAT", static_cast<int32_t>(DXGI_FORMAT_R11G11B10_FLOAT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8B8A8_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R8G8B8A8_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8B8A8_UNORM", static_cast<int32_t>(DXGI_FORMAT_R8G8B8A8_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8B8A8_UNORM_SRGB", static_cast<int32_t>(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8B8A8_UINT", static_cast<int32_t>(DXGI_FORMAT_R8G8B8A8_UINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8B8A8_SNORM", static_cast<int32_t>(DXGI_FORMAT_R8G8B8A8_SNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8B8A8_SINT", static_cast<int32_t>(DXGI_FORMAT_R8G8B8A8_SINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R16G16_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16_FLOAT", static_cast<int32_t>(DXGI_FORMAT_R16G16_FLOAT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16_UNORM", static_cast<int32_t>(DXGI_FORMAT_R16G16_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16_UINT", static_cast<int32_t>(DXGI_FORMAT_R16G16_UINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16_SNORM", static_cast<int32_t>(DXGI_FORMAT_R16G16_SNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16_SINT", static_cast<int32_t>(DXGI_FORMAT_R16G16_SINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R32_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_D32_FLOAT", static_cast<int32_t>(DXGI_FORMAT_D32_FLOAT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32_FLOAT", static_cast<int32_t>(DXGI_FORMAT_R32_FLOAT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32_UINT", static_cast<int32_t>(DXGI_FORMAT_R32_UINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32_SINT", static_cast<int32_t>(DXGI_FORMAT_R32_SINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R24G8_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R24G8_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_D24_UNORM_S8_UINT", static_cast<int32_t>(DXGI_FORMAT_D24_UNORM_S8_UINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R24_UNORM_X8_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R24_UNORM_X8_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_X24_TYPELESS_G8_UINT", static_cast<int32_t>(DXGI_FORMAT_X24_TYPELESS_G8_UINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R8G8_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8_UNORM", static_cast<int32_t>(DXGI_FORMAT_R8G8_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8_UINT", static_cast<int32_t>(DXGI_FORMAT_R8G8_UINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8_SNORM", static_cast<int32_t>(DXGI_FORMAT_R8G8_SNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8_SINT", static_cast<int32_t>(DXGI_FORMAT_R8G8_SINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R16_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16_FLOAT", static_cast<int32_t>(DXGI_FORMAT_R16_FLOAT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_D16_UNORM", static_cast<int32_t>(DXGI_FORMAT_D16_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16_UNORM", static_cast<int32_t>(DXGI_FORMAT_R16_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16_UINT", static_cast<int32_t>(DXGI_FORMAT_R16_UINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16_SNORM", static_cast<int32_t>(DXGI_FORMAT_R16_SNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16_SINT", static_cast<int32_t>(DXGI_FORMAT_R16_SINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R8_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8_UNORM", static_cast<int32_t>(DXGI_FORMAT_R8_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8_UINT", static_cast<int32_t>(DXGI_FORMAT_R8_UINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8_SNORM", static_cast<int32_t>(DXGI_FORMAT_R8_SNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8_SINT", static_cast<int32_t>(DXGI_FORMAT_R8_SINT));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_A8_UNORM", static_cast<int32_t>(DXGI_FORMAT_A8_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R1_UNORM", static_cast<int32_t>(DXGI_FORMAT_R1_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R9G9B9E5_SHAREDEXP", static_cast<int32_t>(DXGI_FORMAT_R9G9B9E5_SHAREDEXP));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8_B8G8_UNORM", static_cast<int32_t>(DXGI_FORMAT_R8G8_B8G8_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_G8R8_G8B8_UNORM", static_cast<int32_t>(DXGI_FORMAT_G8R8_G8B8_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC1_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_BC1_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC1_UNORM", static_cast<int32_t>(DXGI_FORMAT_BC1_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC1_UNORM_SRGB", static_cast<int32_t>(DXGI_FORMAT_BC1_UNORM_SRGB));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC2_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_BC2_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC2_UNORM", static_cast<int32_t>(DXGI_FORMAT_BC2_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC2_UNORM_SRGB", static_cast<int32_t>(DXGI_FORMAT_BC2_UNORM_SRGB));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC3_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_BC3_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC3_UNORM", static_cast<int32_t>(DXGI_FORMAT_BC3_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC3_UNORM_SRGB", static_cast<int32_t>(DXGI_FORMAT_BC3_UNORM_SRGB));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC4_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_BC4_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC4_UNORM", static_cast<int32_t>(DXGI_FORMAT_BC4_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC4_SNORM", static_cast<int32_t>(DXGI_FORMAT_BC4_SNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC5_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_BC5_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC5_UNORM", static_cast<int32_t>(DXGI_FORMAT_BC5_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC5_SNORM", static_cast<int32_t>(DXGI_FORMAT_BC5_SNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B5G6R5_UNORM", static_cast<int32_t>(DXGI_FORMAT_B5G6R5_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B5G5R5A1_UNORM", static_cast<int32_t>(DXGI_FORMAT_B5G5R5A1_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B8G8R8A8_UNORM", static_cast<int32_t>(DXGI_FORMAT_B8G8R8A8_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B8G8R8X8_UNORM", static_cast<int32_t>(DXGI_FORMAT_B8G8R8X8_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM", static_cast<int32_t>(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B8G8R8A8_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_B8G8R8A8_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B8G8R8A8_UNORM_SRGB", static_cast<int32_t>(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B8G8R8X8_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_B8G8R8X8_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B8G8R8X8_UNORM_SRGB", static_cast<int32_t>(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC6H_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_BC6H_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC6H_UF16", static_cast<int32_t>(DXGI_FORMAT_BC6H_UF16));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC6H_SF16", static_cast<int32_t>(DXGI_FORMAT_BC6H_SF16));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC7_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_BC7_TYPELESS));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC7_UNORM", static_cast<int32_t>(DXGI_FORMAT_BC7_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC7_UNORM_SRGB", static_cast<int32_t>(DXGI_FORMAT_BC7_UNORM_SRGB));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_AYUV", static_cast<int32_t>(DXGI_FORMAT_AYUV));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_Y410", static_cast<int32_t>(DXGI_FORMAT_Y410));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_Y416", static_cast<int32_t>(DXGI_FORMAT_Y416));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_NV12", static_cast<int32_t>(DXGI_FORMAT_NV12));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_P010", static_cast<int32_t>(DXGI_FORMAT_P010));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_P016", static_cast<int32_t>(DXGI_FORMAT_P016));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_420_OPAQUE", static_cast<int32_t>(DXGI_FORMAT_420_OPAQUE));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_YUY2", static_cast<int32_t>(DXGI_FORMAT_YUY2));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_Y210", static_cast<int32_t>(DXGI_FORMAT_Y210));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_Y216", static_cast<int32_t>(DXGI_FORMAT_Y216));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_NV11", static_cast<int32_t>(DXGI_FORMAT_NV11));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_AI44", static_cast<int32_t>(DXGI_FORMAT_AI44));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_IA44", static_cast<int32_t>(DXGI_FORMAT_IA44));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_P8", static_cast<int32_t>(DXGI_FORMAT_P8));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_A8P8", static_cast<int32_t>(DXGI_FORMAT_A8P8));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B4G4R4A4_UNORM", static_cast<int32_t>(DXGI_FORMAT_B4G4R4A4_UNORM));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_P208", static_cast<int32_t>(DXGI_FORMAT_P208));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_V208", static_cast<int32_t>(DXGI_FORMAT_V208));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_V408", static_cast<int32_t>(DXGI_FORMAT_V408));
	JsonLoader::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_FORCE_UINT", static_cast<int32_t>(DXGI_FORMAT_FORCE_UINT));
}
PancystarEngine::EngineFailReason PancyEffectGraphic::GetDesc(const std::string &file_name, D3D12_GRAPHICS_PIPELINE_STATE_DESC &desc_out)
{
	Json::Value jsonRoot;
	auto check_error = JsonLoader::GetInstance()->LoadJsonFile(file_name, jsonRoot);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	pancy_json_value now_value;
	//读取rootsignature文件名
	check_error = JsonLoader::GetInstance()->GetJsonData(file_name, jsonRoot, "RootSignature", pancy_json_data_type::json_data_string, now_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if (PancyRootSignatureControl::GetInstance()->GetRootSignature(now_value.string_value) == NULL)
	{
		check_error = PancyRootSignatureControl::GetInstance()->BuildRootSignature(now_value.string_value);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
	}
	auto root_signature_use = PancyRootSignatureControl::GetInstance()->GetRootSignature(now_value.string_value);
	desc_out.pRootSignature = root_signature_use->GetRootSignature().Get();
	//获取着色器
	std::string shader_version[] =
	{
		"vs_5_0",
		"ps_5_0",
		"gs_5_0",
		"hs_5_0",
		"ds_5_0",
	};
	std::string vertex_shader_name, vertex_shader_mainfunc;
	for (int i = 0; i < 5; ++i) 
	{
		std::string shader_file_name, shader_func_name;
		Pancy_json_shader_type now_shader_read = static_cast<Pancy_json_shader_type>(static_cast<int32_t>(Pancy_json_shader_type::json_shader_vertex) + i);
		check_error = JsonLoader::GetInstance()->GetJsonShader(file_name, jsonRoot, now_shader_read, shader_file_name, shader_func_name);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		if (now_shader_read == json_shader_vertex)
		{
			vertex_shader_name = shader_file_name;
			vertex_shader_mainfunc = shader_func_name;
		}
		if (shader_file_name != "0" && shader_func_name != "0")
		{
			auto shader_data = PancyShaderControl::GetInstance()->GetShaderData(shader_file_name, shader_func_name);
			if (shader_data == NULL)
			{
				auto check_error = PancyShaderControl::GetInstance()->LoadShader(shader_file_name, shader_func_name, shader_version[static_cast<int32_t>(now_shader_read)]);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
				shader_data = PancyShaderControl::GetInstance()->GetShaderData(shader_file_name, shader_func_name);
			}
			if (now_shader_read == Pancy_json_shader_type::json_shader_vertex)
			{
				desc_out.VS = CD3DX12_SHADER_BYTECODE(shader_data.Get());
			}
			else if (now_shader_read == Pancy_json_shader_type::json_shader_pixel)
			{
				desc_out.PS = CD3DX12_SHADER_BYTECODE(shader_data.Get());
			}
			else if (now_shader_read == Pancy_json_shader_type::json_shader_geometry)
			{
				desc_out.GS = CD3DX12_SHADER_BYTECODE(shader_data.Get());
			}
			else if (now_shader_read == Pancy_json_shader_type::json_shader_hull)
			{
				desc_out.HS = CD3DX12_SHADER_BYTECODE(shader_data.Get());
			}
			else if (now_shader_read == Pancy_json_shader_type::json_shader_domin)
			{
				desc_out.DS = CD3DX12_SHADER_BYTECODE(shader_data.Get());
			}
		}
	}
	//根据顶点着色器获取顶点输入格式
	auto vertex_shader_reflect = PancyShaderControl::GetInstance()->GetShaderReflection(vertex_shader_name, vertex_shader_mainfunc);
	if (InputLayoutDesc::GetInstance()->GetVertexDesc(vertex_shader_name + "::" + vertex_shader_mainfunc) == NULL) 
	{
		//未找到输入缓冲区，添加新的缓冲区
		std::vector<D3D12_INPUT_ELEMENT_DESC> input_desc_array;
		check_error = GetInputDesc(vertex_shader_reflect, input_desc_array);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		InputLayoutDesc::GetInstance()->AddVertexDesc(vertex_shader_name + "::" + vertex_shader_mainfunc, input_desc_array);
	}
	auto vertex_desc = InputLayoutDesc::GetInstance()->GetVertexDesc(vertex_shader_name + "::" + vertex_shader_mainfunc);
	desc_out.InputLayout.pInputElementDescs = vertex_desc->inputElementDescs;
	desc_out.InputLayout.NumElements = static_cast<UINT>(vertex_desc->input_element_num);
	//读取光栅化格式
	Json::Value value_rasterize_state = jsonRoot.get("RasterizerState", Json::Value::null);
	if (value_rasterize_state == Json::Value::null)
	{
		//无法找到光栅化格式
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable RasterizerState");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	check_error = JsonLoader::GetInstance()->GetJsonData(file_name, value_rasterize_state, "FILL_MODE", pancy_json_data_type::json_data_enum, now_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	desc_out.RasterizerState.FillMode = static_cast<D3D12_FILL_MODE>(now_value.int_value);
	check_error = JsonLoader::GetInstance()->GetJsonData(file_name, value_rasterize_state, "CULL_MODE", pancy_json_data_type::json_data_enum, now_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	desc_out.RasterizerState.CullMode = static_cast<D3D12_CULL_MODE>(now_value.int_value);
	//读取alpha混合格式
	Json::Value value_blend_state = jsonRoot.get("BlendState", Json::Value::null);
	if (value_blend_state == Json::Value::null)
	{
		//无法找到alpha混合格式
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable BlendState");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	check_error = JsonLoader::GetInstance()->GetJsonData(file_name, value_blend_state, "AlphaToCoverageEnable", pancy_json_data_type::json_data_bool, now_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	desc_out.BlendState.AlphaToCoverageEnable = now_value.bool_value;
	check_error = JsonLoader::GetInstance()->GetJsonData(file_name, value_blend_state, "IndependentBlendEnable", pancy_json_data_type::json_data_bool, now_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	desc_out.BlendState.IndependentBlendEnable = now_value.bool_value;
	Json::Value value_blend_target = value_blend_state.get("RenderTarget", Json::Value::null);
	if (value_blend_target == Json::Value::null)
	{
		//无法找到alpha混合目标格式
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable BlendState::RenderTarget");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	if (value_blend_target.size() > 0)
	{
		desc_out.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		for (uint32_t i = 0; i < value_blend_target.size(); ++i)
		{
			pancy_json_value data_num[10];
			pancy_json_data_type out_data_type[] =
			{
				pancy_json_data_type::json_data_bool,
				pancy_json_data_type::json_data_bool,
				pancy_json_data_type::json_data_enum,
				pancy_json_data_type::json_data_enum,
				pancy_json_data_type::json_data_enum,
				pancy_json_data_type::json_data_enum,
				pancy_json_data_type::json_data_enum,
				pancy_json_data_type::json_data_enum,
				pancy_json_data_type::json_data_enum,
				pancy_json_data_type::json_data_enum
			};
			std::string find_name[] =
			{
				"BlendEnable",
				"LogicOpEnable",
				"SrcBlend",
				"DestBlend",
				"BlendOp",
				"SrcBlendAlpha",
				"DestBlendAlpha",
				"BlendOpAlpha",
				"LogicOp",
				"RenderTargetWriteMask"
			};
			for (int j = 0; j < 10; ++j)
			{
				check_error = JsonLoader::GetInstance()->GetJsonData(file_name, value_blend_target[i], find_name[j], out_data_type[j], data_num[j]);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
			}
			desc_out.BlendState.RenderTarget[i].BlendEnable = data_num[0].bool_value;
			desc_out.BlendState.RenderTarget[i].LogicOpEnable = data_num[1].bool_value;
			desc_out.BlendState.RenderTarget[i].SrcBlend = static_cast<D3D12_BLEND>(data_num[2].int_value);
			desc_out.BlendState.RenderTarget[i].DestBlend = static_cast<D3D12_BLEND>(data_num[3].int_value);
			desc_out.BlendState.RenderTarget[i].BlendOp = static_cast<D3D12_BLEND_OP>(data_num[4].int_value);
			desc_out.BlendState.RenderTarget[i].SrcBlendAlpha = static_cast<D3D12_BLEND>(data_num[5].int_value);
			desc_out.BlendState.RenderTarget[i].DestBlendAlpha = static_cast<D3D12_BLEND>(data_num[6].int_value);
			desc_out.BlendState.RenderTarget[i].BlendOpAlpha = static_cast<D3D12_BLEND_OP>(data_num[7].int_value);
			desc_out.BlendState.RenderTarget[i].LogicOp = static_cast<D3D12_LOGIC_OP>(data_num[8].int_value);
			desc_out.BlendState.RenderTarget[i].RenderTargetWriteMask = static_cast<UINT8>(data_num[9].int_value);
		}
	}
	//读取深度模板缓冲区格式
	Json::Value value_depthstencil_state = jsonRoot.get("DepthStencilState", Json::Value::null);
	if (value_depthstencil_state == Json::Value::null)
	{
		//无法找到深度模板缓冲区格式
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable DepthStencilState");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	std::string find_depthstencil_name[] =
	{
		"DepthEnable",
		"DepthWriteMask",
		"DepthFunc",
		"StencilEnable",
		"StencilReadMask",
		"StencilWriteMask",
	};
	pancy_json_value data_depthstencil_num[6];
	pancy_json_data_type out_data_depthstencil_type[] =
	{
		pancy_json_data_type::json_data_bool,
		pancy_json_data_type::json_data_enum,
		pancy_json_data_type::json_data_enum,
		pancy_json_data_type::json_data_bool,
		pancy_json_data_type::json_data_int,
		pancy_json_data_type::json_data_int
	};
	for (int j = 0; j < 6; ++j)
	{
		check_error = JsonLoader::GetInstance()->GetJsonData(file_name, value_depthstencil_state, find_depthstencil_name[j], out_data_depthstencil_type[j], data_depthstencil_num[j]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	desc_out.DepthStencilState.DepthEnable = data_depthstencil_num[0].bool_value;
	desc_out.DepthStencilState.DepthWriteMask = static_cast<D3D12_DEPTH_WRITE_MASK>(data_depthstencil_num[1].int_value);
	desc_out.DepthStencilState.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(data_depthstencil_num[2].int_value);
	desc_out.DepthStencilState.StencilEnable = data_depthstencil_num[3].bool_value;
	desc_out.DepthStencilState.StencilReadMask = static_cast<UINT8>(data_depthstencil_num[4].int_value);
	desc_out.DepthStencilState.StencilWriteMask = static_cast<UINT8>(data_depthstencil_num[5].int_value);
	Json::Value value_depthstencil_frontface_state = value_depthstencil_state.get("FrontFaceDesc", Json::Value::null);
	if (value_depthstencil_frontface_state == Json::Value::null)
	{
		//无法找到深度模板缓冲区格式
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable DepthStencilState::FrontFaceDesc");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	std::string find_depstencil_face_name[] =
	{
		"StencilFunc",
		"StencilDepthFailOp",
		"StencilPassOp",
		"StencilFailOp"
	};
	pancy_json_value data_depstencil_face_num[4];
	pancy_json_data_type out_data_depstencil_face_type[] =
	{
		pancy_json_data_type::json_data_enum,
		pancy_json_data_type::json_data_enum,
		pancy_json_data_type::json_data_enum,
		pancy_json_data_type::json_data_enum
	};
	for (int j = 0; j < 4; ++j)
	{
		check_error = JsonLoader::GetInstance()->GetJsonData(file_name, value_depthstencil_frontface_state, find_depstencil_face_name[j], out_data_depstencil_face_type[j], data_depstencil_face_num[j]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	desc_out.DepthStencilState.FrontFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(data_depstencil_face_num[0].int_value);
	desc_out.DepthStencilState.FrontFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(data_depstencil_face_num[1].int_value);
	desc_out.DepthStencilState.FrontFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(data_depstencil_face_num[2].int_value);
	desc_out.DepthStencilState.FrontFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(data_depstencil_face_num[3].int_value);
	Json::Value value_depthstencil_backface_state = value_depthstencil_state.get("BackFaceDesc", Json::Value::null);
	if (value_depthstencil_backface_state == Json::Value::null)
	{
		//无法找到深度模板缓冲区格式
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable DepthStencilState::BackFaceDesc");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	for (int j = 0; j < 4; ++j)
	{
		check_error = JsonLoader::GetInstance()->GetJsonData(file_name, value_depthstencil_backface_state, find_depstencil_face_name[j], out_data_depstencil_face_type[j], data_depstencil_face_num[j]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	desc_out.DepthStencilState.BackFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(data_depstencil_face_num[0].int_value);
	desc_out.DepthStencilState.BackFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(data_depstencil_face_num[1].int_value);
	desc_out.DepthStencilState.BackFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(data_depstencil_face_num[2].int_value);
	desc_out.DepthStencilState.BackFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(data_depstencil_face_num[3].int_value);
	//读取采样掩码
	check_error = JsonLoader::GetInstance()->GetJsonData(file_name, jsonRoot, "SampleMask", pancy_json_data_type::json_data_int, now_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	desc_out.SampleMask = static_cast<UINT>(now_value.int_value);
	//读取图元组织格式
	check_error = JsonLoader::GetInstance()->GetJsonData(file_name, jsonRoot, "PrimitiveTopologyType", pancy_json_data_type::json_data_enum, now_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	desc_out.PrimitiveTopologyType = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(now_value.int_value);
	//读取渲染目标的数量
	check_error = JsonLoader::GetInstance()->GetJsonData(file_name, jsonRoot, "NumRenderTargets", pancy_json_data_type::json_data_int, now_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	desc_out.NumRenderTargets = now_value.int_value;
	//读取渲染目标格式
	Json::Value data_render_target = jsonRoot.get("RTVFormats", Json::Value::null);
	if (data_render_target == Json::Value::null)
	{
		//无法找到渲染目标数据
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable data_render_target");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	if (desc_out.NumRenderTargets != data_render_target.size())
	{
		//渲染目标数量不对
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "the render_target need " + std::to_string(desc_out.NumRenderTargets) +" but find "+ std::to_string(data_render_target.size()));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	for (uint32_t i = 0; i < desc_out.NumRenderTargets; ++i)
	{
		check_error = JsonLoader::GetInstance()->GetJsonData(file_name, data_render_target, i, pancy_json_data_type::json_data_enum, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		desc_out.RTVFormats[i] = static_cast<DXGI_FORMAT>(now_value.int_value);
	}
	//读取深度模板缓冲区格式
	check_error = JsonLoader::GetInstance()->GetJsonData(file_name, jsonRoot, "DSVFormat", pancy_json_data_type::json_data_enum, now_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	desc_out.DSVFormat = static_cast<DXGI_FORMAT>(now_value.int_value);
	//读取抗锯齿格式
	Json::Value data_sample_desc = jsonRoot.get("SampleDesc", Json::Value::null);
	if (data_sample_desc == Json::Value::null)
	{
		//无法找到渲染目标数据
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable SampleDesc");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	check_error = JsonLoader::GetInstance()->GetJsonData(file_name, data_sample_desc, "Count", pancy_json_data_type::json_data_int, now_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	desc_out.SampleDesc.Count = now_value.int_value;
	check_error = JsonLoader::GetInstance()->GetJsonData(file_name, data_sample_desc, "Quality", pancy_json_data_type::json_data_int, now_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	desc_out.SampleDesc.Quality = now_value.int_value;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyEffectGraphic::BuildPso(std::string pso_config_file)
{
	PancystarEngine::EngineFailReason check_error;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSO_desc_graphic = {};
	check_error = GetDesc(pso_config_file, PSO_desc_graphic);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	PancyPiplineStateObjectGraph *new_pancy = new PancyPiplineStateObjectGraph(pso_config_file);
	check_error = new_pancy->Create(PSO_desc_graphic);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	PSO_array.insert(std::pair<std::string, PancyPiplineStateObjectGraph*>(pso_config_file, new_pancy));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyEffectGraphic::GetInputDesc(ComPtr<ID3D12ShaderReflection> t_ShaderReflection, std::vector<D3D12_INPUT_ELEMENT_DESC> &t_InputElementDescVec)
{
	HRESULT hr;
	PancystarEngine::EngineFailReason check_error;
	/*
	http://www.cnblogs.com/macom/archive/2013/10/30/3396419.html
	*/
	D3D12_SHADER_DESC t_ShaderDesc;
	hr = t_ShaderReflection->GetDesc(&t_ShaderDesc);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "get desc of shader reflect error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get input desc from shader reflect", error_message);
		return error_message;
	}
	unsigned int t_ByteOffset = 0;
	for (int i = 0; i != t_ShaderDesc.InputParameters; ++i)
	{
		D3D12_SIGNATURE_PARAMETER_DESC t_SP_DESC;
		t_ShaderReflection->GetInputParameterDesc(i, &t_SP_DESC);

		D3D12_INPUT_ELEMENT_DESC t_InputElementDesc;
		t_InputElementDesc.SemanticName = t_SP_DESC.SemanticName;
		t_InputElementDesc.SemanticIndex = t_SP_DESC.SemanticIndex;
		t_InputElementDesc.InputSlot = 0;
		t_InputElementDesc.AlignedByteOffset = t_ByteOffset;
		t_InputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		t_InputElementDesc.InstanceDataStepRate = 0;
		if (t_SP_DESC.Mask == 1)
		{
			if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32_UINT;
			}
			else if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32_SINT;
			}
			else if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32_FLOAT;
			}
			t_ByteOffset += 4;
		}
		else if (t_SP_DESC.Mask <= 3)
		{
			if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			}
			else if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			}
			else if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
			}
			t_ByteOffset += 8;
		}
		else if (t_SP_DESC.Mask <= 7)
		{
			if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			}
			else if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			}
			else if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			}
			t_ByteOffset += 12;
		}
		else if (t_SP_DESC.Mask <= 15)
		{
			if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			}
			else if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			}
			else if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			t_ByteOffset += 16;
		}
		t_InputElementDescVec.push_back(t_InputElementDesc);
	}
	return PancystarEngine::succeed;
}
PancyPiplineStateObjectGraph* PancyEffectGraphic::GetPSO(std::string name_in)
{
	auto PSO_array_find = PSO_array.find(name_in);
	if (PSO_array_find != PSO_array.end())
	{
		return PSO_array_find->second;
	}
	return NULL;
}
PancyEffectGraphic::~PancyEffectGraphic()
{
	for (auto data = PSO_array.begin(); data != PSO_array.end(); ++data)
	{
		delete data->second;
	}
	PSO_array.clear();
}