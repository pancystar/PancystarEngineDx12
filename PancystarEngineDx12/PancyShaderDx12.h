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
//RootSignature
class PancyRootSignature
{
	PancystarEngine::PancyString root_signature_name;
	ComPtr<ID3D12RootSignature> root_signature_data;
public:
	PancyRootSignature(const std::string &file_name);
	PancystarEngine::EngineFailReason Create();
};
PancyRootSignature::PancyRootSignature(const std::string &file_name)
{
	root_signature_name = file_name;
}
PancystarEngine::EngineFailReason PancyRootSignature::Create()
{
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	D3D12_ROOT_SIGNATURE_DESC desc_now;
	HRESULT hr;
	hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "serial rootsignature ID: " + std::to_string(root_signature_id) + " error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Create Root Signature", error_message);
		return error_message;
	}
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_signature_data));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create root signature ID: " + std::to_string(root_signature_id) + " failed");
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
//PSO管理器
class PancyPsoControl
{
	std::unordered_map<std::string, PancyRootSignature*> root_signature_array;
private:
	PancyPsoControl();
public:
	PancystarEngine::EngineFailReason BuildRootSignature(std::string pso_config_file);
};
PancyPsoControl::PancyPsoControl()
{
}
PancystarEngine::EngineFailReason PancyPsoControl::BuildRootSignature(std::string pso_config_file)
{
	//获取RootSignature的hash值
	//创建RootSignature
	PancyRootSignature *data_root_signature = new PancyRootSignature(pso_config_file);
	auto check_error = data_root_signature->Create(RootDesc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	root_signature_array.insert(std::pair<uint32_t, PancyRootSignature*>(HashCode, data_root_signature));
	root_signature_id = HashCode;
	return PancystarEngine::succeed;
}
class PancyEffectGraphic
{
	std::unordered_map<uint64_t, PancyPiplineStateObject*> PSO_array;
public:
	PancystarEngine::EngineFailReason BuildPso(std::string pso_config_file, const std::vector<uint32_t> &rtv_id, const uint32_t &dsv_in);
};

