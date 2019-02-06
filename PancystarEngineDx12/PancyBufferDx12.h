#pragma once
#include"PancyResourceBasic.h"
#include"PancyThreadBasic.h"
#define MaxWasteSpace 16777216
//Cbuffer资源堆(开辟范围16K-64k,区间精度1K)
#define ConstantBufferHeapAliaze256K 262144
#define ConstantBufferSubResourceAliaze256K 1024
//Cbuffer资源堆(开辟范围0-16k,区间精度256B)
#define ConstantBufferHeapAliaze64K 65536
#define ConstantBufferSubResourceAliaze64K 256

//64M的资源堆(开辟空间范围为4M-16M，区间精度4M)
#define BufferHeapAliaze64M 67108864
#define BufferSubResourceAliaze64M 4194304
//16M的资源堆(开辟空间范围为1M-4M，区间精度512k,整体最大空闲浪费量80-90M)
#define BufferHeapAliaze16M 16777216
#define BufferSubResourceAliaze16M 524288
//4M的资源堆(开辟空间范围为256k-1M,区间精度128k)
#define BufferHeapAliaze4M 4194304
#define BufferSubResourceAliaze4M 131072
//1M的资源堆(开辟空间范围为0-256k,区间精度32k)
#define BufferHeapAliaze1M 1048576
#define BufferSubResourceAliaze1M 65536
namespace PancystarEngine 
{
	enum PancyBufferType 
	{
		Buffer_ShaderResource_static = 0,
		Buffer_ShaderResource_dynamic,
		Buffer_Constant,
		Buffer_Vertex,
		Buffer_Index
	};
	class PancyBasicBuffer : public PancyBasicVirtualResource
	{
		PancyBufferType  buffer_type;
		SubMemoryPointer buffer_data;     //buffer数据指针
		pancy_object_id  upload_buffer_id;
		PancyFenceIdGPU  WaitFence;       //需要等待的GPU眼位号
	public:
		PancyBasicBuffer(const std::string &resource_name_in, Json::Value root_value_in);
		inline SubMemoryPointer GetBufferSubResource()
		{
			return buffer_data;
		}
	private:
		PancystarEngine::EngineFailReason InitResource(const Json::Value &root_value, const std::string &resource_name, ResourceStateType &now_res_state);
		//重写更新GPU资源到CPU的函数
		PancystarEngine::EngineFailReason UpdateResourceToGPU(
			ResourceStateType &now_res_state,
			void* resource,
			const pancy_resource_size &resource_size_in,
			const pancy_resource_size &resource_offset_in
		);
		//检测当前的资源是否已经被载入GPU
		void CheckIfResourceLoadToGpu(ResourceStateType &now_res_state);
	};
	class PancyBasicBufferControl :public PancyBasicResourceControl
	{
		PancyBasicBufferControl(const std::string &resource_type_name_in);
	public:
		static PancyBasicBufferControl* GetInstance()
		{
			static PancyBasicBufferControl* this_instance;
			if (this_instance == NULL)
			{
				this_instance = new PancyBasicBufferControl("Buffer Resource Control");
			}
			return this_instance;
		}
		//根据指定的纹理资源类型，创建一套确定buffer资源格式的json文件(heap,subresource)
		PancystarEngine::EngineFailReason BuildBufferTypeJson(
			const PancyBufferType &buffer_type,
			const pancy_resource_size &data_size,
			std::string &subresource_desc_name
		);
		//使用标准的DirectX资源更新buffer，与void*数据不同，要求必须是动态buffer，因此可以不需要更新缓冲区状态
		PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
			const pancy_object_id  &resource_id,
			const pancy_resource_size &pointer_offset,
			std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
			UINT64* pRowSizesInBytes,
			UINT* pNumRows
		);
		//获取buffer数据的submemory指针
		PancystarEngine::EngineFailReason GetBufferSubResource(const pancy_object_id  &resource_id, SubMemoryPointer &submemory);
	private:
		PancystarEngine::EngineFailReason BuildResource(
			const Json::Value &root_value,
			const std::string &name_resource_in,
			PancyBasicVirtualResource** resource_out
		);
	};
	
	
}