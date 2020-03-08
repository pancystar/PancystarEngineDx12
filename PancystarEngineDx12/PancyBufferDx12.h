#pragma once
#include"PancyResourceBasic.h"
#include"PancyThreadBasic.h"
namespace PancystarEngine 
{
	enum PancyBufferType 
	{
		Buffer_ShaderResource_static = 0,
		Buffer_ShaderResource_dynamic,
		Buffer_Constant,
		Buffer_Vertex,
		Buffer_Index,
		Buffer_UnorderedAccess_static
	};
	struct PancyCommonBufferDesc 
	{
		PancyBufferType buffer_type;
		D3D12_RESOURCE_DESC buffer_res_desc = {};
		std::string buffer_data_file;
	};
	class CommonBufferJsonReflect :public PancyJsonReflectTemplate<PancyCommonBufferDesc>
	{ 
	public:
		CommonBufferJsonReflect();
	private:
		PancystarEngine::EngineFailReason InitChildReflectClass() override;
		void InitBasicVariable() override;
	};
	CommonBufferJsonReflect::CommonBufferJsonReflect() 
	{
		
	}
	PancystarEngine::EngineFailReason CommonBufferJsonReflect::InitChildReflectClass()
	{
	}
	void CommonBufferJsonReflect::InitBasicVariable() 
	{
		Init_Json_Data_Vatriable(reflect_data.buffer_type);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Dimension);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Alignment);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Width);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Height);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.DepthOrArraySize);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.MipLevels);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Format);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.SampleDesc.Count);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.SampleDesc.Quality);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Layout);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Flags);
		Init_Json_Data_Vatriable(reflect_data.buffer_data_file);
	}
	//缓冲区资源
	class PancyBasicBuffer : public PancyBasicVirtualResource
	{
		pancy_resource_size subresources_size = 0;
		UINT8* map_pointer = NULL;
		ResourceBlockGpu *buffer_data = nullptr;     //buffer数据指针
	public:
		PancyBasicBuffer(const bool &if_could_reload);
		~PancyBasicBuffer();
		inline const pancy_resource_size GetBufferSize() const
		{
			return subresources_size;
		}
		inline UINT8* GetBufferCPUPointer() const
		{
			return map_pointer;
		}
		inline ResourceBlockGpu *GetGpuResourceData() const
		{
			return buffer_data;
		}
		//检测当前的资源是否已经被载入GPU
		bool CheckIfResourceLoadFinish() override;
	private:
		void BuildJsonReflect(PancyJsonReflect **pointer_data) override;
		PancystarEngine::EngineFailReason InitResource() override;
	};
	static ResourceBlockGpu* GetBufferResourceData(VirtualResourcePointer &virtual_pointer, PancystarEngine::EngineFailReason &check_error);
	static PancystarEngine::EngineFailReason BuildBufferResource(
		const std::string &name_resource_in,
		PancyCommonBufferDesc &resource_data,
		VirtualResourcePointer &id_need,
		bool if_allow_repeat
	);
}