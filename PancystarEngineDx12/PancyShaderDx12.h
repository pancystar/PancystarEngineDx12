#pragma once
#include"PancyDx12Basic.h"
#include"PancyJsonTool.h"
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
//RootSignature
class PancyRootSignature
{
	PancystarEngine::PancyString root_signature_name;
	ComPtr<ID3D12RootSignature> root_signature_data;
public:
	PancyRootSignature(const std::string &file_name);
	PancystarEngine::EngineFailReason Create(const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC &rootSignatureDesc);
};
PancyRootSignature::PancyRootSignature(const std::string &file_name)
{
	root_signature_name = file_name;
}
PancystarEngine::EngineFailReason PancyRootSignature::Create(const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC &rootSignatureDesc)
{
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	D3D12_ROOT_SIGNATURE_DESC desc_now;
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
//PSO object
class PancyPiplineStateObject
{
	PancystarEngine::PancyString pso_name;
	ComPtr<ID3D12PipelineState> pso_data;
public:
	PancyPiplineStateObject(uint32_t pso_id_in);
	PancystarEngine::EngineFailReason Create(D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc_in);
};
PancyPiplineStateObject::PancyPiplineStateObject(uint32_t pso_id_in)
{
	pso_id = pso_id_in;
}
PancystarEngine::EngineFailReason PancyPiplineStateObject::Create(D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc_in)
{
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateGraphicsPipelineState(&pso_desc_in, IID_PPV_ARGS(&pso_data));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create PSO error ID: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build PSO", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
struct PancyStaticPiplineState 
{
	D3D12_RASTERIZER_DESC     RasterizerState;    //光栅化状态
	D3D12_DEPTH_STENCIL_DESC  DepthStencilState;  //深度缓冲区状态
	D3D12_BLEND_DESC          BlendState;         //混合状态
};
//RootSignature管理器
class PancyRootSignatureControl
{
	std::unordered_map<std::string, PancyRootSignature*> root_signature_array;
private:
	PancyRootSignatureControl();
public:
	void AddRootSignatureGlobelVariable();
	PancystarEngine::EngineFailReason BuildRootSignature(std::string rootsig_config_file);
	PancystarEngine::EngineFailReason GetDesc(const std::string &file_name, CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC &desc_out);
};
PancyRootSignatureControl::PancyRootSignatureControl()
{
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

	CD3DX12_DESCRIPTOR_RANGE1 *ranges = NULL;
	CD3DX12_ROOT_PARAMETER1 *rootParameters = NULL;
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

	D3D12_STATIC_SAMPLER_DESC *data_sampledesc = NULL;
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
	for (int i = 0; i < value_root_signature_flag.size(); ++i)
	{
		check_error = JsonLoader::GetInstance()->GetJsonData(file_name, value_root_signature_flag, i, pancy_json_data_type::json_data_enum, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		root_signature = root_signature | static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(now_value.int_value);
	}
	desc_out.Init_1_1(num_parameter, rootParameters, num_static_sampler, data_sampledesc, root_signature);
	if (ranges != NULL)
	{
		delete[] ranges;
	}
	if (rootParameters != NULL)
	{
		delete[] rootParameters;
	}
	if (data_sampledesc != NULL)
	{
		delete[] data_sampledesc;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyRootSignatureControl::BuildRootSignature(std::string rootsig_config_file)
{
	PancystarEngine::EngineFailReason check_error;
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
	//判断重复
	auto check_data = root_signature_array.find(rootsig_config_file);
	if (check_data != root_signature_array.end()) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL,"the root signature " + rootsig_config_file +" had been insert to map before",PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
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
	return PancystarEngine::succeed;
}
class PancyEffectGraphic
{
	std::unordered_map<std::string, PancyPiplineStateObject*> PSO_array;
public:
	void AddPSOGlobelVariable();
	PancystarEngine::EngineFailReason BuildPso(std::string pso_config_file, const std::vector<uint32_t> &rtv_id, const uint32_t &dsv_in);
	PancystarEngine::EngineFailReason GetDesc(const std::string &file_name, D3D12_GRAPHICS_PIPELINE_STATE_DESC &desc_out);
};
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
	std::string root_signature_file = now_value.string_value;
	//读取Shader文件名
	std::string shader_file_name, shader_func_name;
	check_error = JsonLoader::GetInstance()->GetJsonShader(file_name, jsonRoot, Pancy_json_shader_type::json_shader_vertex, shader_file_name, shader_func_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = JsonLoader::GetInstance()->GetJsonShader(file_name, jsonRoot, Pancy_json_shader_type::json_shader_pixel, shader_file_name, shader_func_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = JsonLoader::GetInstance()->GetJsonShader(file_name, jsonRoot, Pancy_json_shader_type::json_shader_geometry, shader_file_name, shader_func_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = JsonLoader::GetInstance()->GetJsonShader(file_name, jsonRoot, Pancy_json_shader_type::json_shader_hull, shader_file_name, shader_func_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = JsonLoader::GetInstance()->GetJsonShader(file_name, jsonRoot, Pancy_json_shader_type::json_shader_domin, shader_file_name, shader_func_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
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
		for (int i = 0; i < value_blend_target.size(); ++i)
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
	//读取网格组织格式
	check_error = JsonLoader::GetInstance()->GetJsonData(file_name, jsonRoot, "PrimitiveTopologyType", pancy_json_data_type::json_data_enum, now_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	desc_out.PrimitiveTopologyType = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(now_value.int_value);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyEffectGraphic::BuildPso(std::string pso_config_file, const std::vector<uint32_t> &rtv_id, const uint32_t &dsv_in) 
{
	PancystarEngine::EngineFailReason check_error;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSO_desc_graphic;
	check_error = GetDesc(pso_config_file, PSO_desc_graphic);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
