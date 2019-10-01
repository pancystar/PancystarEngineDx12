#pragma once
#include"PancyDx12Basic.h"
#include"PancyThreadBasic.h"
#include<DirectXTex.h>
#include<LoaderHelpers.h>
#include<DDSTextureLoader.h>
#include<WICTextureLoader.h>
#define MaxHeapDivide 32
//显存指针
struct VirtualMemoryPointer
{
	bool if_heap;//资源是否是由堆上分配而来
	pancy_resource_id heap_type;      //堆类型
	pancy_resource_id heap_list_id;   //堆id号
	pancy_resource_id memory_block_id;//内存块id号
	pancy_object_id memory_resource_id;//直接指向内存的id号
	VirtualMemoryPointer()
	{
		if_heap = false;
		heap_type = 0;
		heap_list_id = 0;
		memory_block_id = 0;
		memory_resource_id = 0;
	}
};
//显存块
class MemoryBlockGpu
{
	pancy_resource_size memory_size;//存储块的大小
	ComPtr<ID3D12Resource> resource_data;//存储块的数据
	D3D12_HEAP_TYPE resource_usage;
	UINT8* map_pointer;
	D3D12_RESOURCE_STATES now_subresource_state;//当前资源的使用格式
public:
	MemoryBlockGpu(
		const uint64_t &memory_size_in,
		ComPtr<ID3D12Resource> resource_data_in,
		const D3D12_HEAP_TYPE &resource_usage_in,
		const D3D12_RESOURCE_STATES &resource_state
	);
	~MemoryBlockGpu();
	inline ComPtr<ID3D12Resource> GetResource() 
	{
		return resource_data;
	}
	inline uint64_t GetSize()
	{
		return memory_size;
	}
	//查看当前资源的使用格式
	inline D3D12_RESOURCE_STATES GetResourceState()
	{
		return now_subresource_state;
	}
	//修改当前资源的使用格式
	inline void SetResourceState(const D3D12_RESOURCE_STATES &state)
	{
		now_subresource_state = state;
	}
	PancystarEngine::EngineFailReason WriteFromCpuToBuffer(const pancy_resource_size &pointer_offset, const void* copy_data, const pancy_resource_size data_size);
	PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
		const pancy_resource_size &pointer_offset,
		std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		UINT64* pRowSizesInBytes,
		UINT* pNumRows
	);
	PancystarEngine::EngineFailReason GetCpuMapPointer(UINT8** map_pointer_out)
	{
		if (resource_usage != D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "resource type is not upload, could not copy data to memory");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get CPU Pointer of memory block gpu", error_message);
			map_pointer_out = NULL;
			return error_message;
		}
		*map_pointer_out = map_pointer;
		return PancystarEngine::succeed;
	}
	PancystarEngine::EngineFailReason ReadFromBufferToCpu(const pancy_resource_size &pointer_offset, void* copy_data, const pancy_resource_size data_size);
};
//保留显存堆
class MemoryHeapGpu
{
	std::string heap_type_name;
	pancy_resource_size size_per_block;
	pancy_resource_id max_block_num;
	ComPtr<ID3D12Heap> heap_data;
	//std::unordered_set<pancy_resource_id> free_list;

	//所有显存的数据
	std::unordered_map<pancy_resource_id, MemoryBlockGpu*> memory_heap_block;
	//空出的显存
	std::unordered_set<pancy_resource_id> empty_memory_block;
public:
	MemoryHeapGpu(const std::string &heap_type_name_in);
	//每个显存块的大小
	inline pancy_resource_size GetMemorySizePerBlock()
	{
		return size_per_block;
	}
	//显存堆的总显存块数量
	inline pancy_resource_id GetMaxMemoryBlockNum()
	{
		return max_block_num;
	}
	//显存堆尚未使用的显存大小
	inline size_t GetFreeMemoryBlockNum()
	{
		return empty_memory_block.size();
	}
	PancystarEngine::EngineFailReason Create(const CD3DX12_HEAP_DESC &heap_desc_in, const uint64_t &size_per_block_in, const pancy_resource_id &max_block_num_in);
	//从显存堆开辟资源
	PancystarEngine::EngineFailReason BuildMemoryResource(
		const D3D12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		pancy_resource_id &memory_block_ID
	);
	//获取显存资源
	MemoryBlockGpu* GetMemoryResource(const pancy_resource_id &memory_block_ID);
	//检验对应id的资源是否已经被分配
	bool CheckIfFree(pancy_resource_id memory_block_ID);
	//释放一个对应id的资源
	PancystarEngine::EngineFailReason FreeMemoryReference(const pancy_resource_id &memory_block_ID);
	~MemoryHeapGpu();
};
//线性增长的显存堆
class MemoryHeapLinear
{
	//显存堆的格式
	CD3DX12_HEAP_DESC heap_desc;
	pancy_resource_size size_per_block;
	pancy_resource_id max_block_num;
	//显存堆的名称
	std::string heap_type_name;

