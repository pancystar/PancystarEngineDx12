#pragma once
#include"PancyDx12Basic.h"
#include"PancyJsonTool.h"
#include"PancyMemoryBasic.h"
#include"PancyBufferDx12.h"
//几何体的格式对接类型
struct PancyVertexBufferDesc
{
	std::string vertex_desc_name;
	size_t input_element_num;
	D3D12_INPUT_ELEMENT_DESC *inputElementDescs = NULL;
};
//几何体格式管理器(用于注册顶点)
class InputLayoutDesc
{
	std::unordered_map<std::string, PancyVertexBufferDesc> vertex_buffer_desc_map;
private:
	InputLayoutDesc();
public:
	static InputLayoutDesc* GetInstance()
	{
		static InputLayoutDesc* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new InputLayoutDesc();
		}
		return this_instance;
	}
	~InputLayoutDesc();
	void AddVertexDesc(std::string vertex_desc_name_in, std::vector<D3D12_INPUT_ELEMENT_DESC> input_element_desc_list);
	inline const PancyVertexBufferDesc* GetVertexDesc(std::string vertex_desc_name_in)
	{
		auto new_vertex_desc = vertex_buffer_desc_map.find(vertex_desc_name_in);
		if (new_vertex_desc != vertex_buffer_desc_map.end())
		{
			return &new_vertex_desc->second;
		}
		return NULL;
	}
};
class PancyShaderBasic
{
	PancystarEngine::PancyString shader_file_name;
	PancystarEngine::PancyString shader_entry_point_name;
	PancystarEngine::PancyString shader_type_name;
	ComPtr<ID3DBlob> shader_memory_pointer;
	ComPtr<ID3D12ShaderReflection> shader_reflection;
	//std::unordered_map<std::string, D3D12_SHADER_BUFFER_DESC> Cbuffer_map;
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
	ComPtr<ID3D12ShaderReflection> GetShaderReflect()
	{
		return shader_reflection;
	}
	PancystarEngine::EngineFailReason Create();
};
class PancyShaderControl
{
	std::unordered_map<std::string, PancyShaderBasic*> shader_list;
	PancyShaderControl();
public:
	static PancyShaderControl* GetInstance()
	{
		static PancyShaderControl* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new PancyShaderControl();
		}
		return this_instance;
	}
	~PancyShaderControl();
	PancystarEngine::EngineFailReason LoadShader(std::string shader_file, std::string shader_main_func, std::string shader_type);
	ComPtr<ID3D12ShaderReflection> GetShaderReflection(std::string shader_file, std::string shader_main_func);
	ComPtr<ID3DBlob> GetShaderData(std::string shader_file, std::string shader_main_func);
};

//RootSignature
class PancyRootSignature
{
	std::string descriptor_heap_name;
	PancystarEngine::PancyString root_signature_name;
	ComPtr<ID3D12RootSignature> root_signature_data;
	std::vector<pancy_object_id> resource_descriptor_distribution;//描述符分布情况
public:
	PancyRootSignature(const std::string &file_name);
	PancystarEngine::EngineFailReason Create();
	inline void GetResource(ID3D12RootSignature** root_signature_data_out)
	{
		*root_signature_data_out = root_signature_data.Get();
	};
	void GetDescriptorDistribute(std::vector<pancy_object_id> &descriptor_distribute);
	void GetDescriptorHeapUse(std::string &descriptor_heap_id_out);
private:
	PancystarEngine::EngineFailReason BuildResource(const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC &rootSignatureDesc);
	std::string GetJsonFileRealName(const std::string &file_name_in);
};

