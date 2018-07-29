#pragma once
#include"PancystarEngineBasicDx12.h"
#include <atomic>
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
	//同步信号
	ComPtr<ID3D12Fence> Direct_queue_fence;
	std::atomic<UINT64> Direct_queue_fence_value;
	HANDLE Direct_queue_fence_event;
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
	inline ComPtr<ID3D12Fence> GetDirectQueueFence()
	{
		return Direct_queue_fence;
	}
	inline UINT64 GetDirectQueueFenceValue()
	{
		return Direct_queue_fence_value;
	}
	inline HANDLE GetDirectQueueFenceEvent()
	{
		return Direct_queue_fence_event;
	}
	inline void AddDirectQueueFenceValue()
	{
		Direct_queue_fence_value += 1;
	}
	//获取帧数字
	inline UINT GetFrameNum()
	{
		return FrameCount;
	}

private:
	void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter);
};
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
	PancystarEngine::EngineFailReason BindFenceToThread(ComPtr<ID3D12Fence> fence_use, UINT64 fence_value_use);
	inline uint32_t GetFenceValue()
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

};
PancyRenderCommandList::PancyRenderCommandList(uint32_t command_list_ID_in)
{
	thread_ID = NULL;
	command_list_ID = command_list_ID_in;
	if_preparing.store(false);
	if_finish.store(true);
}
PancystarEngine::EngineFailReason PancyRenderCommandList::Create(ComPtr<ID3D12CommandAllocator> allocator_use_in, ComPtr<ID3D12PipelineState> pso_use_in, D3D12_COMMAND_LIST_TYPE command_list_type)
{
	//创建commondlist
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandList(0, command_list_type, allocator_use_in.Get(), nullptr, IID_PPV_ARGS(&command_list_data));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create CommandList Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render command list", error_message);
		return error_message;
	}
	if_preparing.store(true);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyRenderCommandList::BindFenceToThread(ComPtr<ID3D12Fence> fence_use, UINT64 fence_value_use)
{
	HRESULT hr;
	fence_value = fence_value_use;
	hr = fence_use->SetEventOnCompletion(fence_value, thread_ID);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "reflect GPU thread fence to CPU thread handle failed");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build fence value in render command list class", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}



