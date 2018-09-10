#pragma once
#include"PancystarEngineBasicDx12.h"
#include"PancyResourceBasic.h"
#include"PancyJsonTool.h"
class PancyDx12DeviceBasic
{
	UINT FrameCount;
	//窗口信息
	HWND hwnd_window;
	uint32_t width;
	uint32_t height;
	//dxgi设备(用于更新交换链)
	ComPtr<IDXGIFactory4> dxgi_factory;
	//d3d设备
	ComPtr<ID3D12Device> m_device;
	//交换链
	ComPtr<IDXGISwapChain3> dx12_swapchain;
	//渲染队列(direct类型,copy类型，compute类型)
	ComPtr<ID3D12CommandQueue> command_queue_direct;
	ComPtr<ID3D12CommandQueue> command_queue_copy;
	ComPtr<ID3D12CommandQueue> command_queue_compute;
	//ROOTsignature
	ComPtr<ID3D12RootSignature> root_signature_draw;
private:
	PancyDx12DeviceBasic(HWND hwnd_window, uint32_t width_in, uint32_t height_in);
public:
	static PancyDx12DeviceBasic* d3dbasic_instance;
	static PancystarEngine::EngineFailReason SingleCreate(HWND hwnd_window_in, UINT wind_width_in, UINT wind_hight_in)
	{
		if (d3dbasic_instance != NULL)
		{
			return PancystarEngine::succeed;
		}
		else
		{
			d3dbasic_instance = new PancyDx12DeviceBasic(hwnd_window_in, wind_width_in, wind_hight_in);
			PancystarEngine::EngineFailReason check_failed = d3dbasic_instance->Init();
			return check_failed;
		}
	}
	static PancyDx12DeviceBasic* GetInstance()
	{
		return d3dbasic_instance;
	}
	PancystarEngine::EngineFailReason Init();
	PancystarEngine::EngineFailReason ResetScreen(uint32_t window_width_in, uint32_t window_height_in);
	//获取com资源信息
	inline ComPtr<ID3D12Device> GetD3dDevice()
	{
		return m_device;
	};
	inline ComPtr<ID3D12CommandQueue> GetCommandQueueDirect()
	{
		return command_queue_direct;
	}
	inline ComPtr<ID3D12CommandQueue> GetCommandQueueCopy()
	{
		return command_queue_copy;
	}
	inline ComPtr<ID3D12CommandQueue> GetCommandQueueCompute()
	{
		return command_queue_compute;
	}
	inline ComPtr<IDXGISwapChain3> GetSwapchain()
	{
		return dx12_swapchain;
	}
	//获取帧数字
	inline UINT GetFrameNum()
	{
		return FrameCount;
	}
private:
	void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter);
};
//commandlist管理(GPU线程(fence)管理)
class PancyRenderCommandList
{
	std::atomic<bool> if_preparing;                       //是否正在准备
	std::atomic<bool> if_finish;                          //是否已经由GPU处理完毕
	uint32_t command_list_ID;                             //commandlist的编号
	HANDLE thread_ID;                                     //GPU线程的编号
	UINT64 fence_value;                                   //待冲破栅栏编号
	ComPtr<ID3D12GraphicsCommandList> command_list_data;  //commandlist指针
	D3D12_COMMAND_LIST_TYPE command_list_type;            //commandlist类型
public:
	PancyRenderCommandList(uint32_t command_list_ID_in);
	PancystarEngine::EngineFailReason Create
	(
		ComPtr<ID3D12CommandAllocator> allocator_use_in,
		ComPtr<ID3D12PipelineState> pso_use_in,
		D3D12_COMMAND_LIST_TYPE command_list_type
	);
	PancystarEngine::EngineFailReason BindFenceToThread(ComPtr<ID3D12Fence> fence_use);
	inline UINT64 GetFenceValue()
	{
		return fence_value;
	}
	inline D3D12_COMMAND_LIST_TYPE GetCommandListType()
	{
		return command_list_type;
	}
	inline HANDLE GetThreadID()
	{
		return thread_ID;
	}
	inline ComPtr<ID3D12GraphicsCommandList> GetCommandList()
	{
		return command_list_data;
	}
	inline bool CheckIfPrepare()
	{
		return if_preparing.load();
	};
	inline bool CheckIfFinish()
	{
		return if_finish.load();
	}
	inline void LockPrepare(ComPtr<ID3D12CommandAllocator> allocator_use_in, ComPtr<ID3D12PipelineState> pso_use_in)
	{
		if (if_preparing.load() == false)
		{
			if_preparing.store(true);
			command_list_data->Reset(allocator_use_in.Get(), pso_use_in.Get());
		}
	}
	inline void UnlockPrepare()
	{
		if (if_preparing.load() == true)
		{
			if_preparing.store(false);
			command_list_data->Close();
		}
	}
	inline void StartProcessing()
	{
		if_finish.store(false);
	}
	inline void EndProcessing()
	{
		if_finish.store(true);
	}
	inline void SetWaitingValue(const UINT64 &wait_value_in) 
	{
		fence_value = wait_value_in;
	}

};
struct CommandListEngine
{
	uint32_t engine_type_id;
	int32_t now_prepare_commandlist;
	//multi engine 变量
	uint32_t command_list_ID_selfadd;
	ComPtr<ID3D12CommandAllocator> allocator_use;//commandlist分配器
	ComPtr<ID3D12Fence> GPU_thread_fence;//用于在当前线程处理GPU同步的fence(direct)
	UINT64 fence_value_self_add;
	CommandListEngine(const D3D12_COMMAND_LIST_TYPE &type_need, PancystarEngine::EngineFailReason &error_message_out);
};
class ThreadPoolGPU
{
	uint32_t GPUThreadPoolID;
	uint32_t max_command_list;
	//multi engine 变量
	std::unordered_map<uint32_t, CommandListEngine*> multi_engine_list;
	//正在工作的commandlist
	std::unordered_map<uint32_t, PancyRenderCommandList*> command_list_work;
	//制作完毕的commandlist
	std::unordered_map<uint32_t, PancyRenderCommandList*> command_list_finish;
	//工作完毕的空闲commandlist
	std::unordered_map<uint32_t, PancyRenderCommandList*> command_list_empty;
public:
	ThreadPoolGPU(uint32_t GPUThreadPoolID_in);
	~ThreadPoolGPU();
	PancystarEngine::EngineFailReason Create();
	//获取一个空闲的commandlist
	PancystarEngine::EngineFailReason GetEmptyRenderlist
	(
		ComPtr<ID3D12PipelineState> pso_use_in,
		const D3D12_COMMAND_LIST_TYPE &command_list_type,
		PancyRenderCommandList** command_list_data,
		uint32_t &command_list_ID
	);
	//等待一个工作完毕的commandlist
	PancystarEngine::EngineFailReason WaitWorkRenderlist(const uint32_t &command_list_ID);
	//提交一个准备完毕的commandlist
	PancystarEngine::EngineFailReason SubmitRenderlist
	(
		const D3D12_COMMAND_LIST_TYPE &command_list_type,
		const uint32_t command_list_num,
		const uint32_t *command_list_ID
	);
	//释放commandalloctor的内存
	PancystarEngine::EngineFailReason FreeAlloctor(const D3D12_COMMAND_LIST_TYPE &command_list_type);
private:
	template<class T>
	void ReleaseList(T &list_in)
	{
		for (auto data_release = list_in.begin(); data_release != list_in.end(); ++data_release)
		{
			delete data_release->second;
		}
		list_in.clear();
	}
	PancystarEngine::EngineFailReason UpdateLastRenderList(const D3D12_COMMAND_LIST_TYPE &command_list_type);
};

