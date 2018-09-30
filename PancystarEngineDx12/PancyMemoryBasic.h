#pragma once
#include"PancyDx12Basic.h"
//显存块
class MemoryBlockGpu
{
	pancy_resource_id memory_heap_list_id;//显存堆类型地址
	pancy_resource_id memory_heap_id;     //显存堆段地址
	pancy_resource_id memory_block_id;    //显存块地址
	uint64_t memory_size;//存储块的大小
	ComPtr<ID3D12Resource> resource_data_dx12;//存储块的数据
	uint64_t now_memory_offset_point;//当前已经开辟的内存指针位置
public:
	MemoryBlockGpu(
		const uint64_t &memory_size_in,
		ComPtr<ID3D12Resource> resource_data_in
	);
};
//保留显存堆
class MemoryHeapGpu
{
	std::string heap_type_name;
	uint64_t size_per_block;
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
	inline uint64_t GetMemorySizePerBlock()
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
	PancystarEngine::EngineFailReason GetMemoryResource(
		const CD3DX12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		pancy_resource_id &memory_block_ID
	);
	//检验对应id的资源是否已经被分配
	bool CheckIfFree(pancy_resource_id memory_block_ID);
	//释放一个对应id的资源
	PancystarEngine::EngineFailReason FreeMemoryReference(const pancy_resource_id &memory_block_ID);
};
//线性增长的显存堆
class MemoryHeapLinear
{
	//显存堆的格式
	CD3DX12_HEAP_DESC heap_desc;
	int32_t size_per_block;
	pancy_resource_id max_block_num;
	//显存堆的名称
	std::string heap_type_name;

	//所有显存堆的数据
	std::unordered_map<pancy_resource_id, MemoryHeapGpu*> memory_heap_data;
	//空出的显存堆
	std::unordered_set<pancy_resource_id> empty_memory_heap;
public:
	MemoryHeapLinear(const std::string &heap_type_name_in, const CD3DX12_HEAP_DESC &heap_desc_in, const uint64_t &size_per_block_in, const pancy_resource_id &max_block_num_in);
	//从显存堆开辟资源
	PancystarEngine::EngineFailReason GetMemoryResource(
		const CD3DX12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		Microsoft::WRL::Details::ComPtrRef<ComPtr<ID3D12Resource>> ppvResourc,
		pancy_resource_id &memory_block_ID,//显存块地址指针
		pancy_resource_id &memory_heap_ID//显存段地址指针
	);
	//释放一个对应id的资源
	PancystarEngine::EngineFailReason FreeMemoryReference(
		const pancy_resource_id &memory_heap_ID,
		const pancy_resource_id &memory_block_ID
	);
	~MemoryHeapLinear();
};
//资源堆管理器
class MemoryHeapGpuControl
{
	std::unordered_map<std::string, pancy_resource_id> resource_init_list;
	std::unordered_map<pancy_resource_id, MemoryHeapLinear*> resource_heap_list;//虚拟显存资源表
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
	
	PancystarEngine::EngineFailReason BuildResource(
		const std::string &HeapFileName,
		const CD3DX12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		Microsoft::WRL::Details::ComPtrRef<ComPtr<ID3D12Resource>> ppvResourc,
		pancy_resource_id &memory_block_ID,//显存块地址指针
		pancy_resource_id &memory_heap_ID//显存段地址指针
	);
	PancystarEngine::EngineFailReason LoadHeapFromFile(
		const std::string &HeapFileName,
		pancy_resource_id &resource_id,
		uint64_t heap_alignment_size = 0
	);
	PancystarEngine::EngineFailReason FreeResource(
		const pancy_resource_id &memory_heap_list_ID,//显存域地址指针
		const pancy_resource_id &memory_heap_ID,//显存段地址指针
		const pancy_resource_id &memory_block_ID//显存块地址指针
	);
	~MemoryHeapGpuControl();
private:
	PancystarEngine::EngineFailReason BuildHeap(
		const std::string &HeapFileName,
		const pancy_resource_id &commit_block_num,
		const uint64_t &per_block_size,
		const D3D12_HEAP_TYPE &heap_type_in,
		const D3D12_HEAP_FLAGS &heap_flag_in,
		pancy_resource_id &resource_id,
		uint64_t heap_alignment_size = 0
	);
};
//资源管理器