	//所有显存堆的数据
	std::unordered_map<pancy_resource_id, MemoryHeapGpu*> memory_heap_data;
	//空出的显存堆
	std::unordered_set<pancy_resource_id> empty_memory_heap;
public:
	MemoryHeapLinear(const std::string &heap_type_name_in, const CD3DX12_HEAP_DESC &heap_desc_in, const pancy_resource_size &size_per_block_in, const pancy_resource_id &max_block_num_in);
	//从显存堆开辟资源
	PancystarEngine::EngineFailReason BuildMemoryResource(
		const D3D12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		pancy_resource_id &memory_block_ID,//显存块地址指针
		pancy_resource_id &memory_heap_ID//显存段地址指针
	);
	MemoryBlockGpu* GetMemoryResource(
		const pancy_resource_id &memory_heap_ID,//显存段地址指针
		const pancy_resource_id &memory_block_ID//显存块地址指针
		
	);
	//释放一个对应id的资源
	PancystarEngine::EngineFailReason FreeMemoryReference(
		const pancy_resource_id &memory_heap_ID,
		const pancy_resource_id &memory_block_ID
	);
	//获取当前堆类型开启的堆数量
	inline pancy_object_id GetHeapNum()
	{
		return memory_heap_data.size();
	}
	//获取当前堆类型的每个堆的大小
	inline pancy_resource_size GetPerHeapSize()
	{
		return heap_desc.SizeInBytes;
	}
	~MemoryHeapLinear();
};
//资源管理器
class MemoryHeapGpuControl
{
	pancy_object_id resource_memory_id_self_add;
	std::unordered_map<std::string, pancy_resource_id> resource_init_list;
	std::unordered_map<pancy_resource_id, MemoryHeapLinear*> resource_heap_list;//显存堆表
	std::unordered_map<pancy_object_id, MemoryBlockGpu*>resource_memory_list;//离散的显存块表
	std::unordered_set<pancy_object_id> resource_memory_free_id;
	MemoryHeapGpuControl();
public:
	static MemoryHeapGpuControl* GetInstance()
	{
		static MemoryHeapGpuControl* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new MemoryHeapGpuControl();
		}
		return this_instance;
	}
	PancystarEngine::EngineFailReason BuildResourceCommit(
		const D3D12_HEAP_TYPE &heap_type_in,
		const D3D12_HEAP_FLAGS &heap_flag_in,
		const CD3DX12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		VirtualMemoryPointer &virtual_pointer
	);
	PancystarEngine::EngineFailReason BuildResourceFromHeap(
		const std::string &HeapFileName,
		const D3D12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		VirtualMemoryPointer &virtual_pointer
	);
	MemoryBlockGpu *GetMemoryResource(const VirtualMemoryPointer &virtual_pointer);
	PancystarEngine::EngineFailReason FreeResource(const VirtualMemoryPointer &virtual_pointer);
	void GetHeapDesc(const pancy_resource_id &heap_id, pancy_object_id &heap_num,pancy_resource_size &per_heap_size);
	void GetHeapDesc(const std::string &heap_name, pancy_object_id &heap_num, pancy_resource_size &per_heap_size);
	~MemoryHeapGpuControl();
