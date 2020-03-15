#pragma once
#include"PancyDx12Basic.h"
#include"PancyJsonTool.h"
#include"PancyMemoryBasic.h"
#include"PancyBufferDx12.h"
#include"PancyDescriptor.h"
enum PSOType
{
	PSO_TYPE_GRAPHIC = 0,
	PSO_TYPE_COMPUTE
};
enum PancyShaderDescriptorType
{
	CbufferPrivate = 0,
	CbufferGlobel,
	SRVGlobel,
	SRVPrivate,
	SRVBindless
};
struct PancyDescriptorPSODescription
{
	std::string descriptor_name;
	pancy_object_id rootsignature_slot;
};

struct RootSignatureParameterDesc
{
	D3D12_DESCRIPTOR_RANGE_TYPE range_type = D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pancy_object_id num_descriptors;
	pancy_object_id base_shader_register;
	pancy_object_id register_space;
	D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	pancy_object_id num_descriptor_ranges;
	D3D12_SHADER_VISIBILITY shader_visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;
};
class RootSignatureParameterDescJsonReflect :public PancyJsonReflectTemplate<RootSignatureParameterDesc>
{
public:
	RootSignatureParameterDescJsonReflect();
private:
	void InitBasicVariable() override;
};
class RootSignatureStaticSamplerJsonReflect :public PancyJsonReflectTemplate<D3D12_STATIC_SAMPLER_DESC>
{
public:
	RootSignatureStaticSamplerJsonReflect();
private:
	void InitBasicVariable() override;
};
struct RootSignatureDesc
{
	std::vector<RootSignatureParameterDesc> root_parameter_data;
	std::vector<D3D12_STATIC_SAMPLER_DESC> static_sampler_data;
	D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags = D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_NONE;
};
class RootSignatureDescJsonReflect :public PancyJsonReflectTemplate<RootSignatureDesc>
{
public:
	RootSignatureDescJsonReflect();
private:
	PancystarEngine::EngineFailReason InitChildReflectClass() override;
	void InitBasicVariable() override;
};

struct DescriptorTypeDesc
{
	std::string name;
	PancyShaderDescriptorType type;
};
class DescriptorTypeDescJsonReflect :public PancyJsonReflectTemplate<DescriptorTypeDesc>
{
public:
	DescriptorTypeDescJsonReflect();
private:
	void InitBasicVariable() override;
};


struct PipelineStateDescCompute 
{
	PSOType pipeline_state_type;
	std::string root_signature_file;
	std::string compute_shader_file;
	std::string compute_shader_func;
	std::vector<DescriptorTypeDesc> descriptor_type;
};
class PipelineStateDescComputeJsonReflect :public PancyJsonReflectTemplate<PipelineStateDescCompute>
{
public:
	PipelineStateDescComputeJsonReflect();
private:
	PancystarEngine::EngineFailReason InitChildReflectClass() override;
	void InitBasicVariable() override;
};



class RenderTargetBlendDescJsonReflect :public PancyJsonReflectTemplate<D3D12_RENDER_TARGET_BLEND_DESC>
{
public:
	RenderTargetBlendDescJsonReflect();
private:
	void InitBasicVariable() override;
};


struct PipelineStateDescGraphic
{
	PSOType pipeline_state_type;
	std::string root_signature_file;
	std::string vertex_shader_file;
	std::string vertex_shader_func;
	std::string pixel_shader_file;
	std::string pixel_shader_func;
	std::string geometry_shader_file;
	std::string geometry_shader_func;
	std::string hull_shader_file;
	std::string hull_shader_func;
	std::string domin_shader_file;
	std::string domin_shader_func;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc;
	std::vector<DescriptorTypeDesc> descriptor_type;
};
class PipelineStateDescGraphicJsonReflect :public PancyJsonReflectTemplate<PipelineStateDescGraphic>
{
public:
	PipelineStateDescGraphicJsonReflect();
private:
	PancystarEngine::EngineFailReason InitChildReflectClass() override;
	void InitBasicVariable() override;
};

