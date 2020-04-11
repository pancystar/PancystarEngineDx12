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
		void InitBasicVariable() override;
	};
	
	//缓冲区资源
	class PancyBasicBuffer : public PancyCommonVirtualResource<PancyCommonBufferDesc>
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
		//将cpu数据拷贝到buffer中
		PancystarEngine::EngineFailReason WriteDataToBuffer(void* cpu_data_pointer,const pancy_resource_size &data_size);
	private:
		PancystarEngine::EngineFailReason LoadResoureDataByDesc(const PancyCommonBufferDesc &ResourceDescStruct) override;
		PancystarEngine::EngineFailReason CopyCpuDataToBufferGpu(void* cpu_data_pointer, const pancy_resource_size &data_size);
	};
	ResourceBlockGpu* GetBufferResourceData(VirtualResourcePointer &virtual_pointer, PancystarEngine::EngineFailReason &check_error);
	PancystarEngine::EngineFailReason BuildBufferResource(
		const std::string &name_resource_in,
		PancyCommonBufferDesc &resource_data,
		VirtualResourcePointer &id_need,
		bool if_allow_repeat
	);
	void InitBufferJsonReflect();
}