private:
	//不存放在指定堆上的资源
	MemoryBlockGpu* GetMemoryFromList(const pancy_object_id &memory_block_ID);
	PancystarEngine::EngineFailReason FreeResourceCommit(const pancy_object_id &memory_block_ID);
	//存放在指定堆上的资源
	MemoryBlockGpu * GetMemoryResourceFromHeap(
		const pancy_resource_id &memory_heap_list_ID,//显存域地址指针
		const pancy_resource_id &memory_heap_ID,//显存段地址指针
		const pancy_resource_id &memory_block_ID//显存块地址指针
	);
	
	PancystarEngine::EngineFailReason LoadHeapFromFile(
		const std::string &HeapFileName,
		pancy_resource_id &resource_id,
		uint64_t heap_alignment_size = 0
	);
	PancystarEngine::EngineFailReason FreeResourceFromHeap(
		const pancy_resource_id &memory_heap_list_ID,//显存域地址指针
		const pancy_resource_id &memory_heap_ID,//显存段地址指针
		const pancy_resource_id &memory_block_ID//显存块地址指针
	);
	PancystarEngine::EngineFailReason BuildHeap(
		const std::string &HeapFileName,
		const pancy_resource_size &heap_size,
		const pancy_resource_size &per_block_size,
		const D3D12_HEAP_TYPE &heap_type_in,
		const D3D12_HEAP_FLAGS &heap_flag_in,
		pancy_resource_id &resource_id,
		uint64_t heap_alignment_size = 0
	);
};
//二级资源
struct SubMemoryPointer 
{
	pancy_resource_id type_id;
	pancy_object_id list_id;
	pancy_object_id offset;
};
class SubMemoryData 
{
	VirtualMemoryPointer buffer_data;//显存资源指针
	pancy_resource_size per_memory_size;//每个缓冲区的大小
	std::unordered_set<pancy_object_id> empty_sub_memory;
	std::unordered_set<pancy_object_id> sub_memory_data;
public:
	SubMemoryData();
	PancystarEngine::EngineFailReason Create(
		const std::string &buffer_desc_file,
		const D3D12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		const pancy_object_id &per_memory_size_in
	);
	PancystarEngine::EngineFailReason BuildSubMemory(pancy_object_id &offset);
	PancystarEngine::EngineFailReason FreeSubMemory(const pancy_object_id &offset);
	//查看当前空闲资源的大小
	inline pancy_object_id GetEmptySize()
	{
		return static_cast<pancy_object_id>(empty_sub_memory.size());
	}
	