//几何体的格式对接类型
struct PancyVertexBufferDesc
{
	std::string vertex_desc_name;
	size_t input_element_num;
	D3D12_INPUT_ELEMENT_DESC *inputElementDescs = NULL;
};
//常量缓冲区
//常量缓冲区
struct CbufferVariable
{
	pancy_resource_size variable_size;
	pancy_resource_size start_offset;
};
//todo:这里之后需要将cbuffer纳入resource管理
class PancyConstantBuffer
{
	bool if_loaded = false;
	std::string cbuffer_name;       //常量缓冲区的名称
	std::string cbuffer_effect_name; //创建常量缓冲区的渲染管线名称
	//常量缓冲区的数据
	PancystarEngine::VirtualResourcePointer buffer_ID;
	pancy_object_id buffer_offset_id;
	pancy_resource_size cbuffer_size;
	//所有成员变量的起始位置
	std::unordered_map<std::string, CbufferVariable> member_variable;
	//常量缓冲区在CPU端的指针
	UINT8* map_pointer_out;
public:
	PancyConstantBuffer();
	PancystarEngine::EngineFailReason Create(
		const std::string &cbuffer_name_in,
		const std::string &cbuffer_effect_name_in,
		const PancystarEngine::VirtualResourcePointer &buffer_id_in,
		const pancy_resource_size &buffer_offset_id_in,
		const pancy_resource_size &cbuffer_size,
		const Json::Value &root_value
	);
	inline PancystarEngine::VirtualResourcePointer &GetBufferResource()
	{
		return buffer_ID;
	}
	inline pancy_resource_size GetCbufferOffsetFromBufferHead() 
	{
		return static_cast<pancy_resource_size>(buffer_offset_id) * cbuffer_size;
	}
	inline pancy_resource_size GetCbufferSize()
	{
		return cbuffer_size;
	}
	~PancyConstantBuffer();
	PancystarEngine::EngineFailReason SetMatrix(const std::string &variable, const DirectX::XMFLOAT4X4 &mat_data, const pancy_resource_size &offset);
	PancystarEngine::EngineFailReason SetFloat4(const std::string &variable, const DirectX::XMFLOAT4 &vector_data, const pancy_resource_size &offset);
	PancystarEngine::EngineFailReason SetUint4(const std::string &variable, const DirectX::XMUINT4 &vector_data, const pancy_resource_size &offset);
	PancystarEngine::EngineFailReason SetStruct(const std::string &variable, const void* struct_data, const pancy_resource_size &data_size, const pancy_resource_size &offset);
private:
	PancystarEngine::EngineFailReason GetCbufferDesc(const std::string &file_name, const Json::Value &root_value);
	PancystarEngine::EngineFailReason ErrorVariableNotFind(const std::string &variable_name);
};
struct CbufferPackList
{
	PancystarEngine::VirtualResourcePointer buffer_pointer;
	pancy_resource_size per_cbuffer_size;
	std::unordered_set<pancy_object_id> now_use_offset;
	std::unordered_set<pancy_object_id> now_empty_offset;
	CbufferPackList(PancystarEngine::VirtualResourcePointer &buffer_data, const pancy_resource_size &per_cbuffer_size_in)
	{
		buffer_pointer = buffer_data;
		if (per_cbuffer_size_in % 256 != 0)
		{
			per_cbuffer_size = ((per_cbuffer_size_in / 256) + 1) * 256;
		}
		else
		{
			per_cbuffer_size = per_cbuffer_size_in;
		}
		const PancystarEngine::PancyBasicBuffer *pointer = dynamic_cast<const PancystarEngine::PancyBasicBuffer*>(buffer_pointer.GetResourceData());
		pancy_object_id all_member_cbuffer_num = static_cast<pancy_object_id>(pointer->GetBufferSize() / per_cbuffer_size);
		for (pancy_object_id index_offset = 0; index_offset < all_member_cbuffer_num; ++index_offset)
		{
			now_empty_offset.insert(index_offset);
		}
	}
};
class ConstantBufferAlloctor
{
	pancy_resource_size cbuffer_size;       //每个常量缓冲区的大小
	std::string cbuffer_name;               //常量缓冲区的名称
	std::string cbuffer_effect_name;        //创建常量缓冲区的渲染管线名称
	PancystarEngine::PancyCommonBufferDesc buffer_resource_desc_value;
	//Json::Value buffer_resource_desc_value; //buffer资源格式
	Json::Value cbuffer_desc_value;         //cbuffer格式
	std::unordered_map<pancy_object_id, CbufferPackList*> all_cbuffer_list;
public:
	ConstantBufferAlloctor(
		const pancy_resource_size &cbuffer_size_in,
		const std::string &cbuffer_name_in,
		const std::string &cbuffer_effect_name_in,
		const PancystarEngine::PancyCommonBufferDesc &buffer_resource_desc_value_in,
		Json::Value &cbuffer_desc_value_in
	);
	PancystarEngine::EngineFailReason BuildNewCbuffer(PancyConstantBuffer &cbuffer_data);
	PancystarEngine::EngineFailReason ReleaseCbuffer(const pancy_object_id &buffer_resource_id, const pancy_object_id &buffer_offset_id);
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
	PancystarEngine::EngineFailReason LoadShader(const std::string & shader_file, const std::string & shader_main_func, const std::string & shader_type);
	PancystarEngine::EngineFailReason GetShaderReflection(const std::string &shader_file, const std::string &shader_main_func, const std::string & shader_type, ComPtr<ID3D12ShaderReflection> *res_data);
	PancystarEngine::EngineFailReason GetShaderData(const std::string & shader_file, const std::string & shader_main_func, const std::string & shader_type, ComPtr<ID3DBlob> *res_data);
};