struct CommandListEngine
{
	uint32_t engine_type_id;
	uint32_t now_prepare_commandlist;
	//multi engine 变量
	uint32_t command_list_ID_selfadd;
	ComPtr<ID3D12CommandAllocator> allocator_use;//commandlist分配器
	ComPtr<ID3D12Fence> GPU_thread_fence;//用于在当前线程处理GPU同步的fence(direct)
	UINT64 fence_value_self_add;
	CommandListEngine(const D3D12_COMMAND_LIST_TYPE &type_need, PancystarEngine::EngineFailReason &error_message_out);
};
CommandListEngine::CommandListEngine(const D3D12_COMMAND_LIST_TYPE &type_need, PancystarEngine::EngineFailReason &error_message_out)
{
	HRESULT hr;
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandAllocator(type_need, IID_PPV_ARGS(&allocator_use));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Direct CommandAllocator Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render ThreadPoolGPU", error_message);
		error_message_out = error_message;
		return;
	}
	command_list_ID_selfadd = 0;
	engine_type_id = static_cast<uint32_t>(type_need);
	fence_value_self_add = 0;
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&GPU_thread_fence));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Direct Fence Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render ThreadPoolGPU", error_message);
		error_message_out = error_message;
		return;
	}
	now_prepare_commandlist = -1;
	error_message_out = PancystarEngine::succeed;
}

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
	PancystarEngine::EngineFailReason Create();
	//获取一个空闲的commandlist
	PancystarEngine::EngineFailReason GetEmptyRenderlist
	(
		ComPtr<ID3D12PipelineState> pso_use_in,
		const D3D12_COMMAND_LIST_TYPE &command_list_type,
		PancyRenderCommandList* command_list_data,
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
};
ThreadPoolGPU::ThreadPoolGPU(uint32_t GPUThreadPoolID_in)
{
	GPUThreadPoolID = GPUThreadPoolID_in;
	max_command_list = 1024 * 1024;
}
PancystarEngine::EngineFailReason ThreadPoolGPU::Create()
{
	HRESULT hr;
	//创建direct类型的commandlist引擎
	PancystarEngine::EngineFailReason check_error;
	CommandListEngine *new_engine_list_direct = new CommandListEngine(D3D12_COMMAND_LIST_TYPE_DIRECT, check_error);
	multi_engine_list.insert(std::pair<uint32_t, CommandListEngine*>(new_engine_list_direct->engine_type_id, new_engine_list_direct));
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//创建copy类型的commandlist引擎
	PancystarEngine::EngineFailReason check_error;
	CommandListEngine *new_engine_list_copy = new CommandListEngine(D3D12_COMMAND_LIST_TYPE_COPY, check_error);
	multi_engine_list.insert(std::pair<uint32_t, CommandListEngine*>(new_engine_list_copy->engine_type_id, new_engine_list_copy));
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//创建compute类型的commandlist引擎
	PancystarEngine::EngineFailReason check_error;
	CommandListEngine *new_engine_list_compute = new CommandListEngine(D3D12_COMMAND_LIST_TYPE_COMPUTE, check_error);
	multi_engine_list.insert(std::pair<uint32_t, CommandListEngine*>(new_engine_list_compute->engine_type_id, new_engine_list_compute));
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	/*
	//创建commandallocator
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator_use_direct));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Direct CommandAllocator Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render ThreadPoolGPU", error_message);
		return error_message;
	}
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&allocator_use_copy));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Copy CommandAllocator Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render ThreadPoolGPU", error_message);
		return error_message;
	}
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&allocator_use_compute));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Compute CommandAllocator Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render ThreadPoolGPU", error_message);
		return error_message;
	}
	//创建fence
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Fence Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render ThreadPoolGPU", error_message);
		return error_message;
	}
	*/
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ThreadPoolGPU::GetEmptyRenderlist
(
	ComPtr<ID3D12PipelineState> pso_use_in,
	const D3D12_COMMAND_LIST_TYPE &command_list_type,
	PancyRenderCommandList* command_list_data,
	uint32_t &command_list_ID
)
{
	auto commandlist_engine = multi_engine_list.find(static_cast<uint32_t>(command_list_type));
	if (commandlist_engine == multi_engine_list.end()) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL,"havent init the commandlist engine ID: " + std::to_string(command_list_type));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add command list int Thread Pool GPU",error_message);
		return error_message;
	}
	if (commandlist_engine->second->now_prepare_commandlist != -1)
	{
		command_list_ID = -1;
		//检查上一个处理的commandlist是否已经closed
		auto now_use_commandlist = command_list_empty.find(commandlist_engine->second->now_prepare_commandlist);
		if (now_use_commandlist != command_list_empty.end())
		{
			if (now_use_commandlist->second->CheckIfPrepare())
			{
				//资源分配器已经被锁，无法添加新的commandlist
				PancystarEngine::EngineFailReason error_message(E_FAIL, "The Direct command allocator is locked by command list ID: " + std::to_string(commandlist_engine->second->now_prepare_commandlist));
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add command list int Thread Pool GPU" + std::to_string(GPUThreadPoolID), error_message);
				return error_message;
			}
			else
			{
				//上一个commandlist已经处理完毕，将其移动到已完成列表
				command_list_finish.insert(*now_use_commandlist);
				command_list_empty.erase(now_use_commandlist);
				commandlist_engine->second->now_prepare_commandlist = -1;
			}
		}
	}
	//检查当前是否还有剩余的空闲commandlist
	if (!command_list_empty.empty())
	{
		//从当前空闲的commanlist里获取一个commandlist
		command_list_ID = command_list_empty.begin()->first;
		command_list_data = command_list_empty.begin()->second;
		command_list_data->LockPrepare(commandlist_engine->second->allocator_use, pso_use_in);
		commandlist_engine->second->now_prepare_commandlist = command_list_ID;
	}
	else
	{
		//新建一个空闲的commandlist
		int ID_offset;
		uint32_t comman_list_type = static_cast<uint32_t>(command_list_type);
		ID_offset = comman_list_type * max_command_list;
		int ID_final = commandlist_engine->second->command_list_ID_selfadd + ID_offset;

		PancyRenderCommandList *new_render_command_list = new PancyRenderCommandList(ID_final);
		PancystarEngine::EngineFailReason check_error;
		check_error = new_render_command_list->Create(commandlist_engine->second->allocator_use, pso_use_in, command_list_type);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		command_list_empty.insert(std::pair<uint32_t, PancyRenderCommandList*>(ID_final, new_render_command_list));
		command_list_ID = command_list_empty.begin()->first;
		command_list_data = command_list_empty.begin()->second;
		commandlist_engine->second->now_prepare_commandlist = command_list_ID;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ThreadPoolGPU::WaitWorkRenderlist(const uint32_t &command_list_ID)
{
	auto work_command_list_use = command_list_work.find(command_list_ID);
	if (work_command_list_use != command_list_work.end())
	{
		auto command_list_type = work_command_list_use->second->GetCommandListType();
		auto commandlist_engine = multi_engine_list.find(static_cast<uint32_t>(command_list_type));
		if (commandlist_engine == multi_engine_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "havent init the commandlist engine ID: " + std::to_string(command_list_type));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("wait command list int Thread Pool GPU", error_message);
			return error_message;
		}
		//找到了工作commandlist，查看是否需要等待
		uint32_t fence_value = work_command_list_use->second->GetFenceValue();
		if (commandlist_engine->second->GPU_thread_fence->GetCompletedValue() < fence_value)
		{
			//栅栏尚未被撞破，开始等待
			WaitForSingleObject(work_command_list_use->second->GetThreadID(), INFINITE);
		}
		//将处理完毕的commandlist回收
		command_list_empty.insert(*work_command_list_use);
		command_list_work.erase(work_command_list_use);
	}
	else
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find working thread ID" + std::to_string(command_list_ID), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("wait command list int Thread Pool GPU", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ThreadPoolGPU::SubmitRenderlist
(
	const D3D12_COMMAND_LIST_TYPE &command_list_type,
	const uint32_t command_list_num,
	const uint32_t *command_list_ID
)
{
	int real_command_list_num = 0;
	ID3D12CommandList** commandlist_array = new ID3D12CommandList*[command_list_num];
	std::vector<PancyRenderCommandList*> now_render_list_array;
	for (uint32_t i = 0; i < command_list_num; ++i)
	{
		//组装commandlist
		auto finish_command_list_use = command_list_finish.find(command_list_ID[i]);
		if (finish_command_list_use == command_list_finish.end())
		{
			if (finish_command_list_use->second->GetCommandListType() == command_list_type)
			{
				now_render_list_array.push_back(finish_command_list_use->second);
				commandlist_array[real_command_list_num] = finish_command_list_use->second->GetCommandList().Get();
				real_command_list_num += 1;
			}
			else
			{
				//commandlist格式不正确
				PancystarEngine::EngineFailReason error_message(E_FAIL, "the command list ID:" + std::to_string(command_list_ID[i]) + "have a wrong command list type", PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("submit command list int Thread Pool GPU", error_message);
				return error_message;
			}
		}
		else
		{
			//无法找到commandlist
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find finish thread ID" + std::to_string(command_list_ID[i]), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("submit command list int Thread Pool GPU", error_message);
			return error_message;
		}
	}
	if (real_command_list_num != 0)
	{
		ComPtr<ID3D12CommandQueue> now_command_queue;
		//找到对应的commandqueue
		if (command_list_type == D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)
		{
			now_command_queue = PancyDx12DeviceBasic::GetInstance()->GetCommandQueueDirect();
		}
		if (command_list_type == D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY)
		{
			now_command_queue = PancyDx12DeviceBasic::GetInstance()->GetCommandQueueCopy();
		}
		if (command_list_type == D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE)
		{
			now_command_queue = PancyDx12DeviceBasic::GetInstance()->GetCommandQueueCompute();
		}
		auto commandlist_engine = multi_engine_list.find(static_cast<uint32_t>(command_list_type));
		if (commandlist_engine == multi_engine_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "havent init the commandlist engine ID: " + std::to_string(command_list_type));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("submit command list int Thread Pool GPU", error_message);
			return error_message;
		}
		//提交commandlist
		now_command_queue->ExecuteCommandLists(real_command_list_num, commandlist_array);
		//设置等待栅栏
		now_command_queue->Signal(commandlist_engine->second->GPU_thread_fence.Get(), commandlist_engine->second->fence_value_self_add);
		for (auto render_command_list = now_render_list_array.begin(); render_command_list != now_render_list_array.end(); ++render_command_list)
		{
			auto check_error = (*render_command_list)->BindFenceToThread(commandlist_engine->second->GPU_thread_fence, commandlist_engine->second->fence_value_self_add);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		commandlist_engine->second->fence_value_self_add += 1;
	}
}