	//获取资源
	inline MemoryBlockGpu* GetResource()
	{
		return MemoryHeapGpuControl::GetInstance()->GetMemoryResource(buffer_data);
	}
	void GetLogMessage(std::vector<std::string> &log_message);
	void GetLogMessage(Json::Value &root_value,bool &if_empty);
	inline pancy_resource_size GetBlockSize()
	{
		return static_cast<pancy_resource_size>(per_memory_size);
	}
};
class SubresourceLiner 
{
	std::string heap_name;
	std::string hash_name;
	D3D12_RESOURCE_DESC resource_desc;
	D3D12_RESOURCE_STATES resource_state;
	pancy_object_id per_memory_size;
	std::unordered_map<pancy_object_id, SubMemoryData*> submemory_list;
	//空出的显存
	std::unordered_set<pancy_object_id> empty_memory_heap;
	//当前最大的id号以及已经释放的id号
	pancy_object_id max_id;
	std::unordered_set<pancy_object_id> free_id;
public:
	SubresourceLiner(
		const std::string &heap_name_in,
		const std::string &hash_name_in,
		const D3D12_RESOURCE_DESC &resource_desc_in,
		const D3D12_RESOURCE_STATES &resource_state_in,
		const pancy_object_id &per_memory_size_in
	);
	~SubresourceLiner();
	inline std::string GetHeapName() 
	{
		return heap_name;
	}
	inline std::string GetResourceName()
	{
		return hash_name;
	}
	void GetLogMessage(std::vector<std::string> &log_message);
	void GetLogMessage(Json::Value &root_value);
	PancystarEngine::EngineFailReason BuildSubresource(
		pancy_object_id &new_memory_block_id, 
		pancy_object_id &sub_memory_offset
	);
	PancystarEngine::EngineFailReason ReleaseSubResource(
		const pancy_object_id &new_memory_block_id,
		const pancy_object_id &sub_memory_offset
	);
	MemoryBlockGpu* GetSubResource(pancy_object_id sub_memory_id, pancy_resource_size &per_memory_size);
};
class SubresourceControl
{
	pancy_object_id subresource_id_self_add;
	std::unordered_map<pancy_resource_id, SubresourceLiner*> subresource_list_map;
	std::unordered_map<std::string, pancy_resource_id> subresource_init_list;
	std::unordered_set<pancy_object_id> subresource_free_id;
private:
	SubresourceControl();
public:
	~SubresourceControl();
	static SubresourceControl* GetInstance()
	{
		static SubresourceControl* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new SubresourceControl();
		}
		return this_instance;
	}
	PancystarEngine::EngineFailReason BuildSubresourceFromFile(
		const std::string &resource_name_in,
		SubMemoryPointer &submemory_pointer
	);
	PancystarEngine::EngineFailReason FreeSubResource(const SubMemoryPointer &submemory_pointer);
	PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
		const SubMemoryPointer &submemory_pointer,
		const pancy_resource_size &pointer_offset, 
		const void* copy_data, 
		const pancy_resource_size data_size
	);
	PancystarEngine::EngineFailReason GetBufferCpuPointer(
		const SubMemoryPointer &submemory_pointer,
		UINT8** map_pointer_out
	);
	PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
		const SubMemoryPointer &submemory_pointer,
		const pancy_resource_size &pointer_offset,
		std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		UINT64* pRowSizesInBytes,
		UINT* pNumRows
	);
	PancystarEngine::EngineFailReason CopyResource(
		PancyRenderCommandList *commandlist,
		const SubMemoryPointer &src_submemory,
		const SubMemoryPointer &dst_submemory,
		const pancy_resource_size &src_offset,
		const pancy_resource_size &dst_offset,
		const pancy_resource_size &data_size
	);
	PancystarEngine::EngineFailReason CopyResource(
		PancyRenderCommandList *commandlist,
		const SubMemoryPointer &src_submemory,
		const SubMemoryPointer &dst_submemory,
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		const pancy_object_id &Layout_num
	);
	PancystarEngine::EngineFailReason ResourceBarrier(
		PancyRenderCommandList *commandlist,
		const SubMemoryPointer &src_submemory,
		const D3D12_RESOURCE_STATES &last_state,
		const D3D12_RESOURCE_STATES &now_state
		);
	PancystarEngine::EngineFailReason GetResourceState(const SubMemoryPointer &src_submemory, D3D12_RESOURCE_STATES &res_state);
	PancystarEngine::EngineFailReason CaptureTextureDataToWindows(
		const SubMemoryPointer &tex_data,
		const bool &if_cube_map,
		DirectX::ScratchImage *new_image
	);
	PancystarEngine::EngineFailReason GetSubResourceDesc(
		const SubMemoryPointer & tex_data,
		D3D12_RESOURCE_DESC &resource_desc
	);
	PancystarEngine::EngineFailReason BuildConstantBufferView(
		const SubMemoryPointer &src_submemory,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor
	);
	PancystarEngine::EngineFailReason BuildShaderResourceView(
		const SubMemoryPointer &src_submemory,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
		const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
	);
	PancystarEngine::EngineFailReason BuildRenderTargetView(
		const SubMemoryPointer &src_submemory,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
		const D3D12_RENDER_TARGET_VIEW_DESC  &RTV_desc
	);
	PancystarEngine::EngineFailReason BuildUnorderedAccessView(
		const SubMemoryPointer &src_submemory,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC  &UAV_desc
	);
	PancystarEngine::EngineFailReason BuildDepthStencilView(
		const SubMemoryPointer &src_submemory,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
		const D3D12_DEPTH_STENCIL_VIEW_DESC  &DSV_desc
	);
	PancystarEngine::EngineFailReason BuildVertexBufferView(
		const SubMemoryPointer &src_submemory,
		UINT StrideInBytes,
		D3D12_VERTEX_BUFFER_VIEW &VBV_out
	);
	PancystarEngine::EngineFailReason BuildIndexBufferView(
		const SubMemoryPointer &src_submemory,
		DXGI_FORMAT StrideInBytes,
		D3D12_INDEX_BUFFER_VIEW &IBV_out
	);
	void WriteSubMemoryMessageToFile(const std::string &log_file_name);
	//void WriteSubMemoryMessageToFile(Json::Value &root_value);