//RootSignature
class PancyRootSignature
{
	RootSignatureDescJsonReflect root_signature_desc_reflect;
	std::string descriptor_heap_name;
	PancystarEngine::PancyString root_signature_name;
	ComPtr<ID3D12RootSignature> root_signature_data;
public:
	PancyRootSignature(const std::string &file_name);
	PancystarEngine::EngineFailReason Create();
	inline void GetResource(ID3D12RootSignature** root_signature_data_out)
	{
		*root_signature_data_out = root_signature_data.Get();
	};
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
	~PancyRootSignatureControl();
private:
	PancystarEngine::EngineFailReason BuildRootSignature(const std::string &rootsig_config_file);
	void AddRootSignatureGlobelVariable();
};

//PSO object
class PancyPiplineStateObjectGraph
{
	PSOType pipline_type;
	//std::unordered_map<std::string,Json::Value> Cbuffer_map;
	std::unordered_map<std::string, ConstantBufferAlloctor*> Cbuffer_map;
	//todo：区分绑定资源的格式
	std::vector<PancyDescriptorPSODescription> globel_cbuffer;
	std::vector<PancyDescriptorPSODescription> private_cbuffer;
	std::vector<PancyDescriptorPSODescription> globel_shader_res;
	std::vector<PancyDescriptorPSODescription> private_shader_res;
	std::vector<PancyDescriptorPSODescription> bindless_shader_res;

	pancy_object_id root_signature_ID;
	PancystarEngine::PancyString pso_name;
	ComPtr<ID3D12PipelineState> pso_data;
	//反射读取
	PipelineStateDescGraphicJsonReflect grapthic_reflect;
	PipelineStateDescComputeJsonReflect compute_reflect;
public:
	PancyPiplineStateObjectGraph(const std::string &pso_name_in);
	~PancyPiplineStateObjectGraph();
	PancystarEngine::EngineFailReason Create();
	inline void GetResource(ID3D12PipelineState** res_data)
	{
		*res_data = pso_data.Get();
	}
	inline pancy_object_id GetRootSignature() 
	{
		return root_signature_ID;
	}
	//PancystarEngine::EngineFailReason GetDescriptorHeapUse(std::string  &descriptor_heap_name);
	//PancystarEngine::EngineFailReason GetDescriptorDistribute(std::vector<pancy_object_id> &descriptor_distribute);
	//PancystarEngine::EngineFailReason CheckCbuffer(const std::string &cbuffer_name);
	//PancystarEngine::EngineFailReason GetCbuffer(const std::string &cbuffer_name, const Json::Value *& CbufferData);
	PancystarEngine::EngineFailReason BuildCbufferByName(const std::string &cbuffer_name, PancyConstantBuffer &cbuffer_data_out);
	PancystarEngine::EngineFailReason ReleaseCbufferByID(
		const std::string &cbuffer_name, 
		const pancy_object_id &buffer_resource_id,
		const pancy_object_id &buffer_offset_id
	);
	const std::vector<PancyDescriptorPSODescription> &GetDescriptor(const PancyShaderDescriptorType &descriptor_type);
private:
	PancystarEngine::EngineFailReason GetInputDesc(ComPtr<ID3D12ShaderReflection> t_ShaderReflection, std::vector<D3D12_INPUT_ELEMENT_DESC> &t_InputElementDescVec);
	PancystarEngine::EngineFailReason BuildCbufferByShaderReflect(ComPtr<ID3D12ShaderReflection> &now_shader_reflect);
	PancystarEngine::EngineFailReason ParseDiscriptorDistribution(const std::vector<DescriptorTypeDesc> &descriptor_desc_in);
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
	//PancystarEngine::EngineFailReason GetPSODescriptorName(const pancy_object_id &PSO_id, std::string &descriptor_heap_name);
	//PancystarEngine::EngineFailReason GetDescriptorDistribute(const pancy_object_id &PSO_id, std::vector<pancy_object_id> &descriptor_distribute);
	//PancystarEngine::EngineFailReason CheckCbuffer(const pancy_object_id &PSO_id, const std::string &name_in);
	//PancystarEngine::EngineFailReason GetCbuffer(const pancy_object_id &PSO_id, const std::string &cbuffer_name, const Json::Value *& CbufferData);
	PancystarEngine::EngineFailReason BuildCbufferByName(const pancy_object_id &PSO_id, const std::string &cbuffer_name, PancyConstantBuffer &cbuffer_data_out);
	PancystarEngine::EngineFailReason ReleaseCbufferByID(
		const pancy_object_id &PSO_id,
		const std::string &cbuffer_name,
		const pancy_object_id &buffer_resource_id,
		const pancy_object_id &buffer_offset_id
	);


	PancystarEngine::EngineFailReason GetDescriptorDesc(
		const pancy_object_id &PSO_id, 
		const PancyShaderDescriptorType &descriptor_type,
		const std::vector<PancyDescriptorPSODescription> *&descriptor_param_data
	);
	~PancyEffectGraphic();
private:
	PancystarEngine::EngineFailReason BuildPso(const std::string &pso_config_file);
	void AddPSOGlobelVariable();
};