class ThreadPoolGPUControl
{	
	ThreadPoolGPU* main_contex;//主渲染线程池
	//其他的GPU线程池表
	int GPU_thread_pool_self_add;
	std::unordered_map<uint32_t, ThreadPoolGPU*> GPU_thread_pool_empty;
	std::unordered_map<uint32_t, ThreadPoolGPU*> GPU_thread_pool_working;
private:
	ThreadPoolGPUControl();
	PancystarEngine::EngineFailReason Create();
public:
	static ThreadPoolGPUControl* threadpool_control_instance;
	static PancystarEngine::EngineFailReason SingleCreate()
	{
		if (threadpool_control_instance != NULL)
		{
			return PancystarEngine::succeed;
		}
		else
		{
			threadpool_control_instance = new ThreadPoolGPUControl();
			PancystarEngine::EngineFailReason check_failed = threadpool_control_instance->Create();
			return check_failed;
		}
	}
	static ThreadPoolGPUControl* GetInstance()
	{
		return threadpool_control_instance;
	}
	ThreadPoolGPU* GetMainContex() 
	{
		return main_contex;
	}
	~ThreadPoolGPUControl();
	//todo:为CPU多线程分配command alloctor
};


//显存块
class MemoryBlockGpu :public PancystarEngine::PancyBasicVirtualResource
{
	bool if_use_heap;//是否有专用的显存堆
	std::string memory_heap;//显存堆的名称
	uint64_t memory_size;//存储块的大小
	ComPtr<ID3D12Resource> resource_data_dx12;//存储块的数据
	uint64_t now_memory_offset_point;//当前已经开辟的内存指针位置
public:
	MemoryBlockGpu(
		bool if_use_heap_in,
		std::string memory_heap_in, 
		const uint64_t &memory_size_in
	);
private:
	PancystarEngine::EngineFailReason InitResource();
};
MemoryBlockGpu::MemoryBlockGpu(
	bool if_use_heap_in,
	std::string memory_heap_in,
	const uint64_t &memory_size_in
) : PancyBasicVirtualResource()
{
	if_use_heap = if_use_heap_in;
	memory_heap = memory_heap_in;
	memory_size = 0;
	now_memory_offset_point = 0;
}
//保留显存堆
class MemoryHeapGpu :public PancystarEngine::PancyBasicVirtualResource
{
	uint64_t size_per_block;
	uint32_t max_block_num;
	ComPtr<ID3D12Heap> heap_data;
	std::unordered_set<uint32_t> free_list;
public:
	MemoryHeapGpu();
	//每个显存块的大小
	inline uint64_t GetMemorySizePerBlock()
	{
		return size_per_block;
	}
	//显存堆保留的显存大小
	inline uint32_t GetMaxMemoryBlockNum()
	{
		return max_block_num;
	}
	//显存堆尚未使用的显存大小
	inline size_t GetFreeMemoryBlockNum()
	{
		return free_list.size();
	}
	//PancystarEngine::EngineFailReason Create(const CD3DX12_HEAP_DESC &heap_desc_in, const int32_t &size_per_block_in, const int32_t &max_block_num_in);
	//从显存堆开辟资源
	PancystarEngine::EngineFailReason GetMemoryResource(
		const CD3DX12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_ALLOCATION_INFO &resource_size_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		Microsoft::WRL::Details::ComPtrRef<ComPtr<ID3D12Resource>> ppvResourc,
		int32_t &memory_block_ID
	);
	//检验对应id的资源是否已经被分配
	bool CheckIfFree(int32_t memory_block_ID);
	//释放一个对应id的资源
	PancystarEngine::EngineFailReason FreeMemoryReference(const int32_t &memory_block_ID);
private:
	PancystarEngine::EngineFailReason InitResource(std::string resource_desc_file);
};
MemoryHeapGpu::MemoryHeapGpu():PancyBasicVirtualResource()
{
	size_per_block = 0;
	max_block_num = 0;
}
PancystarEngine::EngineFailReason MemoryHeapGpu::InitResource(std::string resource_desc_file)
{

}
bool MemoryHeapGpu::CheckIfFree(int32_t memory_block_ID) 
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
	const D3D12_RESOURCE_ALLOCATION_INFO &resource_size_desc,
	const D3D12_RESOURCE_STATES &resource_state,
	Microsoft::WRL::Details::ComPtrRef<ComPtr<ID3D12Resource>> ppvResourc,
	int32_t &memory_block_ID
) 
{
	if (resource_size_desc.SizeInBytes != size_per_block) 
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL, "The resource allocated need size " + std::to_string(resource_size_desc.SizeInBytes) + " But the heap " + resource_name + " Could only buid memory with size " + std::to_string(size_per_block));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Allocated Memorty From Heap", check_error);
		return check_error;
	}
	auto rand_free_memory = free_list.begin();
	if (rand_free_memory == free_list.end()) 
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL, "The Heap " + resource_name + " Is empty, can't alloc new memory");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Allocated Memorty From Heap", check_error);
		return check_error;
	}
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreatePlacedResource(
		heap_data.Get(),
		(*rand_free_memory) * size_per_block,
		&resource_desc,
		resource_state,
		nullptr,
		IID_PPV_ARGS(ppvResourc)
	);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason check_error(hr, "Allocate Memory From Heap " + resource_name + "error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Allocated Memorty From Heap", check_error);
		return check_error;
	}
	memory_block_ID = *rand_free_memory;
	free_list.erase(rand_free_memory);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason MemoryHeapGpu::FreeMemoryReference(const int32_t &memory_block_ID)
{
	if (memory_block_ID >= max_block_num)
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL,"The heap "+ resource_name + " Only have " +std::to_string(max_block_num) +" Memory block,ID " +std::to_string(memory_block_ID) +" Out of range");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free Memorty From Heap", check_error);
		return check_error;
	}
	auto check_data = free_list.find(memory_block_ID);
	if (check_data != free_list.end())
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL,"The memory block ID " + std::to_string(memory_block_ID) + " Haven't been allocated");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free Memorty From Heap", check_error);
		return check_error;
	}
	free_list.insert(memory_block_ID);
	return PancystarEngine::succeed;
}