private:
	MemoryBlockGpu*  GetResourceData(const SubMemoryPointer &submemory_pointer, pancy_resource_size &per_memory_size);
	void InitSubResourceType(
		const std::string &hash_name,
		const std::string &heap_name_in,
		const D3D12_RESOURCE_DESC &resource_desc_in,
		const D3D12_RESOURCE_STATES &resource_state_in,
		const pancy_object_id &per_memory_size_in,
		pancy_resource_id &subresource_type_id
	);
	PancystarEngine::EngineFailReason BuildSubresource(
		const std::string &hash_name,
		const std::string &heap_name_in,
		const D3D12_RESOURCE_DESC &resource_desc_in,
		const D3D12_RESOURCE_STATES &resource_state_in,
		const pancy_object_id &per_memory_size_in,
		SubMemoryPointer &submemory_pointer
	);
};
//资源描述视图
struct ResourceViewPack 
{
	pancy_resource_id descriptor_heap_type_id;
	pancy_resource_id descriptor_heap_offset;
};
struct ResourceViewPointer 
{
	ResourceViewPack resource_view_pack_id;
	pancy_resource_id resource_view_offset_id;
};
class PancyResourceView
{
	ComPtr<ID3D12DescriptorHeap> heap_data;
	D3D12_DESCRIPTOR_HEAP_TYPE resource_type;
	//资源视图的位置
	int32_t heap_offset;
	//资源视图包含的资源
	uint32_t resource_view_number;
	pancy_object_id resource_block_size;
public:
	PancyResourceView(
		ComPtr<ID3D12DescriptorHeap> heap_data_in,
		const int32_t &heap_offset_in,
		D3D12_DESCRIPTOR_HEAP_TYPE &resource_type_in,
		const int32_t &view_number_in
	);
	PancystarEngine::EngineFailReason BuildSRV(
		const pancy_object_id &self_offset, 
		const SubMemoryPointer &resource_in,
		const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
	);
	PancystarEngine::EngineFailReason BuildCBV(
		const pancy_object_id &self_offset,
		const SubMemoryPointer &resource_in
	);
	PancystarEngine::EngineFailReason BuildUAV(
		const pancy_object_id &self_offset,
		const SubMemoryPointer &resource_in,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc
	);
	PancystarEngine::EngineFailReason BuildRTV(
		const pancy_object_id &self_offset,
		const SubMemoryPointer &resource_in,
		const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc
	);
	PancystarEngine::EngineFailReason BuildDSV(
		const pancy_object_id &self_offset,
		const SubMemoryPointer &resource_in,
		const D3D12_DEPTH_STENCIL_VIEW_DESC    &DSV_desc
	);
};


struct ResourceViewPointer
{
	pancy_resource_id resource_view_offset;
	pancy_resource_id resource_view_num ;
};
//用于在描述符堆上进行快速存储区域开辟的指针数据

