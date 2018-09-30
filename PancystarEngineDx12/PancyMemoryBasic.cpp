#include"PancyMemoryBasic.h"
//GPU资源块
MemoryBlockGpu::MemoryBlockGpu(
	const uint64_t &memory_size_in,
	ComPtr<ID3D12Resource> resource_data_in
)
{
	memory_size = memory_size_in;
	resource_data_dx12 = resource_data_in;
	now_memory_offset_point = 0;
}
//GPU资源堆
MemoryHeapGpu::MemoryHeapGpu(const std::string &heap_type_name_in)
{
	heap_type_name = heap_type_name_in;
	size_per_block = 0;
	max_block_num = 0;
}
PancystarEngine::EngineFailReason MemoryHeapGpu::Create(const CD3DX12_HEAP_DESC &heap_desc_in, const uint64_t &size_per_block_in, const pancy_resource_id &max_block_num_in)
{
	size_per_block = size_per_block_in;
	max_block_num = max_block_num_in;
	//检查堆缓存的大小
	if (heap_desc_in.SizeInBytes != size_per_block * max_block_num)
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL, "Memory Heap Size In" + heap_type_name + " need " + std::to_string(size_per_block * max_block_num) + " But Find " + std::to_string(heap_desc_in.SizeInBytes));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Memorty Heap", check_error);
		return check_error;
	}
	//创建资源堆
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateHeap(&heap_desc_in, IID_PPV_ARGS(&heap_data));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason check_error(hr, "Create Memory Heap " + heap_type_name + "error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Memorty Heap", check_error);
		return check_error;
	}
	//初始化堆内空闲的资源块
	for (pancy_resource_id i = 0; i < max_block_num; ++i)
	{
		empty_memory_block.insert(i);
	}
	return PancystarEngine::succeed;
}
bool MemoryHeapGpu::CheckIfFree(pancy_resource_id memory_block_ID)
{
	if (memory_block_ID >= max_block_num)
	{
		return false;
	}
	auto check_data = free_list.find(memory_block_ID);
	if (check_data != free_list.end())
	{
		return true;
	}
	return false;
}
PancystarEngine::EngineFailReason MemoryHeapGpu::GetMemoryResource(
	const CD3DX12_RESOURCE_DESC &resource_desc,
	const D3D12_RESOURCE_STATES &resource_state,
	//Microsoft::WRL::Details::ComPtrRef<ComPtr<ID3D12Resource>> ppvResourc,
	pancy_resource_id &memory_block_ID
)
{
	ComPtr<ID3D12Resource> ppvResourc;
	//检查是否还有空余的存储空间
	auto rand_free_memory = empty_memory_block.begin();
	if (rand_free_memory == empty_memory_block.end())
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL, "The Heap " + heap_type_name + " Is empty, can't alloc new memory");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Allocated Memorty From Heap", check_error);
		return check_error;
	}
	//在显存堆上创建显存资源
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreatePlacedResource(
		heap_data.Get(),
		(*rand_free_memory) * size_per_block,
		&resource_desc,
		resource_state,
		nullptr,
		IID_PPV_ARGS(&ppvResourc)
	);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason check_error(hr, "Allocate Memory From Heap " + heap_type_name + "error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Allocated Memorty From Heap", check_error);
		return check_error;
	}
	memory_block_ID = *rand_free_memory;
	empty_memory_block.erase(rand_free_memory);
	MemoryBlockGpu *new_memory_block_data = new MemoryBlockGpu(size_per_block, ppvResourc);
	memory_heap_block.insert(std::pair<pancy_resource_id, MemoryBlockGpu*>(memory_block_ID, new_memory_block_data));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason MemoryHeapGpu::FreeMemoryReference(const pancy_resource_id &memory_block_ID)
{
	if (memory_block_ID >= max_block_num)
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL, "The heap " + heap_type_name + " Only have " + std::to_string(max_block_num) + " Memory block,ID " + std::to_string(memory_block_ID) + " Out of range");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free Memorty From Heap", check_error);
		return check_error;
	}
	auto check_data = empty_memory_block.find(memory_block_ID);
	if (check_data != empty_memory_block.end())
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL, "The memory block ID " + std::to_string(memory_block_ID) + " Haven't been allocated");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free Memorty From Heap", check_error);
		return check_error;
	}
	auto memory_data = memory_heap_block.find(memory_block_ID);
	if (memory_data == memory_heap_block.end()) 
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL, "The memory block ID " + std::to_string(memory_block_ID) + " Haven't been allocated or illegal memory id");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free Memorty From Heap", check_error);
		return check_error;
	}
	empty_memory_block.insert(memory_block_ID);
	delete memory_data->second;
	memory_heap_block.erase(memory_data);
	return PancystarEngine::succeed;
}
//GPU线性增长的资源堆
MemoryHeapLinear::MemoryHeapLinear(const std::string &heap_type_name_in, const CD3DX12_HEAP_DESC &heap_desc_in, const uint64_t &size_per_block_in, const pancy_resource_id &max_block_num_in)
{
	heap_desc = heap_desc_in;
	size_per_block = size_per_block_in;
	max_block_num = max_block_num_in;
	heap_type_name = heap_type_name_in;
}
PancystarEngine::EngineFailReason MemoryHeapLinear::GetMemoryResource(
	const CD3DX12_RESOURCE_DESC &resource_desc,
	const D3D12_RESOURCE_STATES &resource_state,
	Microsoft::WRL::Details::ComPtrRef<ComPtr<ID3D12Resource>> ppvResourc,
	pancy_resource_id &memory_block_ID,//显存块地址指针
	pancy_resource_id &memory_heap_ID//显存段地址指针
)
{
	if (empty_memory_heap.size() == 0)
	{
		pancy_resource_id new_id = memory_heap_data.size();
		if (new_id + static_cast<pancy_resource_id>(1) < new_id)
		{
			//内存已经满了
			PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource heap: " + heap_type_name + " was full so could not get empty memory");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build resource heap list", error_message);
			return error_message;
		}
		//创建一个空的存储段
		MemoryHeapGpu *new_heap = new MemoryHeapGpu(heap_type_name);
		auto check_error = new_heap->Create(heap_desc, size_per_block, max_block_num);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		memory_heap_data.insert(std::pair<pancy_resource_id, MemoryHeapGpu*>(new_id + static_cast<pancy_resource_id>(1), new_heap));
		empty_memory_heap.insert(new_id + static_cast<pancy_resource_id>(1));
	}
	//挑选一个空的存储段
	pancy_resource_id new_empty_id = *empty_memory_heap.begin();
	auto new_empty_heap = memory_heap_data.find(new_empty_id);
	//开辟存储空间
	auto check_error = new_empty_heap->second->GetMemoryResource(resource_desc, resource_state, ppvResourc, memory_block_ID);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	memory_heap_ID = new_empty_id;
	//如果开辟空间后，该堆的存储空间满，将其从待分配队列移除
	if (new_empty_heap->second->GetFreeMemoryBlockNum() == 0)
	{
		empty_memory_heap.erase(new_empty_id);
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason MemoryHeapLinear::FreeMemoryReference(
	const pancy_resource_id &memory_heap_ID,
	const pancy_resource_id &memory_block_ID
)
{
	//根据段指针找到显存段
	auto memory_heap_now = memory_heap_data.find(memory_heap_ID);
	if (memory_heap_now == memory_heap_data.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource heap: " + std::to_string(memory_heap_ID) + " could not be find");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free resource heap data", error_message);
		return error_message;
	}
	//根据块指针释放显存块
	memory_heap_now->second->FreeMemoryReference(memory_block_ID);
	//如果显存资源堆之前已满，释放资源后重新放入待分配池
	if (empty_memory_heap.find(memory_heap_ID) == empty_memory_heap.end())
	{
		empty_memory_heap.insert(memory_heap_ID);
	}
	return PancystarEngine::succeed;
}
MemoryHeapLinear::~MemoryHeapLinear()
{
	for (auto data_heap = memory_heap_data.begin(); data_heap != memory_heap_data.end(); ++data_heap)
	{
		delete data_heap->second;
	}
	memory_heap_data.clear();
	empty_memory_heap.clear();
}
//GPU资源堆管理器
MemoryHeapGpuControl::MemoryHeapGpuControl()
{
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_TYPE_DEFAULT", static_cast<int32_t>(D3D12_HEAP_TYPE_DEFAULT));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_TYPE_UPLOAD", static_cast<int32_t>(D3D12_HEAP_TYPE_UPLOAD));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_TYPE_READBACK", static_cast<int32_t>(D3D12_HEAP_TYPE_READBACK));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_TYPE_CUSTOM", static_cast<int32_t>(D3D12_HEAP_TYPE_CUSTOM));

	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_NONE", static_cast<int32_t>(D3D12_HEAP_FLAG_NONE));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_SHARED", static_cast<int32_t>(D3D12_HEAP_FLAG_SHARED));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_DENY_BUFFERS", static_cast<int32_t>(D3D12_HEAP_FLAG_DENY_BUFFERS));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_DISPLAY", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_DISPLAY));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER", static_cast<int32_t>(D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES", static_cast<int32_t>(D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES", static_cast<int32_t>(D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_HARDWARE_PROTECTED", static_cast<int32_t>(D3D12_HEAP_FLAG_HARDWARE_PROTECTED));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_WRITE_WATCH", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_WRITE_WATCH));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES));
	JsonLoader::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES));
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::LoadHeapFromFile(
	const std::string &HeapFileName,
	pancy_resource_id &resource_id,
	uint64_t heap_alignment_size
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_resource_id commit_block_num;
	uint64_t per_block_size;
	D3D12_HEAP_TYPE heap_type_in;
	D3D12_HEAP_FLAGS heap_flag_in;
	Json::Value root_value;
	pancy_json_value rec_value;
	check_error = JsonLoader::GetInstance()->LoadJsonFile(HeapFileName, root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = JsonLoader::GetInstance()->GetJsonData(HeapFileName, root_value, "commit_block_num", pancy_json_data_type::json_data_int, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	commit_block_num = static_cast<pancy_resource_id>(rec_value.int_value);
	check_error = JsonLoader::GetInstance()->GetJsonData(HeapFileName, root_value, "per_block_size", pancy_json_data_type::json_data_int, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	per_block_size = static_cast<uint64_t>(rec_value.int_value);
	check_error = JsonLoader::GetInstance()->GetJsonData(HeapFileName, root_value, "heap_type_in", pancy_json_data_type::json_data_enum, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	Json::Value value_heap_flags = root_value.get("heap_flag_in", Json::Value::null);
	int32_t rec_data = 0;
	for (int32_t i = 0; i < value_heap_flags.size(); ++i)
	{
		JsonLoader::GetInstance()->GetJsonData(HeapFileName, root_value, i, pancy_json_data_type::json_data_enum, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		rec_data = rec_data | rec_value.int_value;
	}
	heap_flag_in = static_cast<D3D12_HEAP_FLAGS>(rec_data);
	check_error = BuildHeap(HeapFileName, commit_block_num, per_block_size, heap_type_in, heap_flag_in, resource_id, heap_alignment_size);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::BuildHeap(
	const std::string &heap_desc_name,
	const pancy_resource_id &commit_block_num,
	const uint64_t &per_block_size,
	const D3D12_HEAP_TYPE &heap_type_in,
	const D3D12_HEAP_FLAGS &heap_flag_in,
	pancy_resource_id &resource_id,
	uint64_t heap_alignment_size
)
{
	auto check_resource_id = resource_init_list.find(heap_desc_name);
	if (check_resource_id != resource_init_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource: " + check_resource_id->first + "jave been build", PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build resource heap from file", error_message);
		return error_message;
	}
	CD3DX12_HEAP_DESC heapDesc(commit_block_num * per_block_size, heap_type_in, heap_alignment_size, heap_flag_in);
	MemoryHeapLinear *new_heap = new MemoryHeapLinear(heap_desc_name, heapDesc, per_block_size, commit_block_num);
	resource_id = resource_init_list.size();
	resource_init_list.insert(std::pair<std::string, pancy_resource_id>(heap_desc_name, resource_id));
	resource_heap_list.insert(std::pair<pancy_resource_id, MemoryHeapLinear*>(resource_id, new_heap));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::BuildResource(
	const std::string &HeapFileName,
	const CD3DX12_RESOURCE_DESC &resource_desc,
	const D3D12_RESOURCE_STATES &resource_state,
	Microsoft::WRL::Details::ComPtrRef<ComPtr<ID3D12Resource>> ppvResourc,
	pancy_resource_id &memory_block_ID,//显存块地址指针
	pancy_resource_id &memory_heap_ID//显存段地址指针
)
{
	pancy_resource_id resource_heap_id;
	auto heap_list_id = resource_init_list.find(HeapFileName);
	if (heap_list_id == resource_init_list.end())
	{
		auto check_error = LoadHeapFromFile(HeapFileName, resource_heap_id);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	else 
	{
		resource_heap_id = heap_list_id->second;
	}
	auto heap_list_data = resource_heap_list.find(resource_heap_id);
	auto check_error = heap_list_data->second->GetMemoryResource(resource_desc, resource_state, ppvResourc, memory_block_ID, memory_heap_ID);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::FreeResource(
	const pancy_resource_id &memory_heap_list_ID,//显存域地址指针
	const pancy_resource_id &memory_heap_ID,//显存段地址指针
	const pancy_resource_id &memory_block_ID//显存块地址指针
)
{
	auto heap_list_data = resource_heap_list.find(memory_heap_list_ID);
	if (heap_list_data == resource_heap_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource heap list: " + std::to_string(memory_heap_list_ID) + " could not be find");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free resource heap data", error_message);
		return error_message;
	}
	auto check_error = heap_list_data->second->FreeMemoryReference(memory_heap_ID, memory_block_ID);
}
MemoryHeapGpuControl::~MemoryHeapGpuControl()
{
	for (auto data_heap = resource_heap_list.begin(); data_heap != resource_heap_list.end(); ++data_heap)
	{
		delete data_heap->second;
	}
	resource_heap_list.clear();
	resource_init_list.clear();
}