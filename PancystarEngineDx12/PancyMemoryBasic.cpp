#include"PancyMemoryBasic.h"
//GPU资源块
MemoryBlockGpu::MemoryBlockGpu(
	const uint64_t &memory_size_in,
	ComPtr<ID3D12Resource> resource_data_in,
	const D3D12_HEAP_TYPE &resource_usage_in,
	const D3D12_RESOURCE_STATES &resource_state
)
{
	memory_size = memory_size_in;
	resource_data = resource_data_in;
	resource_usage = resource_usage_in;
	if (resource_usage == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
	{
		CD3DX12_RANGE readRange(0, 0);
		HRESULT hr = resource_data->Map(0, &readRange, reinterpret_cast<void**>(&map_pointer));
		if (FAILED(hr)) 
		{
			PancystarEngine::EngineFailReason error_message(hr,"map dynamic buffer to cpu error");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build memory block gpu", error_message);
		}
	}
	else 
	{
		map_pointer = NULL;
	}
	now_subresource_state = resource_state;
}
PancystarEngine::EngineFailReason MemoryBlockGpu::WriteFromCpuToBuffer(const pancy_resource_size &pointer_offset, const void* copy_data, const pancy_resource_size data_size)
{
	if (resource_usage != D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource type is not upload, could not copy data to memory");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("copy CPU resource to memory block gpu", error_message);
		return error_message;
	}
	memcpy(map_pointer + pointer_offset, copy_data, data_size);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason MemoryBlockGpu::WriteFromCpuToBuffer(
	const pancy_resource_size &pointer_offset,
	std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
	UINT64* pRowSizesInBytes,
	UINT* pNumRows
)
{
	if (resource_usage != D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource type is not upload, could not copy data to memory");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("copy CPU resource to memory block gpu", error_message);
		return error_message;
	}
	//获取待拷贝指针
	UINT8* pData = map_pointer + pointer_offset;
	//获取subresource
	D3D12_SUBRESOURCE_DATA *pSrcData = &subresources[0];
	UINT subres_size = static_cast<UINT>(subresources.size());
	for (UINT i = 0; i < subres_size; ++i)
	{
		D3D12_MEMCPY_DEST DestData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, pLayouts[i].Footprint.RowPitch * pNumRows[i] };
		MemcpySubresource(&DestData, &pSrcData[i], (SIZE_T)pRowSizesInBytes[i], pNumRows[i], pLayouts[i].Footprint.Depth);
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason MemoryBlockGpu::ReadFromBufferToCpu(const pancy_resource_size &pointer_offset, void* copy_data, const pancy_resource_size data_size)
{
	if (resource_usage != D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource type is not readback, could not read data back");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("read memory block gpu to CPU", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
	//todo: 回读GPU数据
}
MemoryBlockGpu::~MemoryBlockGpu()
{
	if (resource_usage == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
	{
		resource_data->Unmap(0,NULL);
	}
}
//GPU资源堆
MemoryHeapGpu::MemoryHeapGpu(const std::string &heap_type_name_in)
{
	heap_type_name = heap_type_name_in;
	size_per_block = 0;
	max_block_num = 0;
}
PancystarEngine::EngineFailReason MemoryHeapGpu::Create(const CD3DX12_HEAP_DESC &heap_desc_in, const pancy_resource_size &size_per_block_in, const pancy_resource_id &max_block_num_in)
{
	size_per_block = size_per_block_in;
	max_block_num = max_block_num_in;
	/*
	//检查堆缓存的大小
	if (heap_desc_in.SizeInBytes != size_per_block * max_block_num)
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL, "Memory Heap Size In" + heap_type_name + " need " + std::to_string(size_per_block * max_block_num) + " But Find " + std::to_string(heap_desc_in.SizeInBytes));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Memorty Heap", check_error);
		return check_error;
	}
	*/
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
	auto check_data = empty_memory_block.find(memory_block_ID);
	if (check_data != empty_memory_block.end())
	{
		return true;
	}
	return false;
}
PancystarEngine::EngineFailReason MemoryHeapGpu::BuildMemoryResource(
	const D3D12_RESOURCE_DESC &resource_desc,
	const D3D12_RESOURCE_STATES &resource_state,
	//Microsoft::WRL::Details::ComPtrRef<ComPtr<ID3D12Resource>> ppvResourc,
	pancy_resource_id &memory_block_ID
)
{
	HRESULT hr;
	ComPtr<ID3D12Resource> ppvResourc;
	//检查是否还有空余的存储空间
	auto rand_free_memory = empty_memory_block.begin();
	if (rand_free_memory == empty_memory_block.end())
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL, "The Heap " + heap_type_name + " Is empty, can't alloc new memory");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Allocated Memorty From Heap", check_error);
		return check_error;
	}
	auto check_desc = heap_data->GetDesc();
	//在显存堆上创建显存资源
	auto heapdesc = heap_data->GetDesc();
	if (resource_desc.Format == DXGI_FORMAT_D24_UNORM_S8_UINT || resource_desc.Format == DXGI_FORMAT_D32_FLOAT)
	{
		D3D12_CLEAR_VALUE clearValue;
		clearValue.Format = resource_desc.Format;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;
		hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreatePlacedResource(
			heap_data.Get(),
			(*rand_free_memory) * size_per_block,
			&resource_desc,
			resource_state,
			&clearValue,
			IID_PPV_ARGS(&ppvResourc)
		);
	}
	else if(resource_desc.Format == DXGI_FORMAT_R8G8B8A8_UINT)
	{
		D3D12_CLEAR_VALUE clearValue;
		clearValue.Format = resource_desc.Format;
		clearValue.Color[0] = 255.0f;
		clearValue.Color[1] = 255.0f;
		clearValue.Color[2] = 255.0f;
		clearValue.Color[3] = 255.0f;
		hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreatePlacedResource(
			heap_data.Get(),
			(*rand_free_memory) * size_per_block,
			&resource_desc,
			resource_state,
			&clearValue,
			IID_PPV_ARGS(&ppvResourc)
		);
	}
	else 
	{
		D3D12_CLEAR_VALUE clearValue;
		clearValue.Format = resource_desc.Format;
		clearValue.Color[0] = 0.0f;
		clearValue.Color[0] = 0.0f;
		clearValue.Color[0] = 0.0f;
		clearValue.Color[0] = 0.0f;
		clearValue.DepthStencil.Stencil = 0;
		hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreatePlacedResource(
			heap_data.Get(),
			(*rand_free_memory) * size_per_block,
			&resource_desc,
			resource_state,
			nullptr,
			IID_PPV_ARGS(&ppvResourc)
		);
	}
	
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason check_error(hr, "Allocate Memory From Heap " + heap_type_name + "error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Allocated Memorty From Heap", check_error);
		return check_error;
	}
	memory_block_ID = *rand_free_memory;
	empty_memory_block.erase(rand_free_memory);
	MemoryBlockGpu *new_memory_block_data;
	new_memory_block_data = new MemoryBlockGpu(size_per_block, ppvResourc, check_desc.Properties.Type, resource_state);
	memory_heap_block.insert(std::pair<pancy_resource_id, MemoryBlockGpu*>(memory_block_ID, new_memory_block_data));
	return PancystarEngine::succeed;
}
MemoryBlockGpu* MemoryHeapGpu::GetMemoryResource(const pancy_resource_id &memory_block_ID) 
{
	auto check_data = memory_heap_block.find(memory_block_ID);
	if (check_data == memory_heap_block.end()) 
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL, "The memory block ID " + std::to_string(memory_block_ID) + " Haven't been allocated or illegal memory id");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free Memorty From Heap"+ heap_type_name, check_error);
		return NULL;
	}
	return check_data->second;
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
MemoryHeapGpu::~MemoryHeapGpu()
{
	for (auto data_heap = memory_heap_block.begin(); data_heap != memory_heap_block.end(); ++data_heap)
	{
		delete data_heap->second;
	}
	memory_heap_block.clear();
}
//GPU线性增长的资源堆
MemoryHeapLinear::MemoryHeapLinear(const std::string &heap_type_name_in, const CD3DX12_HEAP_DESC &heap_desc_in, const pancy_resource_size &size_per_block_in, const pancy_resource_id &max_block_num_in)
{
	heap_desc = heap_desc_in;
	size_per_block = size_per_block_in;
	max_block_num = max_block_num_in;
	heap_type_name = heap_type_name_in;
}
PancystarEngine::EngineFailReason MemoryHeapLinear::BuildMemoryResource(
	const D3D12_RESOURCE_DESC &resource_desc,
	const D3D12_RESOURCE_STATES &resource_state,
	pancy_resource_id &memory_block_ID,//显存块地址指针
	pancy_resource_id &memory_heap_ID//显存段地址指针
)
{
	if (empty_memory_heap.size() == 0)
	{
		pancy_resource_id new_id = static_cast<pancy_resource_id>(memory_heap_data.size());
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
	auto check_error = new_empty_heap->second->BuildMemoryResource(resource_desc, resource_state, memory_block_ID);
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
MemoryBlockGpu* MemoryHeapLinear::GetMemoryResource(
	const pancy_resource_id &memory_heap_ID,//显存段地址指针
	const pancy_resource_id &memory_block_ID//显存块地址指针
	
)
{
	//根据段指针找到显存段
	auto memory_heap_now = memory_heap_data.find(memory_heap_ID);
	if (memory_heap_now == memory_heap_data.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource heap: " + std::to_string(memory_heap_ID) + " could not be find");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free resource heap data", error_message);
		return NULL;
	}
	//根据块指针找到显存块
	return memory_heap_now->second->GetMemoryResource(memory_block_ID);
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
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_TYPE_DEFAULT", static_cast<int32_t>(D3D12_HEAP_TYPE_DEFAULT),typeid(D3D12_HEAP_TYPE_DEFAULT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_TYPE_UPLOAD", static_cast<int32_t>(D3D12_HEAP_TYPE_UPLOAD),typeid(D3D12_HEAP_TYPE_UPLOAD).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_TYPE_READBACK", static_cast<int32_t>(D3D12_HEAP_TYPE_READBACK),typeid(D3D12_HEAP_TYPE_READBACK).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_TYPE_CUSTOM", static_cast<int32_t>(D3D12_HEAP_TYPE_CUSTOM),typeid(D3D12_HEAP_TYPE_CUSTOM).name());

	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_NONE", static_cast<int32_t>(D3D12_HEAP_FLAG_NONE),typeid(D3D12_HEAP_FLAG_NONE).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_SHARED", static_cast<int32_t>(D3D12_HEAP_FLAG_SHARED),typeid(D3D12_HEAP_FLAG_SHARED).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_DENY_BUFFERS", static_cast<int32_t>(D3D12_HEAP_FLAG_DENY_BUFFERS),typeid(D3D12_HEAP_FLAG_DENY_BUFFERS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_DISPLAY", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_DISPLAY),typeid(D3D12_HEAP_FLAG_ALLOW_DISPLAY).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER", static_cast<int32_t>(D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER),typeid(D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES", static_cast<int32_t>(D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES),typeid(D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES", static_cast<int32_t>(D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES),typeid(D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_HARDWARE_PROTECTED", static_cast<int32_t>(D3D12_HEAP_FLAG_HARDWARE_PROTECTED),typeid(D3D12_HEAP_FLAG_HARDWARE_PROTECTED).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_WRITE_WATCH", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_WRITE_WATCH),typeid(D3D12_HEAP_FLAG_ALLOW_WRITE_WATCH).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS),typeid(D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES),typeid(D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS),typeid(D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES),typeid(D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES),typeid(D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES).name());
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::BuildResourceCommit(
	const D3D12_HEAP_TYPE &heap_type_in,
	const D3D12_HEAP_FLAGS &heap_flag_in,
	const CD3DX12_RESOURCE_DESC &resource_desc,
	const D3D12_RESOURCE_STATES &resource_state,
	VirtualMemoryPointer &virtual_pointer
)
{
	//创建资源
	ComPtr<ID3D12Resource> ppvResourc;
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(heap_type_in),
		heap_flag_in,
		&resource_desc,
		resource_state,
		nullptr,
		IID_PPV_ARGS(&ppvResourc));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason check_error(hr, "Build commit memory resource error ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Memorty From List", check_error);
		return check_error;
	}
	if (resource_memory_free_id.size() > 0)
	{
		virtual_pointer.memory_resource_id = *resource_memory_free_id.begin();
		resource_memory_free_id.erase(virtual_pointer.memory_resource_id);
	}
	else
	{
		if (resource_memory_list.size() + static_cast<pancy_resource_id>(1) > resource_memory_list.size())
		{
			virtual_pointer.memory_resource_id = static_cast<pancy_resource_id>(resource_memory_list.size());
		}
		else
		{
			PancystarEngine::EngineFailReason check_error(hr, "commit resource memory list if full,use big id to recombile project");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Memorty From List", check_error);
			return check_error;
		}
	}
	MemoryBlockGpu *new_block = new MemoryBlockGpu(virtual_pointer.memory_resource_id, ppvResourc, heap_type_in, resource_state);
	resource_memory_list.insert(std::pair<pancy_object_id, MemoryBlockGpu *>(virtual_pointer.memory_resource_id, new_block));
	return PancystarEngine::succeed;
}
MemoryBlockGpu* MemoryHeapGpuControl::GetMemoryResource(const VirtualMemoryPointer &virtual_pointer)
{
	if (virtual_pointer.if_heap) 
	{
		return GetMemoryResourceFromHeap(virtual_pointer.heap_type, virtual_pointer.heap_list_id, virtual_pointer.memory_block_id);
	}
	return GetMemoryFromList(virtual_pointer.memory_resource_id);
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::FreeResource(const VirtualMemoryPointer &virtual_pointer)
{
	if (virtual_pointer.if_heap)
	{
		return FreeResourceFromHeap(virtual_pointer.heap_type, virtual_pointer.heap_list_id, virtual_pointer.memory_block_id);
	}
	return FreeResourceCommit(virtual_pointer.memory_resource_id);
}
MemoryBlockGpu* MemoryHeapGpuControl::GetMemoryFromList(const pancy_object_id &memory_block_ID)
{
	auto check_data = resource_memory_list.find(memory_block_ID);
	if (check_data == resource_memory_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the commit resource id:" + std::to_string(memory_block_ID) + " could not be find");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get commit resource  data", error_message);
		return NULL;
	}
	return check_data->second;
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::FreeResourceCommit(const pancy_object_id &memory_block_ID)
{
	auto check_data = resource_memory_list.find(memory_block_ID);
	if (check_data == resource_memory_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the commit resource id:" + std::to_string(memory_block_ID) + " could not be find");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free commit resource  data", error_message);
		return error_message;
	}
	delete check_data->second;
	resource_memory_free_id.insert(check_data->first);
	resource_memory_list.erase(check_data);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::LoadHeapFromFile(
	const std::string &HeapFileName,
	pancy_resource_id &resource_id,
	uint64_t heap_alignment_size
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_resource_size heap_size;
	uint64_t per_block_size;
	D3D12_HEAP_TYPE heap_type_in;
	D3D12_HEAP_FLAGS heap_flag_in;
	Json::Value root_value;
	pancy_json_value rec_value;
	check_error = PancyJsonTool::GetInstance()->LoadJsonFile(HeapFileName, root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancyJsonTool::GetInstance()->GetJsonData(HeapFileName, root_value, "heap_size", pancy_json_data_type::json_data_int, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	heap_size = static_cast<pancy_resource_size>(rec_value.int_value);
	check_error = PancyJsonTool::GetInstance()->GetJsonData(HeapFileName, root_value, "per_block_size", pancy_json_data_type::json_data_int, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	per_block_size = static_cast<uint64_t>(rec_value.int_value);
	check_error = PancyJsonTool::GetInstance()->GetJsonData(HeapFileName, root_value, "heap_type_in", pancy_json_data_type::json_data_enum, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	heap_type_in = static_cast<D3D12_HEAP_TYPE>(rec_value.int_value);
	Json::Value value_heap_flags = root_value.get("heap_flag_in", Json::Value::null);
	int32_t rec_data = 0;
	for (uint32_t i = 0; i < value_heap_flags.size(); ++i)
	{
		PancyJsonTool::GetInstance()->GetJsonData(HeapFileName, value_heap_flags, i, pancy_json_data_type::json_data_enum, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		rec_data = rec_data | rec_value.int_value;
	}
	heap_flag_in = static_cast<D3D12_HEAP_FLAGS>(rec_data);
	check_error = BuildHeap(HeapFileName, heap_size, per_block_size, heap_type_in, heap_flag_in, resource_id, heap_alignment_size);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::BuildHeap(
	const std::string &heap_desc_name,
	const pancy_resource_size &heap_size,
	const pancy_resource_size &per_block_size,
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
	CD3DX12_HEAP_DESC heapDesc(heap_size, heap_type_in, heap_alignment_size, heap_flag_in);
	pancy_resource_id commit_block_num = static_cast<pancy_resource_id>(heap_size / per_block_size);
	MemoryHeapLinear *new_heap = new MemoryHeapLinear(heap_desc_name, heapDesc, per_block_size, commit_block_num);
	resource_id = static_cast<pancy_resource_id>(resource_init_list.size());
	resource_init_list.insert(std::pair<std::string, pancy_resource_id>(heap_desc_name, resource_id));
	resource_heap_list.insert(std::pair<pancy_resource_id, MemoryHeapLinear*>(resource_id, new_heap));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::BuildResourceFromHeap(
	const std::string &HeapFileName,
	const D3D12_RESOURCE_DESC &resource_desc,
	const D3D12_RESOURCE_STATES &resource_state,
	VirtualMemoryPointer &virtual_pointer
)
{
	virtual_pointer.if_heap = true;
	auto heap_list_id = resource_init_list.find(HeapFileName);
	if (heap_list_id == resource_init_list.end())
	{
		auto check_error = LoadHeapFromFile(HeapFileName, virtual_pointer.heap_type);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	else 
	{
		virtual_pointer.heap_type = heap_list_id->second;
	}
	auto heap_list_data = resource_heap_list.find(virtual_pointer.heap_type);
	auto check_error = heap_list_data->second->BuildMemoryResource(resource_desc, resource_state, virtual_pointer.memory_block_id, virtual_pointer.heap_list_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
MemoryBlockGpu* MemoryHeapGpuControl::GetMemoryResourceFromHeap(
	const pancy_resource_id &memory_heap_list_ID,//显存域地址指针
	const pancy_resource_id &memory_heap_ID,//显存段地址指针
	const pancy_resource_id &memory_block_ID//显存块地址指针
) 
{
	auto heap_list_data = resource_heap_list.find(memory_heap_list_ID);
	if (heap_list_data == resource_heap_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource heap list: " + std::to_string(memory_heap_list_ID) + " could not be find");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get resource heap data", error_message);
		return NULL;
	}
	return heap_list_data->second->GetMemoryResource(memory_heap_ID, memory_block_ID);
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::FreeResourceFromHeap(
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
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
void MemoryHeapGpuControl::GetHeapDesc(const pancy_resource_id &heap_id, pancy_object_id &heap_num, pancy_resource_size &per_heap_size)
{
	auto heap_data = resource_heap_list.find(heap_id);
	if (heap_data == resource_heap_list.end())
	{
		heap_num = 0;
		per_heap_size = 0;
		PancystarEngine::EngineFailReason error_message(E_FAIL,"could not find heap ID: " + std::to_string(heap_id),PancystarEngine::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Heap Desc", error_message);
	}
	heap_num = heap_data->second->GetHeapNum();
	per_heap_size = heap_data->second->GetPerHeapSize();
}
void MemoryHeapGpuControl::GetHeapDesc(const std::string &heap_name, pancy_object_id &heap_num, pancy_resource_size &per_heap_size)
{
	auto heap_id = resource_init_list.find(heap_name);
	if (heap_id == resource_init_list.end()) 
	{
		heap_num = 0;
		per_heap_size = 0;
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find heap name: " + heap_name, PancystarEngine::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Heap Desc", error_message);
	}
	GetHeapDesc(heap_id->second, heap_num, per_heap_size);
}
MemoryHeapGpuControl::~MemoryHeapGpuControl()
{
	for (auto data_heap = resource_heap_list.begin(); data_heap != resource_heap_list.end(); ++data_heap)
	{
		delete data_heap->second;
	}
	for (auto data_res = resource_memory_list.begin(); data_res != resource_memory_list.end(); ++data_res)
	{
		delete data_res->second;
	}
	resource_heap_list.clear();
	resource_init_list.clear();
	resource_memory_list.clear();
	resource_memory_free_id.clear();
}
//二级资源
SubMemoryData::SubMemoryData()
{
}

PancystarEngine::EngineFailReason SubMemoryData::Create(
	const std::string &buffer_desc_file,
	const D3D12_RESOURCE_DESC &resource_desc,
	const D3D12_RESOURCE_STATES &resource_state,
	const pancy_object_id &per_memory_size_in
)
{
	PancystarEngine::EngineFailReason check_error;
	check_error = MemoryHeapGpuControl::GetInstance()->BuildResourceFromHeap(buffer_desc_file, resource_desc, resource_state, buffer_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	per_memory_size = static_cast<pancy_resource_size>(per_memory_size_in);
	auto memory_data = MemoryHeapGpuControl::GetInstance()->GetMemoryResource(buffer_data);
	/*
	auto check_size = memory_data->GetSize() % static_cast<uint64_t>(per_memory_size_in);
	if (check_size != 0)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the memory size:" + std::to_string(memory_data->GetSize()) + " could not mod the submemory size: " + std::to_string(per_memory_size_in));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build subresource from memory block", error_message);
		return error_message;
	}
	*/
	pancy_object_id all_empty_num = static_cast<pancy_object_id>(memory_data->GetSize() / per_memory_size_in);
	for (pancy_object_id i = 0; i < all_empty_num; ++i)
	{
		empty_sub_memory.insert(i);
	}
	return PancystarEngine::succeed;
}
void SubMemoryData::GetLogMessage(std::vector<std::string> &log_message)
{
	std::string log_message_out = "";
	log_message_out = "heap_list_id:" + std::to_string(buffer_data.heap_list_id) + " memory_block_id:" + std::to_string(buffer_data.memory_block_id);
	log_message.push_back(log_message_out);
	log_message_out = "per_memory_size: " + std::to_string(per_memory_size);
	log_message.push_back(log_message_out);
	log_message_out = "empty_sub_memory_num: " + std::to_string(empty_sub_memory.size());
	log_message.push_back(log_message_out);
	log_message_out = "sub_memory_data_num: " + std::to_string(sub_memory_data.size());
	log_message.push_back(log_message_out);
}
void SubMemoryData::GetLogMessage(Json::Value &root_value, bool &if_empty)
{
	if_empty = true;
	if (sub_memory_data.size() != 0) 
	{
		if_empty = false;
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, "empty_sub_memory_num", empty_sub_memory.size());
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, "sub_memory_data_num", sub_memory_data.size());
	}
}
PancystarEngine::EngineFailReason SubMemoryData::BuildSubMemory(pancy_object_id &offset)
{
	if (empty_sub_memory.size() == 0)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the memory block is full, could not build new memory");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build subresource from memory block", error_message);
		return error_message;
	}
	auto new_sub_memory = *empty_sub_memory.begin();
	offset = new_sub_memory;
	empty_sub_memory.erase(new_sub_memory);
	sub_memory_data.insert(offset);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubMemoryData::FreeSubMemory(const pancy_object_id &offset)
{
	auto check_data = sub_memory_data.find(offset);
	if (check_data == sub_memory_data.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the sub memory" + std::to_string(offset) + " from memory_block", PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free subresource from memory block", error_message);
		return error_message;
	}
	sub_memory_data.erase(offset);
	empty_sub_memory.insert(offset);
	return PancystarEngine::succeed;
}
//二级资源链
SubresourceLiner::SubresourceLiner(
	const std::string &heap_name_in,
	const std::string &hash_name_in,
	const D3D12_RESOURCE_DESC &resource_desc_in,
	const D3D12_RESOURCE_STATES &resource_state_in,
	const pancy_object_id &per_memory_size_in
)
{
	max_id = 0;
	heap_name = heap_name_in;
	hash_name = hash_name_in;
	resource_desc = resource_desc_in;
	resource_state = resource_state_in;
	per_memory_size = per_memory_size_in;
}
void SubresourceLiner::GetLogMessage(std::vector<std::string> &log_message)
{
	std::string now_string;
	now_string = "heap_name: " + heap_name;
	log_message.push_back(now_string);
	now_string = "used_memory_num: " + std::to_string(submemory_list.size());
	log_message.push_back(now_string);
	now_string = "empty_memory_num: " + std::to_string(empty_memory_heap.size());
	log_message.push_back(now_string);
	for (auto data_log = submemory_list.begin(); data_log != submemory_list.end(); ++data_log)
	{
		now_string = "memory_ID: " + std::to_string(data_log->first);
		log_message.push_back(now_string);
		data_log->second->GetLogMessage(log_message);
	}
}
void SubresourceLiner::GetLogMessage(Json::Value &root_value) 
{
	PancyJsonTool::GetInstance()->SetJsonValue(root_value, "all_memory_num", submemory_list.size());
	PancyJsonTool::GetInstance()->SetJsonValue(root_value, "empty_memory_num", empty_memory_heap.size());
	
	for (auto data_log = submemory_list.begin(); data_log != submemory_list.end(); ++data_log)
	{
		Json::Value submemory_member_value;
		PancyJsonTool::GetInstance()->SetJsonValue(submemory_member_value, "memory_ID", data_log->first);
		bool if_empty = true;
		data_log->second->GetLogMessage(submemory_member_value, if_empty);
		if (!if_empty) 
		{
			PancyJsonTool::GetInstance()->SetJsonValue(root_value, "memory_" + std::to_string(data_log->first), submemory_member_value);
		}
		else 
		{
			PancyJsonTool::GetInstance()->SetJsonValue(root_value, "memory_" + std::to_string(data_log->first), "empty");
		}
	}
	PancyJsonTool::GetInstance()->SetJsonValue(root_value, "per_memory_size", per_memory_size);
}
PancystarEngine::EngineFailReason SubresourceLiner::BuildSubresource(
	pancy_object_id &new_memory_block_id,
	pancy_object_id &sub_memory_offset
)
{
	PancystarEngine::EngineFailReason check_error;
	if (empty_memory_heap.size() == 0)
	{
		SubMemoryData *new_data = new SubMemoryData();
		check_error = new_data->Create(heap_name, resource_desc, resource_state, per_memory_size);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		pancy_object_id id_now;
		//获取当前闲置的id号
		if (free_id.size() == 0)
		{
			id_now = max_id;
			max_id += 1;
		}
		else
		{
			id_now = *free_id.begin();
			free_id.erase(id_now);
		}
		//插入一个空的资源
		submemory_list.insert(std::pair<pancy_object_id, SubMemoryData*>(id_now, new_data));
		empty_memory_heap.insert(id_now);
	}
	//获取一个尚有空间的内存块
	new_memory_block_id = *empty_memory_heap.begin();
	auto new_memory_block = submemory_list.find(new_memory_block_id);
	//在该内存块中开辟一个subresource
	check_error = new_memory_block->second->BuildSubMemory(sub_memory_offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//检查开辟过后该内存块是否已满
	if (new_memory_block->second->GetEmptySize() == 0)
	{
		empty_memory_heap.erase(new_memory_block_id);
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceLiner::ReleaseSubResource
(
	const pancy_object_id &new_memory_block_id,
	const pancy_object_id &sub_memory_offset
)
{
	auto memory_check = submemory_list.find(new_memory_block_id);
	if (memory_check == submemory_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find memory_block id:" + std::to_string(new_memory_block_id) + " from submemory list: " + heap_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Release sub memory from submemory list", error_message);
		return error_message;
	}
	memory_check->second->FreeSubMemory(sub_memory_offset);
	//检查是否之前已满
	if (empty_memory_heap.find(new_memory_block_id) == empty_memory_heap.end())
	{
		empty_memory_heap.insert(new_memory_block_id);
	}
	//todo: 一个资源链上没有资源引用时则删除该资源链
	return PancystarEngine::succeed;
}
MemoryBlockGpu* SubresourceLiner::GetSubResource(pancy_object_id sub_memory_id, pancy_resource_size &per_memory_size)
{
	auto subresource_block = submemory_list.find(sub_memory_id);
	if (subresource_block == submemory_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find memory_block id:" + std::to_string(sub_memory_id) + " from submemory list: " + heap_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Release sub memory from submemory list", error_message);
		return NULL;
	}
	per_memory_size = subresource_block->second->GetBlockSize();
	return subresource_block->second->GetResource();
}
SubresourceLiner::~SubresourceLiner()
{
	for (auto data_release = submemory_list.begin(); data_release != submemory_list.end(); ++data_release)
	{
		delete data_release->second;
	}
	submemory_list.clear();
	empty_memory_heap.clear();
	free_id.clear();
}
//二级资源管理
SubresourceControl::SubresourceControl()
{
	subresource_id_self_add = 0;
	//资源状态
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_COMMON", static_cast<int32_t>(D3D12_RESOURCE_STATE_COMMON),typeid(D3D12_RESOURCE_STATE_COMMON).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER", static_cast<int32_t>(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),typeid(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_INDEX_BUFFER", static_cast<int32_t>(D3D12_RESOURCE_STATE_INDEX_BUFFER),typeid(D3D12_RESOURCE_STATE_INDEX_BUFFER).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_RENDER_TARGET", static_cast<int32_t>(D3D12_RESOURCE_STATE_RENDER_TARGET),typeid(D3D12_RESOURCE_STATE_RENDER_TARGET).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_UNORDERED_ACCESS", static_cast<int32_t>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS),typeid(D3D12_RESOURCE_STATE_UNORDERED_ACCESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_DEPTH_WRITE", static_cast<int32_t>(D3D12_RESOURCE_STATE_DEPTH_WRITE),typeid(D3D12_RESOURCE_STATE_DEPTH_WRITE).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_DEPTH_READ", static_cast<int32_t>(D3D12_RESOURCE_STATE_DEPTH_READ),typeid(D3D12_RESOURCE_STATE_DEPTH_READ).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE", static_cast<int32_t>(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),typeid(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE", static_cast<int32_t>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),typeid(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_STREAM_OUT", static_cast<int32_t>(D3D12_RESOURCE_STATE_STREAM_OUT),typeid(D3D12_RESOURCE_STATE_STREAM_OUT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT", static_cast<int32_t>(D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),typeid(D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_COPY_DEST", static_cast<int32_t>(D3D12_RESOURCE_STATE_COPY_DEST),typeid(D3D12_RESOURCE_STATE_COPY_DEST).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_COPY_SOURCE", static_cast<int32_t>(D3D12_RESOURCE_STATE_COPY_SOURCE),typeid(D3D12_RESOURCE_STATE_COPY_SOURCE).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_RESOLVE_DEST", static_cast<int32_t>(D3D12_RESOURCE_STATE_RESOLVE_DEST),typeid(D3D12_RESOURCE_STATE_RESOLVE_DEST).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_RESOLVE_SOURCE", static_cast<int32_t>(D3D12_RESOURCE_STATE_RESOLVE_SOURCE),typeid(D3D12_RESOURCE_STATE_RESOLVE_SOURCE).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_GENERIC_READ", static_cast<int32_t>(D3D12_RESOURCE_STATE_GENERIC_READ),typeid(D3D12_RESOURCE_STATE_GENERIC_READ).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_PRESENT", static_cast<int32_t>(D3D12_RESOURCE_STATE_PRESENT),typeid(D3D12_RESOURCE_STATE_PRESENT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_PREDICATION", static_cast<int32_t>(D3D12_RESOURCE_STATE_PREDICATION),typeid(D3D12_RESOURCE_STATE_PREDICATION).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_VIDEO_DECODE_READ", static_cast<int32_t>(D3D12_RESOURCE_STATE_VIDEO_DECODE_READ),typeid(D3D12_RESOURCE_STATE_VIDEO_DECODE_READ).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE", static_cast<int32_t>(D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE),typeid(D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ", static_cast<int32_t>(D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ),typeid(D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE", static_cast<int32_t>(D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE),typeid(D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE).name());
	//资源格式demention
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_DIMENSION_UNKNOWN", static_cast<int32_t>(D3D12_RESOURCE_DIMENSION_UNKNOWN),typeid(D3D12_RESOURCE_DIMENSION_UNKNOWN).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_DIMENSION_BUFFER", static_cast<int32_t>(D3D12_RESOURCE_DIMENSION_BUFFER),typeid(D3D12_RESOURCE_DIMENSION_BUFFER).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_DIMENSION_TEXTURE1D", static_cast<int32_t>(D3D12_RESOURCE_DIMENSION_TEXTURE1D),typeid(D3D12_RESOURCE_DIMENSION_TEXTURE1D).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_DIMENSION_TEXTURE2D", static_cast<int32_t>(D3D12_RESOURCE_DIMENSION_TEXTURE2D),typeid(D3D12_RESOURCE_DIMENSION_TEXTURE2D).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_DIMENSION_TEXTURE3D", static_cast<int32_t>(D3D12_RESOURCE_DIMENSION_TEXTURE3D),typeid(D3D12_RESOURCE_DIMENSION_TEXTURE3D).name());
	//资源格式DXGI格式
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_UNKNOWN", static_cast<int32_t>(DXGI_FORMAT_UNKNOWN),typeid(DXGI_FORMAT_UNKNOWN).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32B32A32_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R32G32B32A32_TYPELESS),typeid(DXGI_FORMAT_R32G32B32A32_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32B32A32_FLOAT", static_cast<int32_t>(DXGI_FORMAT_R32G32B32A32_FLOAT),typeid(DXGI_FORMAT_R32G32B32A32_FLOAT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32B32A32_UINT", static_cast<int32_t>(DXGI_FORMAT_R32G32B32A32_UINT),typeid(DXGI_FORMAT_R32G32B32A32_UINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32B32A32_SINT", static_cast<int32_t>(DXGI_FORMAT_R32G32B32A32_SINT),typeid(DXGI_FORMAT_R32G32B32A32_SINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32B32_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R32G32B32_TYPELESS),typeid(DXGI_FORMAT_R32G32B32_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32B32_FLOAT", static_cast<int32_t>(DXGI_FORMAT_R32G32B32_FLOAT),typeid(DXGI_FORMAT_R32G32B32_FLOAT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32B32_UINT", static_cast<int32_t>(DXGI_FORMAT_R32G32B32_UINT),typeid(DXGI_FORMAT_R32G32B32_UINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32B32_SINT", static_cast<int32_t>(DXGI_FORMAT_R32G32B32_SINT),typeid(DXGI_FORMAT_R32G32B32_SINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16B16A16_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R16G16B16A16_TYPELESS),typeid(DXGI_FORMAT_R16G16B16A16_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16B16A16_FLOAT", static_cast<int32_t>(DXGI_FORMAT_R16G16B16A16_FLOAT),typeid(DXGI_FORMAT_R16G16B16A16_FLOAT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16B16A16_UNORM", static_cast<int32_t>(DXGI_FORMAT_R16G16B16A16_UNORM),typeid(DXGI_FORMAT_R16G16B16A16_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16B16A16_UINT", static_cast<int32_t>(DXGI_FORMAT_R16G16B16A16_UINT),typeid(DXGI_FORMAT_R16G16B16A16_UINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16B16A16_SNORM", static_cast<int32_t>(DXGI_FORMAT_R16G16B16A16_SNORM),typeid(DXGI_FORMAT_R16G16B16A16_SNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16B16A16_SINT", static_cast<int32_t>(DXGI_FORMAT_R16G16B16A16_SINT),typeid(DXGI_FORMAT_R16G16B16A16_SINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R32G32_TYPELESS),typeid(DXGI_FORMAT_R32G32_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32_FLOAT", static_cast<int32_t>(DXGI_FORMAT_R32G32_FLOAT),typeid(DXGI_FORMAT_R32G32_FLOAT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32_UINT", static_cast<int32_t>(DXGI_FORMAT_R32G32_UINT),typeid(DXGI_FORMAT_R32G32_UINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G32_SINT", static_cast<int32_t>(DXGI_FORMAT_R32G32_SINT),typeid(DXGI_FORMAT_R32G32_SINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32G8X24_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R32G8X24_TYPELESS),typeid(DXGI_FORMAT_R32G8X24_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_D32_FLOAT_S8X24_UINT", static_cast<int32_t>(DXGI_FORMAT_D32_FLOAT_S8X24_UINT),typeid(DXGI_FORMAT_D32_FLOAT_S8X24_UINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS),typeid(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_X32_TYPELESS_G8X24_UINT", static_cast<int32_t>(DXGI_FORMAT_X32_TYPELESS_G8X24_UINT),typeid(DXGI_FORMAT_X32_TYPELESS_G8X24_UINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R10G10B10A2_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R10G10B10A2_TYPELESS),typeid(DXGI_FORMAT_R10G10B10A2_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R10G10B10A2_UNORM", static_cast<int32_t>(DXGI_FORMAT_R10G10B10A2_UNORM),typeid(DXGI_FORMAT_R10G10B10A2_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R10G10B10A2_UINT", static_cast<int32_t>(DXGI_FORMAT_R10G10B10A2_UINT),typeid(DXGI_FORMAT_R10G10B10A2_UINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R11G11B10_FLOAT", static_cast<int32_t>(DXGI_FORMAT_R11G11B10_FLOAT),typeid(DXGI_FORMAT_R11G11B10_FLOAT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8B8A8_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R8G8B8A8_TYPELESS),typeid(DXGI_FORMAT_R8G8B8A8_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8B8A8_UNORM", static_cast<int32_t>(DXGI_FORMAT_R8G8B8A8_UNORM),typeid(DXGI_FORMAT_R8G8B8A8_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8B8A8_UNORM_SRGB", static_cast<int32_t>(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB),typeid(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8B8A8_UINT", static_cast<int32_t>(DXGI_FORMAT_R8G8B8A8_UINT),typeid(DXGI_FORMAT_R8G8B8A8_UINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8B8A8_SNORM", static_cast<int32_t>(DXGI_FORMAT_R8G8B8A8_SNORM),typeid(DXGI_FORMAT_R8G8B8A8_SNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8B8A8_SINT", static_cast<int32_t>(DXGI_FORMAT_R8G8B8A8_SINT),typeid(DXGI_FORMAT_R8G8B8A8_SINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R16G16_TYPELESS),typeid(DXGI_FORMAT_R16G16_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16_FLOAT", static_cast<int32_t>(DXGI_FORMAT_R16G16_FLOAT),typeid(DXGI_FORMAT_R16G16_FLOAT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16_UNORM", static_cast<int32_t>(DXGI_FORMAT_R16G16_UNORM),typeid(DXGI_FORMAT_R16G16_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16_UINT", static_cast<int32_t>(DXGI_FORMAT_R16G16_UINT),typeid(DXGI_FORMAT_R16G16_UINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16_SNORM", static_cast<int32_t>(DXGI_FORMAT_R16G16_SNORM),typeid(DXGI_FORMAT_R16G16_SNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16G16_SINT", static_cast<int32_t>(DXGI_FORMAT_R16G16_SINT),typeid(DXGI_FORMAT_R16G16_SINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R32_TYPELESS),typeid(DXGI_FORMAT_R32_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_D32_FLOAT", static_cast<int32_t>(DXGI_FORMAT_D32_FLOAT),typeid(DXGI_FORMAT_D32_FLOAT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32_FLOAT", static_cast<int32_t>(DXGI_FORMAT_R32_FLOAT),typeid(DXGI_FORMAT_R32_FLOAT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32_UINT", static_cast<int32_t>(DXGI_FORMAT_R32_UINT),typeid(DXGI_FORMAT_R32_UINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R32_SINT", static_cast<int32_t>(DXGI_FORMAT_R32_SINT),typeid(DXGI_FORMAT_R32_SINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R24G8_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R24G8_TYPELESS),typeid(DXGI_FORMAT_R24G8_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_D24_UNORM_S8_UINT", static_cast<int32_t>(DXGI_FORMAT_D24_UNORM_S8_UINT),typeid(DXGI_FORMAT_D24_UNORM_S8_UINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R24_UNORM_X8_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R24_UNORM_X8_TYPELESS),typeid(DXGI_FORMAT_R24_UNORM_X8_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_X24_TYPELESS_G8_UINT", static_cast<int32_t>(DXGI_FORMAT_X24_TYPELESS_G8_UINT),typeid(DXGI_FORMAT_X24_TYPELESS_G8_UINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R8G8_TYPELESS),typeid(DXGI_FORMAT_R8G8_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8_UNORM", static_cast<int32_t>(DXGI_FORMAT_R8G8_UNORM),typeid(DXGI_FORMAT_R8G8_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8_UINT", static_cast<int32_t>(DXGI_FORMAT_R8G8_UINT),typeid(DXGI_FORMAT_R8G8_UINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8_SNORM", static_cast<int32_t>(DXGI_FORMAT_R8G8_SNORM),typeid(DXGI_FORMAT_R8G8_SNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8_SINT", static_cast<int32_t>(DXGI_FORMAT_R8G8_SINT),typeid(DXGI_FORMAT_R8G8_SINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R16_TYPELESS),typeid(DXGI_FORMAT_R16_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16_FLOAT", static_cast<int32_t>(DXGI_FORMAT_R16_FLOAT),typeid(DXGI_FORMAT_R16_FLOAT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_D16_UNORM", static_cast<int32_t>(DXGI_FORMAT_D16_UNORM),typeid(DXGI_FORMAT_D16_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16_UNORM", static_cast<int32_t>(DXGI_FORMAT_R16_UNORM),typeid(DXGI_FORMAT_R16_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16_UINT", static_cast<int32_t>(DXGI_FORMAT_R16_UINT),typeid(DXGI_FORMAT_R16_UINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16_SNORM", static_cast<int32_t>(DXGI_FORMAT_R16_SNORM),typeid(DXGI_FORMAT_R16_SNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R16_SINT", static_cast<int32_t>(DXGI_FORMAT_R16_SINT),typeid(DXGI_FORMAT_R16_SINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_R8_TYPELESS),typeid(DXGI_FORMAT_R8_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8_UNORM", static_cast<int32_t>(DXGI_FORMAT_R8_UNORM),typeid(DXGI_FORMAT_R8_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8_UINT", static_cast<int32_t>(DXGI_FORMAT_R8_UINT),typeid(DXGI_FORMAT_R8_UINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8_SNORM", static_cast<int32_t>(DXGI_FORMAT_R8_SNORM),typeid(DXGI_FORMAT_R8_SNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8_SINT", static_cast<int32_t>(DXGI_FORMAT_R8_SINT),typeid(DXGI_FORMAT_R8_SINT).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_A8_UNORM", static_cast<int32_t>(DXGI_FORMAT_A8_UNORM),typeid(DXGI_FORMAT_A8_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R1_UNORM", static_cast<int32_t>(DXGI_FORMAT_R1_UNORM),typeid(DXGI_FORMAT_R1_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R9G9B9E5_SHAREDEXP", static_cast<int32_t>(DXGI_FORMAT_R9G9B9E5_SHAREDEXP),typeid(DXGI_FORMAT_R9G9B9E5_SHAREDEXP).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R8G8_B8G8_UNORM", static_cast<int32_t>(DXGI_FORMAT_R8G8_B8G8_UNORM),typeid(DXGI_FORMAT_R8G8_B8G8_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_G8R8_G8B8_UNORM", static_cast<int32_t>(DXGI_FORMAT_G8R8_G8B8_UNORM),typeid(DXGI_FORMAT_G8R8_G8B8_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC1_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_BC1_TYPELESS),typeid(DXGI_FORMAT_BC1_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC1_UNORM", static_cast<int32_t>(DXGI_FORMAT_BC1_UNORM),typeid(DXGI_FORMAT_BC1_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC1_UNORM_SRGB", static_cast<int32_t>(DXGI_FORMAT_BC1_UNORM_SRGB),typeid(DXGI_FORMAT_BC1_UNORM_SRGB).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC2_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_BC2_TYPELESS),typeid(DXGI_FORMAT_BC2_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC2_UNORM", static_cast<int32_t>(DXGI_FORMAT_BC2_UNORM),typeid(DXGI_FORMAT_BC2_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC2_UNORM_SRGB", static_cast<int32_t>(DXGI_FORMAT_BC2_UNORM_SRGB),typeid(DXGI_FORMAT_BC2_UNORM_SRGB).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC3_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_BC3_TYPELESS),typeid(DXGI_FORMAT_BC3_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC3_UNORM", static_cast<int32_t>(DXGI_FORMAT_BC3_UNORM),typeid(DXGI_FORMAT_BC3_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC3_UNORM_SRGB", static_cast<int32_t>(DXGI_FORMAT_BC3_UNORM_SRGB),typeid(DXGI_FORMAT_BC3_UNORM_SRGB).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC4_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_BC4_TYPELESS),typeid(DXGI_FORMAT_BC4_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC4_UNORM", static_cast<int32_t>(DXGI_FORMAT_BC4_UNORM),typeid(DXGI_FORMAT_BC4_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC4_SNORM", static_cast<int32_t>(DXGI_FORMAT_BC4_SNORM),typeid(DXGI_FORMAT_BC4_SNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC5_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_BC5_TYPELESS),typeid(DXGI_FORMAT_BC5_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC5_UNORM", static_cast<int32_t>(DXGI_FORMAT_BC5_UNORM),typeid(DXGI_FORMAT_BC5_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC5_SNORM", static_cast<int32_t>(DXGI_FORMAT_BC5_SNORM),typeid(DXGI_FORMAT_BC5_SNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B5G6R5_UNORM", static_cast<int32_t>(DXGI_FORMAT_B5G6R5_UNORM),typeid(DXGI_FORMAT_B5G6R5_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B5G5R5A1_UNORM", static_cast<int32_t>(DXGI_FORMAT_B5G5R5A1_UNORM),typeid(DXGI_FORMAT_B5G5R5A1_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B8G8R8A8_UNORM", static_cast<int32_t>(DXGI_FORMAT_B8G8R8A8_UNORM),typeid(DXGI_FORMAT_B8G8R8A8_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B8G8R8X8_UNORM", static_cast<int32_t>(DXGI_FORMAT_B8G8R8X8_UNORM),typeid(DXGI_FORMAT_B8G8R8X8_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM", static_cast<int32_t>(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM),typeid(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B8G8R8A8_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_B8G8R8A8_TYPELESS),typeid(DXGI_FORMAT_B8G8R8A8_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B8G8R8A8_UNORM_SRGB", static_cast<int32_t>(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB),typeid(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B8G8R8X8_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_B8G8R8X8_TYPELESS),typeid(DXGI_FORMAT_B8G8R8X8_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B8G8R8X8_UNORM_SRGB", static_cast<int32_t>(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB),typeid(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC6H_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_BC6H_TYPELESS),typeid(DXGI_FORMAT_BC6H_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC6H_UF16", static_cast<int32_t>(DXGI_FORMAT_BC6H_UF16),typeid(DXGI_FORMAT_BC6H_UF16).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC6H_SF16", static_cast<int32_t>(DXGI_FORMAT_BC6H_SF16),typeid(DXGI_FORMAT_BC6H_SF16).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC7_TYPELESS", static_cast<int32_t>(DXGI_FORMAT_BC7_TYPELESS),typeid(DXGI_FORMAT_BC7_TYPELESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC7_UNORM", static_cast<int32_t>(DXGI_FORMAT_BC7_UNORM),typeid(DXGI_FORMAT_BC7_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_BC7_UNORM_SRGB", static_cast<int32_t>(DXGI_FORMAT_BC7_UNORM_SRGB),typeid(DXGI_FORMAT_BC7_UNORM_SRGB).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_AYUV", static_cast<int32_t>(DXGI_FORMAT_AYUV),typeid(DXGI_FORMAT_AYUV).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_Y410", static_cast<int32_t>(DXGI_FORMAT_Y410),typeid(DXGI_FORMAT_Y410).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_Y416", static_cast<int32_t>(DXGI_FORMAT_Y416),typeid(DXGI_FORMAT_Y416).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_NV12", static_cast<int32_t>(DXGI_FORMAT_NV12),typeid(DXGI_FORMAT_NV12).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_P010", static_cast<int32_t>(DXGI_FORMAT_P010),typeid(DXGI_FORMAT_P010).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_P016", static_cast<int32_t>(DXGI_FORMAT_P016),typeid(DXGI_FORMAT_P016).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_420_OPAQUE", static_cast<int32_t>(DXGI_FORMAT_420_OPAQUE),typeid(DXGI_FORMAT_420_OPAQUE).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_YUY2", static_cast<int32_t>(DXGI_FORMAT_YUY2),typeid(DXGI_FORMAT_YUY2).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_Y210", static_cast<int32_t>(DXGI_FORMAT_Y210),typeid(DXGI_FORMAT_Y210).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_Y216", static_cast<int32_t>(DXGI_FORMAT_Y216),typeid(DXGI_FORMAT_Y216).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_NV11", static_cast<int32_t>(DXGI_FORMAT_NV11),typeid(DXGI_FORMAT_NV11).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_AI44", static_cast<int32_t>(DXGI_FORMAT_AI44),typeid(DXGI_FORMAT_AI44).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_IA44", static_cast<int32_t>(DXGI_FORMAT_IA44),typeid(DXGI_FORMAT_IA44).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_P8", static_cast<int32_t>(DXGI_FORMAT_P8),typeid(DXGI_FORMAT_P8).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_A8P8", static_cast<int32_t>(DXGI_FORMAT_A8P8),typeid(DXGI_FORMAT_A8P8).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_B4G4R4A4_UNORM", static_cast<int32_t>(DXGI_FORMAT_B4G4R4A4_UNORM),typeid(DXGI_FORMAT_B4G4R4A4_UNORM).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_P208", static_cast<int32_t>(DXGI_FORMAT_P208),typeid(DXGI_FORMAT_P208).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_V208", static_cast<int32_t>(DXGI_FORMAT_V208),typeid(DXGI_FORMAT_V208).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_V408", static_cast<int32_t>(DXGI_FORMAT_V408),typeid(DXGI_FORMAT_V408).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("DXGI_FORMAT_FORCE_UINT", static_cast<int32_t>(DXGI_FORMAT_FORCE_UINT),typeid(DXGI_FORMAT_FORCE_UINT).name());
	//资源格式layout格式
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_TEXTURE_LAYOUT_UNKNOWN", static_cast<int32_t>(D3D12_TEXTURE_LAYOUT_UNKNOWN),typeid(D3D12_TEXTURE_LAYOUT_UNKNOWN).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_TEXTURE_LAYOUT_ROW_MAJOR", static_cast<int32_t>(D3D12_TEXTURE_LAYOUT_ROW_MAJOR),typeid(D3D12_TEXTURE_LAYOUT_ROW_MAJOR).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE", static_cast<int32_t>(D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE),typeid(D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE", static_cast<int32_t>(D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE),typeid(D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE).name());
	//资源格式flag格式
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_FLAG_NONE", static_cast<int32_t>(D3D12_RESOURCE_FLAG_NONE),typeid(D3D12_RESOURCE_FLAG_NONE).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET", static_cast<int32_t>(D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),typeid(D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL", static_cast<int32_t>(D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),typeid(D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS", static_cast<int32_t>(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),typeid(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE", static_cast<int32_t>(D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE),typeid(D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER", static_cast<int32_t>(D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER),typeid(D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS", static_cast<int32_t>(D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS),typeid(D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY", static_cast<int32_t>(D3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY),typeid(D3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY).name());
}
PancystarEngine::EngineFailReason SubresourceControl::BuildSubresourceFromFile(
	const std::string &resource_name_in,
	SubMemoryPointer &submemory_pointer
) 
{
	PancystarEngine::EngineFailReason check_error;
	D3D12_RESOURCE_DESC res_desc;
	D3D12_RESOURCE_STATES res_states;
	pancy_object_id per_memory_size;
	std::string resource_heap_name;
	pancy_json_value value_root;
	//加载json文件
	Json::Value root_value;
	check_error = PancyJsonTool::GetInstance()->LoadJsonFile(resource_name_in, root_value);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	//加载资源状态
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name_in,root_value,"D3D12_RESOURCE_STATES",pancy_json_data_type::json_data_enum, value_root);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	res_states = static_cast<D3D12_RESOURCE_STATES>(value_root.int_value);
	//加载资源分块大小
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name_in, root_value, "per_block_size", pancy_json_data_type::json_data_int, value_root);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	per_memory_size = value_root.int_value;
	//加载资源堆的名称
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name_in, root_value, "ResourceType", pancy_json_data_type::json_data_string, value_root);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	resource_heap_name = value_root.string_value;
	//加载资源格式
	pancy_json_value value_res_desc;
	Json::Value resource_desc_value = root_value.get("D3D12_RESOURCE_DESC",Json::Value::null);
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name_in, resource_desc_value, "Alignment", pancy_json_data_type::json_data_int, value_res_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	res_desc.Alignment = value_res_desc.int_value;

	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name_in, resource_desc_value, "DepthOrArraySize", pancy_json_data_type::json_data_int, value_res_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	res_desc.DepthOrArraySize = value_res_desc.int_value;

	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name_in, resource_desc_value, "Dimension", pancy_json_data_type::json_data_enum, value_res_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	res_desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(value_res_desc.int_value);

	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name_in, resource_desc_value, "Flags", pancy_json_data_type::json_data_enum, value_res_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	res_desc.Flags = static_cast<D3D12_RESOURCE_FLAGS>(value_res_desc.int_value);

	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name_in, resource_desc_value, "Format", pancy_json_data_type::json_data_enum, value_res_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	res_desc.Format = static_cast<DXGI_FORMAT>(value_res_desc.int_value);

	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name_in, resource_desc_value, "Height", pancy_json_data_type::json_data_int, value_res_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	res_desc.Height = value_res_desc.int_value;

	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name_in, resource_desc_value, "Width", pancy_json_data_type::json_data_int, value_res_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	res_desc.Width = value_res_desc.int_value;

	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name_in, resource_desc_value, "Layout", pancy_json_data_type::json_data_enum, value_res_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	res_desc.Layout = static_cast<D3D12_TEXTURE_LAYOUT>(value_res_desc.int_value);

	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name_in, resource_desc_value, "MipLevels", pancy_json_data_type::json_data_int, value_res_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	res_desc.MipLevels = value_res_desc.int_value;
	//采样格式
	pancy_json_value value_res_sample;
	Json::Value resource_sample_value = resource_desc_value.get("SampleDesc", Json::Value::null);
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name_in, resource_sample_value, "Count", pancy_json_data_type::json_data_int, value_res_sample);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	res_desc.SampleDesc.Count = value_res_sample.int_value;
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name_in, resource_sample_value, "Quality", pancy_json_data_type::json_data_int, value_res_sample);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	res_desc.SampleDesc.Quality = value_res_sample.int_value;
	//创建资源
	check_error = BuildSubresource(resource_name_in,resource_heap_name, res_desc, res_states, per_memory_size, submemory_pointer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
void SubresourceControl::InitSubResourceType(
	const std::string &hash_name,
	const std::string &heap_name_in,
	const D3D12_RESOURCE_DESC &resource_desc_in,
	const D3D12_RESOURCE_STATES &resource_state_in,
	const pancy_object_id &per_memory_size_in,
	pancy_resource_id &subresource_type_id
)
{
	//std::string hash_name = heap_name_in + "::" + std::to_string(resource_state_in) + "::" + std::to_string(per_memory_size_in);
	SubresourceLiner *new_subresource = new SubresourceLiner(heap_name_in, hash_name, resource_desc_in, resource_state_in, per_memory_size_in);
	if (subresource_free_id.size() != 0)
	{
		subresource_type_id = *subresource_free_id.begin();
		subresource_free_id.erase(subresource_type_id);
	}
	else
	{
		subresource_type_id = subresource_id_self_add;
		subresource_id_self_add += 1;
	}
	//创建一个新的资源存储链并保存名称
	subresource_list_map.insert(std::pair<pancy_resource_id, SubresourceLiner*>(subresource_type_id, new_subresource));
	subresource_init_list.insert(std::pair<std::string, pancy_resource_id>(hash_name, subresource_type_id));
}
PancystarEngine::EngineFailReason SubresourceControl::BuildSubresource(
	const std::string &hash_name,
	const std::string &heap_name_in,
	const D3D12_RESOURCE_DESC &resource_desc_in,
	const D3D12_RESOURCE_STATES &resource_state_in,
	const pancy_object_id &per_memory_size_in,
	SubMemoryPointer &submemory_pointer
)
{
	PancystarEngine::EngineFailReason check_error;
	//todo::修改资源的hash名称
	//std::string hash_name = heap_name_in + "::" + std::to_string(resource_state_in) + "::" + std::to_string(per_memory_size_in);
	//获取资源名称对应的id号,如果没有则重新创建一个
	auto check_data = subresource_init_list.find(hash_name);
	if (check_data == subresource_init_list.end())
	{
		InitSubResourceType(hash_name,heap_name_in, resource_desc_in, resource_state_in, per_memory_size_in, submemory_pointer.type_id);
	}
	else
	{
		submemory_pointer.type_id = check_data->second;
	}
	auto now_subresource_type = subresource_list_map.find(submemory_pointer.type_id);
	check_error = now_subresource_type->second->BuildSubresource(submemory_pointer.list_id, submemory_pointer.offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::WriteFromCpuToBuffer(
	const SubMemoryPointer &submemory_pointer,
	const pancy_resource_size &pointer_offset,
	const void* copy_data,
	const pancy_resource_size data_size
) 
{
	PancystarEngine::EngineFailReason check_error;
	pancy_resource_size per_memory_size;
	auto memory_block = GetResourceData(submemory_pointer, per_memory_size);
	if (memory_block == NULL) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL,"could not find submemory, check log for detail");
		return error_message;
	}
	check_error = memory_block->WriteFromCpuToBuffer(submemory_pointer.offset* per_memory_size + pointer_offset, copy_data, data_size);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::GetBufferCpuPointer(
	const SubMemoryPointer &submemory_pointer,
	UINT8** map_pointer_out
) 
{
	PancystarEngine::EngineFailReason check_error;
	pancy_resource_size per_memory_size;
	auto memory_block = GetResourceData(submemory_pointer, per_memory_size);
	if (memory_block == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find submemory, check log for detail");
		return error_message;
	}
	memory_block->GetCpuMapPointer(map_pointer_out);
	*map_pointer_out += submemory_pointer.offset * per_memory_size;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::WriteFromCpuToBuffer(
	const SubMemoryPointer &submemory_pointer,
	const pancy_resource_size &pointer_offset,
	std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
	UINT64* pRowSizesInBytes,
	UINT* pNumRows
) 
{
	PancystarEngine::EngineFailReason check_error;
	pancy_resource_size per_memory_size;
	auto memory_block = GetResourceData(submemory_pointer, per_memory_size);
	if (memory_block == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find submemory, check log for detail");
		return error_message;
	}
	check_error = memory_block->WriteFromCpuToBuffer(
		submemory_pointer.offset* per_memory_size + pointer_offset,
		subresources,
		pLayouts,
		pRowSizesInBytes,
		pNumRows
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::CopyResource(
	PancyRenderCommandList *commandlist,
	const SubMemoryPointer &src_submemory,
	const SubMemoryPointer &dst_submemory,
	const pancy_resource_size &src_offset,
	const pancy_resource_size &dst_offset,
	const pancy_resource_size &data_size
) 
{
	pancy_resource_size per_memory_size_src;
	pancy_resource_size per_memory_size_dst;
	auto dst_res = GetResourceData(dst_submemory, per_memory_size_dst);
	auto src_res = GetResourceData(src_submemory, per_memory_size_src);
	if (dst_res == NULL || src_res == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find submemory, check log for detail");
		return error_message;
	}
	ResourceBarrier(commandlist, dst_submemory, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
	commandlist->GetCommandList()->CopyBufferRegion(
		dst_res->GetResource().Get(),
		dst_submemory.offset * per_memory_size_dst + dst_offset, 
		src_res->GetResource().Get(),
		src_submemory.offset*per_memory_size_src + src_offset,
		data_size);
	ResourceBarrier(commandlist, dst_submemory, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::CopyResource(
	PancyRenderCommandList *commandlist,
	const SubMemoryPointer &src_submemory,
	const SubMemoryPointer &dst_submemory,
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
	const pancy_object_id &Layout_num
) 
{
	pancy_resource_size per_memory_size_src;
	pancy_resource_size per_memory_size_dst;
	auto dst_res = GetResourceData(dst_submemory, per_memory_size_dst);
	auto src_res = GetResourceData(src_submemory, per_memory_size_src);
	if (dst_res == NULL || src_res == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find submemory, check log for detail");
		return error_message;
	}
	ResourceBarrier(commandlist, dst_submemory, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
	for (UINT i = 0; i < Layout_num; ++i)
	{
		
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT real_layout;
		real_layout.Footprint = pLayouts[i].Footprint;
		real_layout.Offset = pLayouts[i].Offset + per_memory_size_src * src_submemory.offset;
		
		CD3DX12_TEXTURE_COPY_LOCATION Dst(dst_res->GetResource().Get(), i + 0);
		CD3DX12_TEXTURE_COPY_LOCATION Src(src_res->GetResource().Get(), real_layout);
		commandlist->GetCommandList()->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
	}
	ResourceBarrier(commandlist, dst_submemory, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::ResourceBarrier(
	PancyRenderCommandList *commandlist,
	const SubMemoryPointer &src_submemory,
	const D3D12_RESOURCE_STATES &last_state,
	const D3D12_RESOURCE_STATES &now_state
) 
{
	pancy_resource_size per_memory_size;
	auto dst_res = GetResourceData(src_submemory, per_memory_size);
	if (dst_res == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find submemory, check log for detail");
		return error_message;
	}
	commandlist->GetCommandList()->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			dst_res->GetResource().Get(),
			last_state,
			now_state
		)
	);
	//修改资源的访问格式
	pancy_resource_size per_block_size;
	auto res_data = GetResourceData(src_submemory, per_block_size);
	if (res_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find submemory, check log for detail");
		return error_message;
	}
	res_data->SetResourceState(now_state);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::GetResourceState(const SubMemoryPointer &src_submemory, D3D12_RESOURCE_STATES &res_state)
{
	pancy_resource_size per_block_size;
	auto res_data = GetResourceData(src_submemory, per_block_size);
	if (res_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find submemory, check log for detail");
		return error_message;
	}
	res_state = res_data->GetResourceState();
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::CaptureTextureDataToWindows(
	const SubMemoryPointer &tex_data,
	const bool &if_cube_map,
	DirectX::ScratchImage *new_image
)
{
	pancy_resource_size per_block_size;
	auto res_data = GetResourceData(tex_data, per_block_size);
	if (res_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find submemory, check log for detail");
		return error_message;
	}
	HRESULT hr = DirectX::CaptureTexture(
		PancyDx12DeviceBasic::GetInstance()->GetCommandQueueDirect(),
		res_data->GetResource().Get(),
		if_cube_map,
		*new_image,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	if (FAILED(hr)) 
	{
		PancystarEngine::EngineFailReason error_message(hr, "could not Capture texture to windows desc");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("capture texture data for saving", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::GetSubResourceDesc(
	const SubMemoryPointer & tex_data,
	D3D12_RESOURCE_DESC &resource_desc
) 
{
	pancy_resource_size per_block_size;
	auto res_data = GetResourceData(tex_data, per_block_size);
	if (res_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find submemory, check log for detail");
		return error_message;
	}
	resource_desc = res_data->GetResource()->GetDesc();
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::BuildConstantBufferView(
	const SubMemoryPointer &src_submemory,
	const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor
) 
{
	//根据资源指针获取资源
	pancy_resource_size per_block_size;
	auto data_submemory = GetResourceData(src_submemory, per_block_size);
	if (data_submemory == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find submemory, check log for detail");
		return error_message;
	}
	//根据资源数据创建描述符
	D3D12_CONSTANT_BUFFER_VIEW_DESC  CBV_desc;
	CBV_desc.BufferLocation = data_submemory->GetResource()->GetGPUVirtualAddress() + src_submemory.offset * per_block_size;
	CBV_desc.SizeInBytes = per_block_size;
	//创建描述符
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateConstantBufferView(&CBV_desc, DestDescriptor);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::BuildShaderResourceView(
	const SubMemoryPointer &src_submemory,
	const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
	const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
) 
{
	//根据资源指针获取资源
	pancy_resource_size per_block_size;
	auto data_submemory = GetResourceData(src_submemory, per_block_size);
	if (data_submemory == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find submemory, check log for detail");
		return error_message;
	}
	//创建描述符
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateShaderResourceView(data_submemory->GetResource().Get(), &SRV_desc, DestDescriptor);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::BuildRenderTargetView(
	const SubMemoryPointer &src_submemory,
	const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
	const D3D12_RENDER_TARGET_VIEW_DESC  &RTV_desc
) 
{
	//根据资源指针获取资源
	pancy_resource_size per_block_size;
	auto data_submemory = GetResourceData(src_submemory, per_block_size);
	if (data_submemory == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find submemory, check log for detail");
		return error_message;
	}
	//创建描述符
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateRenderTargetView(data_submemory->GetResource().Get(), &RTV_desc, DestDescriptor);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::BuildUnorderedAccessView(
	const SubMemoryPointer &src_submemory,
	const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
	const D3D12_UNORDERED_ACCESS_VIEW_DESC  &UAV_desc
) 
{
	//根据资源指针获取资源
	pancy_resource_size per_block_size;
	auto data_submemory = GetResourceData(src_submemory, per_block_size);
	if (data_submemory == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find submemory, check log for detail");
		return error_message;
	}
	//创建描述符
	/*
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateUnorderedAccessView(resource_data->GetResource().Get(), &UAV_desc, cpuHandle);
	*/
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::BuildDepthStencilView(
	const SubMemoryPointer &src_submemory,
	const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
	const D3D12_DEPTH_STENCIL_VIEW_DESC  &DSV_desc
) 
{
	//根据资源指针获取资源
	pancy_resource_size per_block_size;
	auto data_submemory = GetResourceData(src_submemory, per_block_size);
	if (data_submemory == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find submemory, check log for detail");
		return error_message;
	}
	//创建描述符
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateDepthStencilView(data_submemory->GetResource().Get(), &DSV_desc, DestDescriptor);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::BuildVertexBufferView(
	const SubMemoryPointer &src_submemory,
	UINT StrideInBytes,
	D3D12_VERTEX_BUFFER_VIEW &VBV_out
) 
{
	//根据资源指针获取资源
	pancy_resource_size per_block_size;
	auto data_submemory = GetResourceData(src_submemory, per_block_size);
	if (data_submemory == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find submemory, check log for detail");
		return error_message;
	}
	//创建描述符
	VBV_out.BufferLocation = data_submemory->GetResource()->GetGPUVirtualAddress() + src_submemory.offset * per_block_size;
	VBV_out.StrideInBytes = StrideInBytes;
	VBV_out.SizeInBytes = per_block_size;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::BuildIndexBufferView(
	const SubMemoryPointer &src_submemory,
	DXGI_FORMAT StrideInBytes,
	D3D12_INDEX_BUFFER_VIEW &IBV_out
) 
{
	//根据资源指针获取资源
	pancy_resource_size per_block_size;
	auto data_submemory = GetResourceData(src_submemory, per_block_size);
	if (data_submemory == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find submemory, check log for detail");
		return error_message;
	}
	//创建描述符
	IBV_out.BufferLocation = data_submemory->GetResource()->GetGPUVirtualAddress() + src_submemory.offset * per_block_size;
	IBV_out.Format = StrideInBytes;
	IBV_out.SizeInBytes = per_block_size;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::FreeSubResource(const SubMemoryPointer &submemory_pointer)
{
	PancystarEngine::EngineFailReason check_error;
	auto now_subresource_type = subresource_list_map.find(submemory_pointer.type_id);
	if (now_subresource_type == subresource_list_map.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find memory_type id:" + std::to_string(submemory_pointer.type_id) + " from subresource control: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Release sub memory from submemory control", error_message);
		return error_message;
	}
	check_error = now_subresource_type->second->ReleaseSubResource(submemory_pointer.list_id, submemory_pointer.offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
MemoryBlockGpu*  SubresourceControl::GetResourceData(const SubMemoryPointer &submemory_pointer,pancy_resource_size &per_memory_size)
{
	auto submemory_list = subresource_list_map.find(submemory_pointer.type_id);
	if (submemory_list == subresource_list_map.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find memory_type id:" + std::to_string(submemory_pointer.type_id) + " from subresource control: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get sub memory from submemory control", error_message);
		return NULL;
	}
	return submemory_list->second->GetSubResource(submemory_pointer.list_id, per_memory_size);
}
/*
void SubresourceControl::WriteSubMemoryMessageToFile(const std::string &log_file_name)
{
	std::ofstream write_stream;
	write_stream.open(log_file_name);
	for (auto data_submemory = subresource_list_map.begin(); data_submemory != subresource_list_map.end(); ++data_submemory)
	{
		std::vector<std::string> log_message;
		data_submemory->second->GetLogMessage(log_message);
		for (int i = 0; i < log_message.size(); ++i) 
		{
			write_stream<< log_message[i]<<endl;
		}
	}
	write_stream.close();
}
*/
void SubresourceControl::WriteSubMemoryMessageToFile(const std::string &log_file_name)
{
	Json::Value root_value;
	for (auto data_submemory = subresource_list_map.begin(); data_submemory != subresource_list_map.end(); ++data_submemory)
	{
		Json::Value list_value;
		data_submemory->second->GetLogMessage(list_value);
		Json::Value heap_json = root_value.get(data_submemory->second->GetHeapName(), Json::Value::null);
		if (heap_json == Json::Value::null)
		{
			Json::Value new_heap_json;
			PancyJsonTool::GetInstance()->SetJsonValue(new_heap_json, data_submemory->second->GetResourceName(), list_value);
			uint32_t heap_num;
			pancy_resource_size per_heap_size;
			MemoryHeapGpuControl::GetInstance()->GetHeapDesc(data_submemory->second->GetHeapName(), heap_num, per_heap_size);
			PancyJsonTool::GetInstance()->SetJsonValue(new_heap_json, "AllHeapNum", heap_num);
			PancyJsonTool::GetInstance()->SetJsonValue(new_heap_json, "PerHeapSize", per_heap_size);
			PancyJsonTool::GetInstance()->SetJsonValue(root_value, data_submemory->second->GetHeapName(), new_heap_json);
		}
		else 
		{
			PancyJsonTool::GetInstance()->SetJsonValue(heap_json, data_submemory->second->GetResourceName(), list_value);
			PancyJsonTool::GetInstance()->SetJsonValue(root_value, data_submemory->second->GetHeapName(), heap_json);
		}
		
	}
	PancyJsonTool::GetInstance()->WriteValueToJson(root_value, log_file_name);
}
SubresourceControl::~SubresourceControl()
{
	for (auto data_release = subresource_list_map.begin(); data_release != subresource_list_map.end(); ++data_release)
	{
		delete data_release->second;
	}
	subresource_list_map.clear();
	subresource_init_list.clear();
	subresource_free_id.clear();
}
//资源描述视图
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
	resource_block_size = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetDescriptorHandleIncrementSize(resource_type);
}
PancystarEngine::EngineFailReason PancyResourceView::BuildSRV(
	const pancy_object_id &self_offset,
	const SubMemoryPointer &resource_in,
	const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
)
{
	PancystarEngine::EngineFailReason check_error;
	//检验偏移是否合法
	if (self_offset >= resource_view_number)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the srv id: " + std::to_string(self_offset) + " is bigger than the max id of descriptor heap block");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add SRV to descriptor heap block", error_message);
		return error_message;
	}
	//创建描述符
	int32_t heap_start_pos = heap_offset * resource_view_number * static_cast<int32_t>(resource_block_size) + static_cast<int32_t>(self_offset) * static_cast<int32_t>(resource_block_size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap_data->GetCPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(heap_start_pos);
	check_error = SubresourceControl::GetInstance()->BuildShaderResourceView(resource_in, cpuHandle, SRV_desc);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyResourceView::BuildCBV(
	const pancy_object_id &self_offset,
	const SubMemoryPointer &resource_in
)
{
	PancystarEngine::EngineFailReason check_error;
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
	check_error = SubresourceControl::GetInstance()->BuildConstantBufferView(resource_in, cpuHandle);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyResourceView::BuildUAV(
	const pancy_object_id &self_offset,
	const SubMemoryPointer &resource_in,
	const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc
)
{
	PancystarEngine::EngineFailReason check_error;
	//检验偏移是否合法
	if (self_offset >= resource_view_number)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the UAV id: " + std::to_string(self_offset) + " is bigger than the max id of descriptor heap block");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add UAV to descriptor heap block", error_message);
		return error_message;
	}
	//创建描述符
	int32_t heap_start_pos = heap_offset * resource_view_number * static_cast<int32_t>(resource_block_size) + static_cast<int32_t>(self_offset) * static_cast<int32_t>(resource_block_size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap_data->GetCPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(heap_start_pos);
	//创建描述符
	check_error = SubresourceControl::GetInstance()->BuildUnorderedAccessView(resource_in, cpuHandle, UAV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyResourceView::BuildRTV(
	const pancy_object_id &self_offset,
	const SubMemoryPointer &resource_in,
	const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc
)
{
	PancystarEngine::EngineFailReason check_error;
	//检验偏移是否合法
	if (self_offset >= resource_view_number)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the RTV id: " + std::to_string(self_offset) + " is bigger than the max id of descriptor heap block");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add RTV to descriptor heap block", error_message);
		return error_message;
	}
	//创建描述符
	int32_t heap_start_pos = heap_offset * resource_view_number * static_cast<int32_t>(resource_block_size) + static_cast<int32_t>(self_offset) * static_cast<int32_t>(resource_block_size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap_data->GetCPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(heap_start_pos);
	check_error = SubresourceControl::GetInstance()->BuildRenderTargetView(resource_in, cpuHandle, RTV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyResourceView::BuildDSV(
	const pancy_object_id &self_offset,
	const SubMemoryPointer &resource_in,
	const D3D12_DEPTH_STENCIL_VIEW_DESC    &DSV_desc
) 
{
	PancystarEngine::EngineFailReason check_error;
	//检验偏移是否合法
	if (self_offset >= resource_view_number)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the DSV id: " + std::to_string(self_offset) + " is bigger than the max id of descriptor heap block");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add DSV to descriptor heap block", error_message);
		return error_message;
	}
	//创建描述符
	int32_t heap_start_pos = heap_offset * resource_view_number * static_cast<int32_t>(resource_block_size) + static_cast<int32_t>(self_offset) * static_cast<int32_t>(resource_block_size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap_data->GetCPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(heap_start_pos);
	check_error = SubresourceControl::GetInstance()->BuildDepthStencilView(resource_in, cpuHandle, DSV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
//资源描述符管理堆
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
	per_offset_size = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetDescriptorHandleIncrementSize(heap_desc.Type);
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::Create()
{
	//创建描述符堆
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap_data));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "create descriptor heap error");
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
	/*
	if (resource_view_ID > heap_block_num)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor heap resource id: " + std::to_string(resource_view_ID) + " is bigger than the descriptor heap" + descriptor_heap_name + " size: " + std::to_string(heap_block_num));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build resource view from desciptor heap", error_message);
		return error_message;
	}
	if (resource_view_heap_block.find(resource_view_ID) != resource_view_heap_block.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor heap resource id: " + std::to_string(resource_view_ID) + " is already build do not rebuild resource in heap: " + descriptor_heap_name, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build resource view from desciptor heap", error_message);
		return error_message;
	}
	*/
	if (empty_view_block.size() == 0)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor heap" + descriptor_heap_name + " do not have enough space to build a new resource view");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build resource view from desciptor heap", error_message);
		return error_message;
	}
	resource_view_ID = *empty_view_block.begin();
	PancyResourceView *new_data = new PancyResourceView(heap_data, resource_view_ID, heap_desc.Type, heap_block_size);
	resource_view_heap_block.insert(std::pair<pancy_object_id, PancyResourceView*>(resource_view_ID, new_data));
	empty_view_block.erase(resource_view_ID);
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
PancystarEngine::EngineFailReason PancyDescriptorHeap::GetDescriptorHeap(ID3D12DescriptorHeap **descriptor_heap_use)
{
	if (heap_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the discriptor heap haven't succeed inited");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get desciptor heap real pointer", error_message);
		*descriptor_heap_use = NULL;
		return error_message;
	}
	*descriptor_heap_use = heap_data.Get();
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildSRV(
	const pancy_object_id &descriptor_block_id,
	const pancy_object_id &self_offset,
	const SubMemoryPointer &resource_in,
	const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
)
{
	PancystarEngine::EngineFailReason check_error;
	PancyResourceView *descriptor_heap_use = GetHeapBlock(descriptor_block_id, check_error);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = descriptor_heap_use->BuildSRV(self_offset, resource_in, SRV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildCBV(
	const pancy_object_id &descriptor_block_id,
	const pancy_object_id &self_offset,
	const SubMemoryPointer &resource_in
)
{
	PancystarEngine::EngineFailReason check_error;
	PancyResourceView *descriptor_heap_use = GetHeapBlock(descriptor_block_id, check_error);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = descriptor_heap_use->BuildCBV(self_offset, resource_in);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildUAV(
	const pancy_object_id &descriptor_block_id,
	const pancy_object_id &self_offset,
	const SubMemoryPointer &resource_in,
	const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc
)
{
	PancystarEngine::EngineFailReason check_error;
	PancyResourceView *descriptor_heap_use = GetHeapBlock(descriptor_block_id, check_error);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = descriptor_heap_use->BuildUAV(self_offset, resource_in, UAV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildRTV(
	const pancy_object_id &descriptor_block_id,
	const pancy_object_id &self_offset,
	const SubMemoryPointer &resource_in,
	const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc
)
{
	PancystarEngine::EngineFailReason check_error;
	PancyResourceView *descriptor_heap_use = GetHeapBlock(descriptor_block_id, check_error);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = descriptor_heap_use->BuildRTV(self_offset, resource_in, RTV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildDSV(
	const pancy_object_id &descriptor_block_id,
	const pancy_object_id &self_offset,
	const SubMemoryPointer &resource_in,
	const D3D12_DEPTH_STENCIL_VIEW_DESC    &DSV_desc
) 
{
	PancystarEngine::EngineFailReason check_error;
	PancyResourceView *descriptor_heap_use = GetHeapBlock(descriptor_block_id, check_error);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = descriptor_heap_use->BuildDSV(self_offset, resource_in, DSV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
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
PancyDescriptorHeapControl::PancyDescriptorHeapControl()
{
	descriptor_heap_id_selfadd = 0;
	//描述符类型
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV", static_cast<int32_t>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),typeid(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER", static_cast<int32_t>(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),typeid(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_DESCRIPTOR_HEAP_TYPE_RTV", static_cast<int32_t>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV),typeid(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_DESCRIPTOR_HEAP_TYPE_DSV", static_cast<int32_t>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV),typeid(D3D12_DESCRIPTOR_HEAP_TYPE_DSV).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES", static_cast<int32_t>(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES),typeid(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_DESCRIPTOR_HEAP_FLAG_NONE", static_cast<int32_t>(D3D12_DESCRIPTOR_HEAP_FLAG_NONE),typeid(D3D12_DESCRIPTOR_HEAP_FLAG_NONE).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE", static_cast<int32_t>(D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),typeid(D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE).name());
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildDescriptorHeap(
	const std::string &descriptor_heap_name_in,
	const pancy_object_id &heap_block_size_in,
	const D3D12_DESCRIPTOR_HEAP_DESC &heap_desc_in,
	ResourceViewPack &RSV_pack_id
)
{
	PancystarEngine::EngineFailReason check_error;
	//没有对应的描述符堆，先创建一个
	if (resource_init_list.find(descriptor_heap_name_in) == resource_init_list.end())
	{
		pancy_object_id new_heap_id;
		if (resource_memory_free_id.size() != 0)
		{
			new_heap_id = *resource_memory_free_id.begin();
			resource_memory_free_id.erase(new_heap_id);
		}
		else
		{
			new_heap_id = descriptor_heap_id_selfadd;
			descriptor_heap_id_selfadd += 1;
		}
		PancyDescriptorHeap *new_heap = new PancyDescriptorHeap(descriptor_heap_name_in, heap_block_size_in, heap_desc_in);
		check_error = new_heap->Create();
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		resource_init_list.insert(std::pair<std::string, pancy_resource_id>(descriptor_heap_name_in, new_heap_id));
		resource_heap_list.insert(std::pair<pancy_resource_id, PancyDescriptorHeap*>(new_heap_id, new_heap));
	}
	//在描述符下创建一个资源描述包
	RSV_pack_id.descriptor_heap_type_id = resource_init_list.find(descriptor_heap_name_in)->second;
	auto resview_heap_data = resource_heap_list.find(RSV_pack_id.descriptor_heap_type_id);
	check_error = resview_heap_data->second->BuildHeapBlock(RSV_pack_id.descriptor_heap_offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildResourceViewFromFile(
	const std::string &file_name,
	ResourceViewPack &RSV_pack_id,
	pancy_object_id &per_resource_view_pack_size
)
{
	PancystarEngine::EngineFailReason check_error;
	D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc;
	pancy_json_value root_data;
	//加载json文件
	Json::Value root_value;
	check_error = PancyJsonTool::GetInstance()->LoadJsonFile(file_name, root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//读取每个resourceview绑定包的大小
	check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, root_value, "heap_block_size", pancy_json_data_type::json_data_int, root_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	per_resource_view_pack_size = static_cast<pancy_object_id>(root_data.int_value);
	//读取描述符堆的格式
	Json::Value heap_desc_value = root_value.get("D3D12_DESCRIPTOR_HEAP_DESC", Json::Value::null);
	check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, heap_desc_value, "Type", pancy_json_data_type::json_data_enum, root_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	descriptor_heap_desc.Type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(root_data.int_value);

	check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, heap_desc_value, "NumDescriptors", pancy_json_data_type::json_data_int, root_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	descriptor_heap_desc.NumDescriptors = root_data.int_value;

	check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, heap_desc_value, "NodeMask", pancy_json_data_type::json_data_int, root_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	descriptor_heap_desc.NodeMask = root_data.int_value;

	check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, heap_desc_value, "Flags", pancy_json_data_type::json_data_enum, root_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	descriptor_heap_desc.Flags = static_cast<D3D12_DESCRIPTOR_HEAP_FLAGS>(root_data.int_value);
	//创建描述符数据
	check_error = BuildDescriptorHeap(file_name, per_resource_view_pack_size, descriptor_heap_desc, RSV_pack_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
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
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::GetDescriptorHeap(const ResourceViewPack &heap_id, ID3D12DescriptorHeap **descriptor_heap_use)
{
	auto heap_data = resource_heap_list.find(heap_id.descriptor_heap_type_id);
	if (heap_data == resource_heap_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the descriptor heap ID: " + std::to_string(heap_id.descriptor_heap_type_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get descriptor heap", error_message);
		*descriptor_heap_use = NULL;
		return error_message;
	}
	return heap_data->second->GetDescriptorHeap(descriptor_heap_use);
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildSRV(
	const ResourceViewPointer &RSV_point,
	const SubMemoryPointer &resource_in,
	const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
)
{
	auto descriptor_heap_use = resource_heap_list.find(RSV_point.resource_view_pack_id.descriptor_heap_type_id);
	if (descriptor_heap_use == resource_heap_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the descriptor heap id: " + std::to_string(RSV_point.resource_view_pack_id.descriptor_heap_type_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Resource view from descriptor heap", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = descriptor_heap_use->second->BuildSRV(
		RSV_point.resource_view_pack_id.descriptor_heap_offset, 
		RSV_point.resource_view_offset_id, 
		resource_in, 
		SRV_desc
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildCBV(
	const ResourceViewPointer &RSV_point,
	const SubMemoryPointer &resource_in
)
{
	auto descriptor_heap_use = resource_heap_list.find(RSV_point.resource_view_pack_id.descriptor_heap_type_id);
	if (descriptor_heap_use == resource_heap_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the descriptor heap id: " + std::to_string(RSV_point.resource_view_pack_id.descriptor_heap_type_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Resource view from descriptor heap", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = descriptor_heap_use->second->BuildCBV(
		RSV_point.resource_view_pack_id.descriptor_heap_offset, 
		RSV_point.resource_view_offset_id, 
		resource_in
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildUAV(
	const ResourceViewPointer &RSV_point,
	const SubMemoryPointer &resource_in,
	const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc
)
{
	auto descriptor_heap_use = resource_heap_list.find(RSV_point.resource_view_pack_id.descriptor_heap_type_id);
	if (descriptor_heap_use == resource_heap_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the descriptor heap id: " + std::to_string(RSV_point.resource_view_pack_id.descriptor_heap_type_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Resource view from descriptor heap", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = descriptor_heap_use->second->BuildUAV(
		RSV_point.resource_view_pack_id.descriptor_heap_offset, 
		RSV_point.resource_view_offset_id, 
		resource_in, 
		UAV_desc
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildRTV(
	const ResourceViewPointer &RSV_point,
	const SubMemoryPointer &resource_in,
	const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc
)
{
	auto descriptor_heap_use = resource_heap_list.find(RSV_point.resource_view_pack_id.descriptor_heap_type_id);
	if (descriptor_heap_use == resource_heap_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the descriptor heap id: " + std::to_string(RSV_point.resource_view_pack_id.descriptor_heap_type_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Resource view from descriptor heap", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = descriptor_heap_use->second->BuildRTV(
		RSV_point.resource_view_pack_id.descriptor_heap_offset, 
		RSV_point.resource_view_offset_id, 
		resource_in,
		RTV_desc
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildDSV(
	const ResourceViewPointer &RSV_point,
	const SubMemoryPointer &resource_in,
	const D3D12_DEPTH_STENCIL_VIEW_DESC    &DSV_desc
) 
{
	auto descriptor_heap_use = resource_heap_list.find(RSV_point.resource_view_pack_id.descriptor_heap_type_id);
	if (descriptor_heap_use == resource_heap_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the descriptor heap id: " + std::to_string(RSV_point.resource_view_pack_id.descriptor_heap_type_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Resource view from descriptor heap", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = descriptor_heap_use->second->BuildDSV(
		RSV_point.resource_view_pack_id.descriptor_heap_offset,
		RSV_point.resource_view_offset_id,
		resource_in,
		DSV_desc
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
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