class MemoryHeapGpuControl : public PancystarEngine::PancyBasicResourceControl
{
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
	void BuildResource(PancystarEngine::PancyBasicVirtualResource** resource_out = NULL);
	/*
	PancystarEngine::EngineFailReason BuildHeap(
		const uint32_t &commit_block_num,
		const uint64_t &per_block_size,
		const D3D12_HEAP_TYPE &heap_type_in,
		const D3D12_HEAP_FLAGS &heap_flag_in,
		uint64_t &resource_id,
		uint64_t heap_alignment_size = 0
		);
	*/
};
MemoryHeapGpuControl::MemoryHeapGpuControl() :PancystarEngine::PancyBasicResourceControl("MemoryHeapGpu")
{
}
void MemoryHeapGpuControl::BuildResource(PancystarEngine::PancyBasicVirtualResource** resource_out = NULL)
{
	*resource_out = new MemoryHeapGpu();
}
/*
PancystarEngine::EngineFailReason MemoryHeapGpuControl::BuildHeap(
	const uint32_t &commit_block_num,
	const uint64_t &per_block_size,
	const D3D12_HEAP_TYPE &heap_type_in,
	const D3D12_HEAP_FLAGS &heap_flag_in,
	uint64_t &resource_id,
	uint64_t heap_alignment_size
)
{
	CD3DX12_HEAP_DESC heapDesc(commit_block_num * per_block_size, heap_type_in, heap_alignment_size, heap_flag_in);
	std::string heap_desc_hash_name;
	heap_desc_hash_name = std::to_string(heap_type_in) + 
		std::to_string(heap_flag_in) + 
		"::" + 
		std::to_string(commit_block_num) +
		"::" + 
		std::to_string(per_block_size);
	MemoryHeapGpu *new_heap = new MemoryHeapGpu(heap_desc_hash_name);
	PancystarEngine::EngineFailReason check_error;
	check_error = new_heap->Create(heapDesc, per_block_size, commit_block_num);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = BuildResource(new_heap, resource_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
*/

