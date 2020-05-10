#pragma once
#include"PancyRenderParam.h"
namespace PancystarEngine 
{
	enum MaterialShaderResourceType 
	{
		MaterialShaderResourceBufferBind = 0,
		MaterialShaderResourceTextureBind,
		MaterialShaderResourceBufferBindLess,
		MaterialShaderResourceTextureBindLess
	};
	//buffer资源的描述符格式
	struct PancyBufferDescriptorDesc 
	{
		//基础属性
		DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
		UINT Shader4ComponentMapping = 0;
		//buffer属性
		UINT64 FirstElement = 0;
		UINT NumElements = 0;
		UINT StructureByteStride = 0;
		D3D12_BUFFER_SRV_FLAGS Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	};
	class PancyJsonMaterialBufferDescriptor :public PancyJsonReflectTemplate<PancyBufferDescriptorDesc>
	{
	public:
		PancyJsonMaterialBufferDescriptor();
	private:
		void InitBasicVariable() override;
	};
	//buffer资源的shader绑定格式
	struct PancyMaterialShaderResourceDataBuffer
	{
		MaterialShaderResourceType shader_resource_type = MaterialShaderResourceBufferBind;
		std::string shader_resource_slot_name;
		std::vector<std::string> shader_resource_path;
		std::vector<PancyBufferDescriptorDesc> buffer_descriptor_desc;//buffer的每个buffer的格式(非bindless则只有一个)
	};
	class PancyJsonMaterialBufferShaderResource:public PancyJsonReflectTemplate<PancyMaterialShaderResourceDataBuffer>
	{
	public:
		PancyJsonMaterialBufferShaderResource();
	private:
		void InitBasicVariable() override;
	};
	//texture资源的描述符格式
	struct PancyTextureDescriptorDesc
	{
		//基础属性
		DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
		D3D12_SRV_DIMENSION ViewDimension = D3D12_SRV_DIMENSION_UNKNOWN;
		UINT Shader4ComponentMapping = 0;
		//纹理属性
		UINT MostDetailedMip = 0;
		UINT MipLevels = 0;
		UINT PlaneSlice = 0;
		UINT First2DArrayFace = 0;
		UINT NumCubes = 0;
		UINT FirstArraySlice = 0;
		UINT ArraySize = 0;
		UINT UnusedField_NothingToDefine = 0;
		FLOAT ResourceMinLODClamp = 0;
	};
	class PancyJsonMaterialTexureDescriptor :public PancyJsonReflectTemplate<PancyTextureDescriptorDesc>
	{
	public:
		PancyJsonMaterialTexureDescriptor();
	private:
		void InitBasicVariable() override;
	};
	//texture资源的shader绑定格式
	struct PancyMaterialShaderResourceDataTexture
	{
		MaterialShaderResourceType shader_resource_type = MaterialShaderResourceBufferBind;
		std::string shader_resource_slot_name;
		std::vector<std::string> shader_resource_path;
		std::vector<PancyTextureDescriptorDesc> texture_descriptor_desc;//纹理的每个buffer的格式(非bindless则只有一个)
	};
	class PancyJsonMaterialTexureShaderResource :public PancyJsonReflectTemplate<PancyMaterialShaderResourceDataTexture>
	{
	public:
		PancyJsonMaterialTexureShaderResource();
	private:
		void InitBasicVariable() override;
	};
	//普通材质的输入格式
	struct PancyCommonMaterialDesc
	{
		std::string pipeline_state_name;
		//所有用到的缓冲区
		std::vector<PancyMaterialShaderResourceDataBuffer> marterial_buffer_slot_data;
		//所有用到的纹理
		std::vector<PancyMaterialShaderResourceDataTexture> marterial_texture_slot_data;
	};
	class CommonMaterialDescJsonReflect :public PancyJsonReflectTemplate<PancyCommonMaterialDesc>
	{
	public:
		CommonMaterialDescJsonReflect();
	private:
		void InitBasicVariable() override;
	};
	class PancyMaterialBasic : public PancyCommonVirtualResource<PancyCommonMaterialDesc>
	{
		std::vector<VirtualResourcePointer> ShaderResourceData;
		//渲染所需的描述符数据
		std::unordered_map<std::string, BindDescriptorPointer> bind_shader_resource;         //私有描述符(材质专用的绑定资源，需要传入或创建)
		std::unordered_map<std::string, BindlessDescriptorPointer> bindless_shader_resource; //解绑定描述符(材质专用的解绑定资源，需要传入或创建)
	public:
		PancyMaterialBasic(const bool &if_could_reload_in);
		//根据资源格式创建资源数据
		PancystarEngine::EngineFailReason LoadResoureDataByDesc(const PancyCommonMaterialDesc &ResourceDescStruct) override;
		//创建一个渲染param
		PancystarEngine::EngineFailReason BuildRenderParam(PancyRenderParamID &render_param_id);
		bool CheckIfResourceLoadFinish() override;;
	private:
		PancystarEngine::EngineFailReason BuildBufferDescriptorByDesc(
			const std::string &resource_file_name,
			const PancyBufferDescriptorDesc &descriptor_desc,
			std::vector<VirtualResourcePointer> &resource_pointer_array,
			std::vector<BasicDescriptorDesc> &resource_descriptor_descr_array
			);
		PancystarEngine::EngineFailReason BuildTextureDescriptorByDesc(
			const std::string &resource_file_name,
			const PancyTextureDescriptorDesc &descriptor_desc,
			std::vector<VirtualResourcePointer> &resource_pointer_array,
			std::vector<BasicDescriptorDesc> &resource_descriptor_descr_array
		);
	};
	PancystarEngine::EngineFailReason LoadMaterialFromFile(
		const std::string &name_resource_in,
		VirtualResourcePointer &id_need
	);
	void InitMaterialJsonReflect();
}