//RootSignature管理器
class PancyRootSignatureControl
{
	//空闲的RootSignature ID号
	pancy_object_id RootSig_id_self_add;
	std::queue<pancy_object_id> empty_RootSig_id;
	//RootSignature的存储结构
	std::unordered_map<std::string, pancy_object_id> RootSig_name;
	std::unordered_map<pancy_object_id, std::string> RootSig_name_reflect;
	std::unordered_map<pancy_object_id, PancyRootSignature*> root_signature_array;
private:
	PancyRootSignatureControl();
public:
	static PancyRootSignatureControl* GetInstance()
	{
		static PancyRootSignatureControl* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new PancyRootSignatureControl();
		}
		return this_instance;
	}
	PancystarEngine::EngineFailReason GetRootSignature(const std::string &name_in, pancy_object_id &root_signature_id);
	PancystarEngine::EngineFailReason GetResource(const pancy_object_id &root_signature_id, ID3D12RootSignature** root_signature_res);
	PancystarEngine::EngineFailReason GetDescriptorHeapUse(const pancy_object_id &root_signature_id,std::string &descriptor_heap_name);
	PancystarEngine::EngineFailReason GetDescriptorDistribute(const pancy_object_id &root_signature_id, std::vector<pancy_object_id> &descriptor_distribute);
	~PancyRootSignatureControl();
private:
	PancystarEngine::EngineFailReason BuildRootSignature(const std::string &rootsig_config_file);
	void AddRootSignatureGlobelVariable();
};

//PSO object
class PancyPiplineStateObjectGraph
{
	std::unordered_set<std::string> Cbuffer_map;
	pancy_object_id root_signature_ID;
	PancystarEngine::PancyString pso_name;
	ComPtr<ID3D12PipelineState> pso_data;
public:
	PancyPiplineStateObjectGraph(const std::string &pso_name_in);
	PancystarEngine::EngineFailReason Create();
	inline void GetResource(ID3D12PipelineState** res_data)
	{
		*res_data = pso_data.Get();
	}
	inline pancy_object_id GetRootSignature() 
	{
		return root_signature_ID;
	}
	PancystarEngine::EngineFailReason GetDescriptorHeapUse(std::string  &descriptor_heap_name);
	PancystarEngine::EngineFailReason GetDescriptorDistribute(std::vector<pancy_object_id> &descriptor_distribute);
	PancystarEngine::EngineFailReason CheckCbuffer(const std::string &cbuffer_name);
private:
	PancystarEngine::EngineFailReason GetInputDesc(ComPtr<ID3D12ShaderReflection> t_ShaderReflection, std::vector<D3D12_INPUT_ELEMENT_DESC> &t_InputElementDescVec);
};

//pso管理器
class PancyEffectGraphic
{
	//空闲的PSO ID号
	pancy_object_id PSO_id_self_add;
	std::queue<pancy_object_id> empty_PSO_id;
	//PSO的存储结构
	std::unordered_map<std::string, pancy_object_id> PSO_name;
	std::unordered_map<pancy_object_id,std::string> PSO_name_reflect;
	std::unordered_map<pancy_object_id, PancyPiplineStateObjectGraph*> PSO_array;
private:
	PancyEffectGraphic();
public:
	static PancyEffectGraphic* GetInstance()
	{
		static PancyEffectGraphic* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new PancyEffectGraphic();
		}
		return this_instance;
	}
	PancystarEngine::EngineFailReason GetPSO(const std::string &name_in,pancy_object_id &PSO_id);
	PancystarEngine::EngineFailReason GetPSOResource(const pancy_object_id &PSO_id, ID3D12PipelineState** PSO_res);
	PancystarEngine::EngineFailReason GetRootSignatureResource(const pancy_object_id &PSO_id, ID3D12RootSignature** RootSig_res);
	PancystarEngine::EngineFailReason GetPSOName(const pancy_object_id &PSO_id,std::string &pso_name_out);
	PancystarEngine::EngineFailReason GetPSODescriptorName(const pancy_object_id &PSO_id, std::string &descriptor_heap_name);
	PancystarEngine::EngineFailReason GetDescriptorDistribute(const pancy_object_id &PSO_id, std::vector<pancy_object_id> &descriptor_distribute);
	PancystarEngine::EngineFailReason CheckCbuffer(const pancy_object_id &PSO_id, const std::string &name_in);
	~PancyEffectGraphic();
private:
	PancystarEngine::EngineFailReason BuildPso(const std::string &pso_config_file);
	void AddPSOGlobelVariable();
};



