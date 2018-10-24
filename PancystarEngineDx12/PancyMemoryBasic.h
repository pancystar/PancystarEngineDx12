#pragma once
#include"PancyDx12Basic.h"
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
	uint64_t memory_size;//存储块的大小
	ComPtr<ID3D12Resource> resource_data;//存储块的数据
public:
	MemoryBlockGpu(
		const uint64_t &memory_size_in,
		ComPtr<ID3D12Resource> resource_data_in
	);
	inline ComPtr<ID3D12Resource> GetResource() 
	{
		return resource_data;
	}
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
	PancystarEngine::EngineFailReason BuildMemoryResource(
		const CD3DX12_RESOURCE_DESC &resource_desc,
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
	PancystarEngine::EngineFailReason BuildMemoryResource(
		const CD3DX12_RESOURCE_DESC &resource_desc,
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
		const CD3DX12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		VirtualMemoryPointer &virtual_pointer
	);
	MemoryBlockGpu *GetMemoryResource(const VirtualMemoryPointer &virtual_pointer);
	PancystarEngine::EngineFailReason FreeResource(const VirtualMemoryPointer &virtual_pointer);
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
		const pancy_resource_id &commit_block_num,
		const uint64_t &per_block_size,
		const D3D12_HEAP_TYPE &heap_type_in,
		const D3D12_HEAP_FLAGS &heap_flag_in,
		pancy_resource_id &resource_id,
		uint64_t heap_alignment_size = 0
	);
};
//资源描述视图
class PancyResourceView
{
	ComPtr<ID3D12DescriptorHeap> heap_data;
	D3D12_DESCRIPTOR_HEAP_TYPE resource_type;
	//资源视图的位置
	int32_t heap_offset;
	//资源视图包含的资源
	int32_t resource_view_number;
	pancy_object_id resource_block_size;
public:
	PancyResourceView(
		ComPtr<ID3D12DescriptorHeap> heap_data_in,
		const int32_t &heap_offset_in,
		D3D12_DESCRIPTOR_HEAP_TYPE &resource_type_in,
		const int32_t &view_number_in
	);
private:
	PancystarEngine::EngineFailReason BuildSRV(const pancy_object_id &self_offset, const VirtualMemoryPointer &resource_in, const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc);
	PancystarEngine::EngineFailReason BuildCBV(const pancy_object_id &self_offset, const D3D12_CONSTANT_BUFFER_VIEW_DESC  &CBV_desc);
	PancystarEngine::EngineFailReason BuildUAV(const pancy_object_id &self_offset, const VirtualMemoryPointer &resource_in, const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc);
	PancystarEngine::EngineFailReason BuildRTV(const pancy_object_id &self_offset, const VirtualMemoryPointer &resource_in, const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc);
};
PancyResourceView::PancyResourceView(
	ComPtr<ID3D12DescriptorHeap> heap_data_in,
	const int32_t &heap_offset_in,
	D3D12_DESCRIPTOR_HEAP_TYPE &resource_type_in,
	const int32_t &view_number_in
) 
{
	heap_data = heap_data_in;
	heap_offset = heap_offset_in;
	resource_view_number = view_number_in;
	resource_type = resource_type_in;
	resource_block_size =  PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetDescriptorHandleIncrementSize(resource_type);
}
PancystarEngine::EngineFailReason PancyResourceView::BuildSRV(const pancy_object_id &self_offset, const VirtualMemoryPointer &resource_in, const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc)
{
	//检验偏移是否合法
	if (self_offset >= resource_view_number)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL,"the srv id: "+std::to_string(self_offset) +" is bigger than the max id of descriptor heap block");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add SRV to descriptor heap block",error_message);
		return error_message;
	}
	//检验资源是否存在
	auto resource_data = MemoryHeapGpuControl::GetInstance()->GetMemoryResource(resource_in);
	if (resource_data == NULL) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource from pointer: " + std::to_string(resource_in.heap_type) + "::" + std::to_string(resource_in.heap_list_id) +"::" + std::to_string(resource_in.memory_block_id)+"//"+std::to_string(resource_in.memory_resource_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add SRV to descriptor heap block", error_message);
		return error_message;
	}
	//创建描述符
	int32_t heap_start_pos = heap_offset * resource_view_number * static_cast<int32_t>(resource_block_size) + static_cast<int32_t>(self_offset) * static_cast<int32_t>(resource_block_size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap_data->GetCPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(heap_start_pos);
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateShaderResourceView(resource_data->GetResource().Get(), &SRV_desc, cpuHandle);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyResourceView::BuildCBV(const pancy_object_id &self_offset, const D3D12_CONSTANT_BUFFER_VIEW_DESC  &CBV_desc)
{
	//检验偏移是否合法
	if (self_offset >= resource_view_number)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the CBV id: " + std::to_string(self_offset) + " is bigger than the max id of descriptor heap block");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add CBV to descriptor heap block", error_message);
		return error_message;
	}
	//创建描述符
	int32_t heap_start_pos = heap_offset * resource_view_number * static_cast<int32_t>(resource_block_size) + static_cast<int32_t>(self_offset) * static_cast<int32_t>(resource_block_size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap_data->GetCPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(heap_start_pos);
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateConstantBufferView(&CBV_desc, cpuHandle);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyResourceView::BuildUAV(const pancy_object_id &self_offset, const VirtualMemoryPointer &resource_in, const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc)
{
	//检验偏移是否合法
	if (self_offset >= resource_view_number)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the UAV id: " + std::to_string(self_offset) + " is bigger than the max id of descriptor heap block");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add UAV to descriptor heap block", error_message);
		return error_message;
	}
	//检验资源是否存在
	auto resource_data = MemoryHeapGpuControl::GetInstance()->GetMemoryResource(resource_in);
	if (resource_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource from pointer: " + std::to_string(resource_in.heap_type) + "::" + std::to_string(resource_in.heap_list_id) + "::" + std::to_string(resource_in.memory_block_id) + "//" + std::to_string(resource_in.memory_resource_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add UAV to descriptor heap block", error_message);
		return error_message;
	}
	//创建描述符
	/*
	int32_t heap_start_pos = heap_offset * resource_view_number * static_cast<int32_t>(resource_block_size) + static_cast<int32_t>(self_offset) * static_cast<int32_t>(resource_block_size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap_data->GetCPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(heap_start_pos);
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateUnorderedAccessView(resource_data->GetResource().Get(), &UAV_desc, cpuHandle);
	*/
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyResourceView::BuildRTV(const pancy_object_id &self_offset, const VirtualMemoryPointer &resource_in, const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc)
{
	//检验偏移是否合法
	if (self_offset >= resource_view_number)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the RTV id: " + std::to_string(self_offset) + " is bigger than the max id of descriptor heap block");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add RTV to descriptor heap block", error_message);
		return error_message;
	}
	//检验资源是否存在
	auto resource_data = MemoryHeapGpuControl::GetInstance()->GetMemoryResource(resource_in);
	if (resource_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource from pointer: " + std::to_string(resource_in.heap_type) + "::" + std::to_string(resource_in.heap_list_id) + "::" + std::to_string(resource_in.memory_block_id) + "//" + std::to_string(resource_in.memory_resource_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add RTV to descriptor heap block", error_message);
		return error_message;
	}
	//创建描述符
	int32_t heap_start_pos = heap_offset * resource_view_number * static_cast<int32_t>(resource_block_size) + static_cast<int32_t>(self_offset) * static_cast<int32_t>(resource_block_size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap_data->GetCPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(heap_start_pos);
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateRenderTargetView(resource_data->GetResource().Get(), &RTV_desc, cpuHandle);
	return PancystarEngine::succeed;
}
//资源描述符管理
class PancyDescriptorHeap 
{
	std::string descriptor_heap_name;
	pancy_object_id heap_block_size;    //描述符每个存储块的大小(拥有的视图数量)
	pancy_object_id heap_block_num;     //描述符的存储块数量
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
	ComPtr<ID3D12DescriptorHeap> heap_data;
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
	PancyResourceView* GetHeapBlock(const pancy_resource_id &resource_view_ID,PancystarEngine::EngineFailReason &check_error);
	//创建资源视图
	PancystarEngine::EngineFailReason BuildSRV(const pancy_object_id &descriptor_block_id, const pancy_object_id &self_offset, const VirtualMemoryPointer &resource_in, const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc);
	PancystarEngine::EngineFailReason BuildCBV(const pancy_object_id &descriptor_block_id, const pancy_object_id &self_offset, const D3D12_CONSTANT_BUFFER_VIEW_DESC  &CBV_desc);
	PancystarEngine::EngineFailReason BuildUAV(const pancy_object_id &descriptor_block_id, const pancy_object_id &self_offset, const VirtualMemoryPointer &resource_in, const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc);
	PancystarEngine::EngineFailReason BuildRTV(const pancy_object_id &descriptor_block_id, const pancy_object_id &self_offset, const VirtualMemoryPointer &resource_in, const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc);
};
PancyDescriptorHeap::PancyDescriptorHeap(
	const std::string &descriptor_heap_name_in,
	const pancy_object_id &heap_block_size_in,
	const D3D12_DESCRIPTOR_HEAP_DESC &heap_desc_in
)
{
	descriptor_heap_name = descriptor_heap_name_in;
	heap_block_size = heap_block_size_in;
	heap_block_num = heap_desc_in.NumDescriptors / heap_block_size_in;
	heap_desc = heap_desc_in;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::Create() 
{
	//创建描述符堆
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap_data));
	if (FAILED(hr)) 
	{
		PancystarEngine::EngineFailReason error_message(hr,"create descriptor heap error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap", error_message);
		return error_message;
	}
	//将描述符堆内的数据全部赋为空闲状态
	for (pancy_object_id i = 0; i < heap_block_num; ++i) 
	{
		empty_view_block.insert(i);
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildHeapBlock(pancy_resource_id &resource_view_ID)
{
	if (resource_view_ID > heap_block_num) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor heap resource id: " + std::to_string(resource_view_ID) + " is bigger than the descriptor heap" + descriptor_heap_name + " size: "+std::to_string(heap_block_num));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build resource view from desciptor heap", error_message);
		return error_message;
	}
	if (resource_view_heap_block.find(resource_view_ID) != resource_view_heap_block.end()) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor heap resource id: " + std::to_string(resource_view_ID) + " is already build do not rebuild resource in heap: " + descriptor_heap_name,PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build resource view from desciptor heap", error_message);
		return error_message;
	}
	if (empty_view_block.size() == 0) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL,"the descriptor heap"+ descriptor_heap_name + " do not have enough space to build a new resource view");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build resource view from desciptor heap", error_message);
		return error_message;
	}
	pancy_object_id empty_id = *empty_view_block.begin();
	PancyResourceView *new_data = new PancyResourceView(heap_data, empty_id, heap_desc.Type, heap_block_size);
	resource_view_heap_block.insert(std::pair<pancy_object_id, PancyResourceView*>(empty_id, new_data));
	empty_view_block.erase(empty_id);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::FreeHeapBlock(const pancy_resource_id &resource_view_ID)
{
	auto now_resource_remove = resource_view_heap_block.find(resource_view_ID);
	if (now_resource_remove == resource_view_heap_block.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find heap resource id: " + std::to_string(resource_view_ID) + " in descriptor heap: " + descriptor_heap_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("remove resource view from desciptor heap", error_message);
		return error_message;
	}
	delete now_resource_remove->second;
	empty_view_block.insert(now_resource_remove->first);
	resource_view_heap_block.erase(now_resource_remove);
	return PancystarEngine::succeed;
}
PancyResourceView* PancyDescriptorHeap::GetHeapBlock(const pancy_resource_id &resource_view_ID, PancystarEngine::EngineFailReason &check_error)
{
	auto now_resource_remove = resource_view_heap_block.find(resource_view_ID);
	if (now_resource_remove == resource_view_heap_block.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find heap resource id: " + std::to_string(resource_view_ID) + " in descriptor heap: " + descriptor_heap_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("remove resource view from desciptor heap", error_message);
		check_error = error_message;
		return NULL;
	}
	check_error = PancystarEngine::succeed;
	return now_resource_remove->second;
}
PancystarEngine::EngineFailReason BuildSRV(const pancy_object_id &descriptor_block_id, const pancy_object_id &self_offset, const VirtualMemoryPointer &resource_in, const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc) 
{
	PancyResourceView *new_descriptor_heap
}
PancyDescriptorHeap::~PancyDescriptorHeap()
{
	for (auto free_data = resource_view_heap_block.begin(); free_data != resource_view_heap_block.end(); ++free_data) 
	{
		delete free_data->second;
	}
	resource_view_heap_block.clear();
}
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
	PancystarEngine::EngineFailReason BuildDescriptorHeap(
		const std::string &descriptor_heap_name_in,
		const pancy_object_id &heap_block_size_in,
		const D3D12_DESCRIPTOR_HEAP_DESC &heap_desc_in,
		pancy_resource_id &descriptor_heap_id
	);
	PancystarEngine::EngineFailReason FreeDescriptorHeap(
		pancy_resource_id &descriptor_heap_id
	);
	~PancyDescriptorHeapControl();
};
PancyDescriptorHeapControl::PancyDescriptorHeapControl() 
{
	descriptor_heap_id_selfadd = 0;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildDescriptorHeap(
	const std::string &descriptor_heap_name_in,
	const pancy_object_id &heap_block_size_in,
	const D3D12_DESCRIPTOR_HEAP_DESC &heap_desc_in,
	pancy_resource_id &descriptor_heap_id
) 
{
	if (resource_init_list.find(descriptor_heap_name_in) != resource_init_list.end()) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor heap: "+ descriptor_heap_name_in +" already been build before",PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap", error_message);
		return error_message;
	}
	if (resource_memory_free_id.size() != 0) 
	{
		descriptor_heap_id = *resource_memory_free_id.begin();
		resource_memory_free_id.erase(descriptor_heap_id);
	}
	else 
	{
		descriptor_heap_id = descriptor_heap_id_selfadd;
		descriptor_heap_id_selfadd += 1;
	}
	PancyDescriptorHeap *new_heap = new PancyDescriptorHeap(descriptor_heap_name_in, heap_block_size_in, heap_desc_in);
	PancystarEngine::EngineFailReason check_error = new_heap->Create();
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	resource_init_list.insert(std::pair<std::string, pancy_resource_id>(descriptor_heap_name_in, descriptor_heap_id));
	resource_heap_list.insert(std::pair<pancy_resource_id, PancyDescriptorHeap*>(descriptor_heap_id, new_heap));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::FreeDescriptorHeap(pancy_resource_id &descriptor_heap_id)
{
	if (resource_heap_list.find(descriptor_heap_id) == resource_heap_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the descriptor heap id: " + std::to_string(descriptor_heap_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free descriptor heap", error_message);
		return error_message;
	}
	auto now_free_data = resource_heap_list.find(descriptor_heap_id);
	//删除资源的名称存档
	resource_init_list.erase(now_free_data->second->GetDescriptorName());
	//释放资源的id到空闲队列
	resource_memory_free_id.insert(descriptor_heap_id);
	//删除资源
	delete now_free_data->second;
	//删除资源的记录
	resource_heap_list.erase(now_free_data);
	return PancystarEngine::succeed;
}
PancyDescriptorHeapControl::~PancyDescriptorHeapControl()
{
	std::vector<pancy_resource_id> free_id_list;
	for (auto data_free = resource_heap_list.begin(); data_free != resource_heap_list.end(); ++data_free) 
	{
		free_id_list.push_back(data_free->first);
	}
	for (int i = 0; i < free_id_list.size(); ++i) 
	{
		FreeDescriptorHeap(free_id_list[i]);
	}
	resource_memory_free_id.clear();
}