struct DescriptorHeapPointer
{
	//资源块的起始指针位置
	pancy_resource_size resource_pointer_start;
	//资源块的当前开辟指针位置(默认从后往前生长)
	pancy_resource_size resource_pointer_now;
	//资源块的结束指针位置
	pancy_resource_size resource_pointer_end;
	//资源块的待释放位置
	pancy_resource_size resource_pointer_free;
	DescriptorHeapPointer()
	{
		resource_pointer_start = 0;
		resource_pointer_now = 0;
		resource_pointer_end = 0;
		resource_pointer_free = 0;
	}
};
class PancyDescriptorHeap 
{
	string descriptor_heap_name;  // 描述符管理堆的名称
	int globel_resource_view_num; //描述符管理堆最大支持的全局描述符数量
	int constant_buffer_view_num; // 描述符管理堆最大支持的常量缓冲区描述符数量
	DescriptorHeapPointer private_resource_view_num_8;  //8个一组的连续描述符包
	DescriptorHeapPointer private_resource_view_num_16; //16个一组的连续描述符包
	DescriptorHeapPointer private_resource_view_num_32; //32个一组的连续描述符包
	DescriptorHeapPointer private_resource_view_num_64; //64个一组的连续描述符包
	//所有描述符的数据
	std::unordered_map<pancy_object_id, ResourceViewPointer> resource_view_heap_block;
public:
	PancyDescriptorHeap();
	PancystarEngine::EngineFailReason BuildDescriptorGlobel();

};
//资源描述符管理堆
class PancyDescriptorHeap 
{
	std::string descriptor_heap_name;
	pancy_object_id heap_block_size;    //描述符每个存储块的大小(拥有的视图数量)
	pancy_object_id heap_block_num;     //描述符的存储块数量
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
	ComPtr<ID3D12DescriptorHeap> heap_data;
	UINT per_offset_size;
	//所有描述符的数据
	std::unordered_map<pancy_object_id, PancyResourceView*> resource_view_heap_block;
	//空出的描述符
	std::unordered_set<pancy_object_id> empty_view_block;
public:
	PancyDescriptorHeap(
		const std::string &descriptor_heap_name_in,
		const pancy_object_id &heap_block_size_in,
		const D3D12_DESCRIPTOR_HEAP_DESC &heap_desc_in
	);
	~PancyDescriptorHeap();
	inline std::string GetDescriptorName()
	{
		return descriptor_heap_name;
	};
	PancystarEngine::EngineFailReason Create();
	PancystarEngine::EngineFailReason BuildHeapBlock(pancy_resource_id &resource_view_ID);
	PancystarEngine::EngineFailReason FreeHeapBlock(const pancy_resource_id &resource_view_ID);
	PancystarEngine::EngineFailReason GetDescriptorHeap(ID3D12DescriptorHeap **descriptor_heap_use);
	inline PancystarEngine::EngineFailReason GetOffsetNum(pancy_resource_id heap_offset, pancy_object_id self_offset, CD3DX12_GPU_DESCRIPTOR_HANDLE &descriptor_table)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(heap_data->GetGPUDescriptorHandleForHeapStart());
		pancy_object_id id_offset = static_cast<pancy_object_id>(heap_offset) * heap_block_size * per_offset_size + self_offset * per_offset_size;
		srvHandle.Offset(id_offset);
		descriptor_table = srvHandle;
		return PancystarEngine::succeed;
	}
	inline pancy_object_id GetOffsetNum(pancy_resource_id heap_offset, pancy_object_id self_offset, CD3DX12_CPU_DESCRIPTOR_HANDLE &descriptor_table)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(heap_data->GetCPUDescriptorHandleForHeapStart());
		pancy_object_id id_offset = static_cast<pancy_object_id>(heap_offset) * heap_block_size * per_offset_size + self_offset * per_offset_size;
		srvHandle.Offset(id_offset);
		descriptor_table = srvHandle;
		return id_offset;
	}
	//创建资源视图
	PancystarEngine::EngineFailReason BuildSRV(
		const pancy_object_id &descriptor_block_id, 
		const pancy_object_id &self_offset,
		const SubMemoryPointer &resource_in,
		const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
	);
	PancystarEngine::EngineFailReason BuildCBV(
		const pancy_object_id &descriptor_block_id, 
		const pancy_object_id &self_offset, 
		const SubMemoryPointer &resource_in
	);
	PancystarEngine::EngineFailReason BuildUAV(
		const pancy_object_id &descriptor_block_id, 
		const pancy_object_id &self_offset, 
		const SubMemoryPointer &resource_in,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc
	);
	PancystarEngine::EngineFailReason BuildRTV(
		const pancy_object_id &descriptor_block_id, 
		const pancy_object_id &self_offset, 
		const SubMemoryPointer &resource_in,
		const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc
	);
	PancystarEngine::EngineFailReason BuildDSV(
		const pancy_object_id &descriptor_block_id,
		const pancy_object_id &self_offset,
		const SubMemoryPointer &resource_in,
		const D3D12_DEPTH_STENCIL_VIEW_DESC    &DSV_desc
	);
private:
	PancyResourceView* GetHeapBlock(const pancy_resource_id &resource_view_ID,PancystarEngine::EngineFailReason &check_error);
};
//资源描述符堆管理器
class PancyDescriptorHeapControl
{
	pancy_resource_id descriptor_heap_id_selfadd;
	std::unordered_map<std::string, pancy_resource_id> resource_init_list;
	std::unordered_map<pancy_resource_id, PancyDescriptorHeap*> resource_heap_list;
	std::unordered_set<pancy_resource_id> resource_memory_free_id;
	PancyDescriptorHeapControl();
public:
	static PancyDescriptorHeapControl* GetInstance()
	{
		static PancyDescriptorHeapControl* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new PancyDescriptorHeapControl();
		}
		return this_instance;
	}
	PancystarEngine::EngineFailReason BuildResourceViewFromFile(
		const std::string &file_name,
		ResourceViewPack &RSV_pack_id,
		pancy_object_id &per_resource_view_pack_size
	);
	inline PancystarEngine::EngineFailReason FreeResourceView(const ResourceViewPack &RSV_pack_id) 
	{
		auto heap_data = resource_heap_list.find(RSV_pack_id.descriptor_heap_type_id);
		if (heap_data == resource_heap_list.end()) 
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL,"could not find resource view type:"+std::to_string(RSV_pack_id.descriptor_heap_type_id));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free Resource View from descriptor heap",error_message);
			return error_message;
		}
		PancystarEngine::EngineFailReason check_error = heap_data->second->FreeHeapBlock(RSV_pack_id.descriptor_heap_offset);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
	PancystarEngine::EngineFailReason FreeDescriptorHeap(
		pancy_resource_id &descriptor_heap_id
	);
	PancystarEngine::EngineFailReason GetDescriptorHeap(const ResourceViewPack &heap_id, ID3D12DescriptorHeap **descriptor_heap_use);
	inline PancystarEngine::EngineFailReason GetOffsetNum(ResourceViewPointer heap_pointer, CD3DX12_GPU_DESCRIPTOR_HANDLE &descriptor_table)
	{
		PancystarEngine::EngineFailReason check_error;
		auto heap_data = resource_heap_list.find(heap_pointer.resource_view_pack_id.descriptor_heap_type_id);
		if (heap_data == resource_heap_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the descriptor heap ID: " + std::to_string(heap_pointer.resource_view_pack_id.descriptor_heap_type_id));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get descriptor heap offset", error_message);
			return error_message;
		}
		check_error = heap_data->second->GetOffsetNum(heap_pointer.resource_view_pack_id.descriptor_heap_offset, heap_pointer.resource_view_offset_id, descriptor_table);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
	inline pancy_object_id GetOffsetNum(ResourceViewPointer heap_pointer, CD3DX12_CPU_DESCRIPTOR_HANDLE &descriptor_table)
	{
		auto heap_data = resource_heap_list.find(heap_pointer.resource_view_pack_id.descriptor_heap_type_id);
		if (heap_data == resource_heap_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the descriptor heap ID: " + std::to_string(heap_pointer.resource_view_pack_id.descriptor_heap_type_id));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get descriptor heap offset", error_message);
			return NULL;
		}
		return heap_data->second->GetOffsetNum(heap_pointer.resource_view_pack_id.descriptor_heap_offset, heap_pointer.resource_view_offset_id, descriptor_table);
	}
	//创建资源视图
	PancystarEngine::EngineFailReason BuildSRV(
		const ResourceViewPointer &RSV_point,
		const SubMemoryPointer &resource_in,
		const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
	);
	PancystarEngine::EngineFailReason BuildCBV(
		const ResourceViewPointer &RSV_point,
		const SubMemoryPointer &resource_in
	);
	PancystarEngine::EngineFailReason BuildUAV(
		const ResourceViewPointer &RSV_point,
		const SubMemoryPointer &resource_in,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc
	);
	PancystarEngine::EngineFailReason BuildRTV(
		const ResourceViewPointer &RSV_point,
		const SubMemoryPointer &resource_in,
		const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc
	);
	PancystarEngine::EngineFailReason BuildDSV(
		const ResourceViewPointer &RSV_point,
		const SubMemoryPointer &resource_in,
		const D3D12_DEPTH_STENCIL_VIEW_DESC    &DSV_desc
	);
	~PancyDescriptorHeapControl();
private:
PancystarEngine::EngineFailReason BuildDescriptorHeap(
		const std::string &descriptor_heap_name_in,
		const pancy_object_id &heap_block_size_in,
		const D3D12_DESCRIPTOR_HEAP_DESC &heap_desc_in,
		ResourceViewPack &RSV_pack_id
	);
};

