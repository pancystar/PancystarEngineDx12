#include"PancyDx12Basic.h"
PancyDx12DeviceBasic* PancyDx12DeviceBasic::d3dbasic_instance = NULL;
ThreadPoolGPUControl* ThreadPoolGPUControl::threadpool_control_instance = NULL;
PancyDx12DeviceBasic::PancyDx12DeviceBasic(HWND hwnd_window_in, uint32_t width_in, uint32_t height_in)
{
	FrameCount = 2;
	width = width_in;
	height = height_in;
	hwnd_window = hwnd_window_in;
	//Direct_queue_fence_value.store(0);
}
PancystarEngine::EngineFailReason PancyDx12DeviceBasic::Init()
{
	HRESULT hr;
	PancystarEngine::EngineFailReason check_error;
	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif
	hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgi_factory));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create DXGIFactory Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Init dx12 basic state", error_message);
		return error_message;
	}
	ComPtr<IDXGIAdapter1> hardwareAdapter;
	GetHardwareAdapter(dxgi_factory.Get(), &hardwareAdapter);
	hr = D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Dx12 Device Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Init dx12 basic state", error_message);
		return error_message;
	}
	//创建渲染队列
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&command_queue_direct));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Command Queue(Direct) Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Init dx12 basic state", error_message);
		return error_message;
	}
	//创建屏幕
	check_error = ResetScreen(width, height);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//创建root signature draw
	
	//禁止alt+回车全屏
	dxgi_factory->MakeWindowAssociation(hwnd_window, DXGI_MWA_NO_ALT_ENTER);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDx12DeviceBasic::ResetScreen(uint32_t window_width_in, uint32_t window_height_in)
{
	HRESULT hr;
	//重新创建交换链
	if (dx12_swapchain != NULL)
	{
		dx12_swapchain.Reset();
	}
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	ComPtr<IDXGISwapChain1> swapChain;
	hr = dxgi_factory->CreateSwapChainForHwnd(
		command_queue_direct.Get(),
		hwnd_window,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Swap Chain Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Change window size", error_message);
		return error_message;
	}
	swapChain.As(&dx12_swapchain);
	return PancystarEngine::succeed;
}

void PancyDx12DeviceBasic::GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	ComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			//跳过CPU渲染过程
			continue;
		}
		//检验是否支持dx12
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}
	*ppAdapter = adapter.Detach();
}


PancyRenderCommandList::PancyRenderCommandList(uint32_t command_list_ID_in)
{
	thread_ID = NULL;
	command_list_ID = command_list_ID_in;
	if_preparing.store(false);
	if_finish.store(true);
}
PancystarEngine::EngineFailReason PancyRenderCommandList::Create(ComPtr<ID3D12CommandAllocator> allocator_use_in, ComPtr<ID3D12PipelineState> pso_use_in, D3D12_COMMAND_LIST_TYPE command_list_type_in)
{
	command_list_type = command_list_type_in;
	//创建commondlist
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandList(0, command_list_type_in, allocator_use_in.Get(), nullptr, IID_PPV_ARGS(&command_list_data));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create CommandList Error When build commandlist");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render command list", error_message);
		return error_message;
	}
	//创建fence event
	thread_ID = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (thread_ID == nullptr)
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create fence event Error When build commandlist");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render command list", error_message);
		return error_message;
	}
	if_preparing.store(true);

	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyRenderCommandList::BindFenceToThread(ComPtr<ID3D12Fence> fence_use)
{
	HRESULT hr;
	hr = fence_use->SetEventOnCompletion(fence_value, thread_ID);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "reflect GPU thread fence to CPU thread handle failed");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build fence value in render command list class", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}

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
	fence_value_self_add = 1;
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
ThreadPoolGPU::ThreadPoolGPU(uint32_t GPUThreadPoolID_in)
{
	GPUThreadPoolID = GPUThreadPoolID_in;
	max_command_list = 1024 * 1024;
}

ThreadPoolGPU::~ThreadPoolGPU()
{
	ReleaseList(multi_engine_list);
	ReleaseList(command_list_work);
	ReleaseList(command_list_finish);
	ReleaseList(command_list_empty);
}
PancystarEngine::EngineFailReason ThreadPoolGPU::Create()
{
	//创建direct类型的commandlist引擎
	PancystarEngine::EngineFailReason check_error;
	CommandListEngine *new_engine_list_direct = new CommandListEngine(D3D12_COMMAND_LIST_TYPE_DIRECT, check_error);
	multi_engine_list.insert(std::pair<uint32_t, CommandListEngine*>(new_engine_list_direct->engine_type_id, new_engine_list_direct));
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//创建copy类型的commandlist引擎
	CommandListEngine *new_engine_list_copy = new CommandListEngine(D3D12_COMMAND_LIST_TYPE_COPY, check_error);
	multi_engine_list.insert(std::pair<uint32_t, CommandListEngine*>(new_engine_list_copy->engine_type_id, new_engine_list_copy));
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//创建compute类型的commandlist引擎
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
PancystarEngine::EngineFailReason ThreadPoolGPU::FreeAlloctor(const D3D12_COMMAND_LIST_TYPE &command_list_type)
{
	auto commandlist_engine = multi_engine_list.find(static_cast<uint32_t>(command_list_type));
	if (commandlist_engine == multi_engine_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "havent init the commandlist engine ID: " + std::to_string(command_list_type));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free command list engine in Thread Pool GPU", error_message);
		return error_message;
	}
	//遍历commandlist
	for (auto working_renderabel = command_list_work.begin(); working_renderabel != command_list_work.end(); ++working_renderabel)
	{
		if (working_renderabel->second->GetCommandListType() == command_list_type)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "not all of the command list work finished engine ID: " + std::to_string(command_list_type));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free command allocter int Thread Pool GPU", error_message);
			return error_message;
		}
	}
	HRESULT hr = commandlist_engine->second->allocator_use->Reset();
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "release command allocator error: " + std::to_string(command_list_type));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free command allocter int Thread Pool GPU", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ThreadPoolGPU::UpdateLastRenderList(const D3D12_COMMAND_LIST_TYPE &command_list_type)
{
	auto commandlist_engine = multi_engine_list.find(static_cast<uint32_t>(command_list_type));
	if (commandlist_engine == multi_engine_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "havent init the commandlist engine ID: " + std::to_string(command_list_type));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add command list int Thread Pool GPU", error_message);
		return error_message;
	}
	if (commandlist_engine->second->now_prepare_commandlist != -1)
	{
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
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ThreadPoolGPU::GetEmptyRenderlist
(
	ComPtr<ID3D12PipelineState> pso_use_in,
	const D3D12_COMMAND_LIST_TYPE &command_list_type,
	PancyRenderCommandList** command_list_data,
	uint32_t &command_list_ID
)
{
	//更新commandlist池
	auto check_error = UpdateLastRenderList(command_list_type);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//获取对应类型的command alloctor
	auto commandlist_engine = multi_engine_list.find(static_cast<uint32_t>(command_list_type));
	if (commandlist_engine == multi_engine_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "havent init the commandlist engine ID: " + std::to_string(command_list_type));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add command list int Thread Pool GPU", error_message);
		return error_message;
	}

	//检查当前是否还有剩余的空闲commandlist
	if (!command_list_empty.empty())
	{
		//从当前空闲的commanlist里获取一个commandlist
		command_list_ID = command_list_empty.begin()->first;
		*command_list_data = command_list_empty.begin()->second;
		(*command_list_data)->LockPrepare(commandlist_engine->second->allocator_use, pso_use_in);
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
		*command_list_data = command_list_empty.begin()->second;
		commandlist_engine->second->now_prepare_commandlist = command_list_ID;
		commandlist_engine->second->command_list_ID_selfadd += 1;
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
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("wait command list in Thread Pool GPU", error_message);
			return error_message;
		}
		//找到了工作commandlist，查看是否需要等待
		UINT64 fence_value = work_command_list_use->second->GetFenceValue();
		auto now_wait_value = commandlist_engine->second->GPU_thread_fence->GetCompletedValue();
		if (now_wait_value < fence_value)
		{
			//栅栏尚未被撞破，开始等待
			auto check_error = work_command_list_use->second->BindFenceToThread(commandlist_engine->second->GPU_thread_fence);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			WaitForSingleObject(work_command_list_use->second->GetThreadID(), INFINITE);
		}
		//关闭commandlist
		work_command_list_use->second->EndProcessing();
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
	//更新commandlist池
	auto check_error = UpdateLastRenderList(command_list_type);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//开辟用于提交的commandlist数组
	int real_command_list_num = 0;
	ID3D12CommandList** commandlist_array = new ID3D12CommandList*[command_list_num];
	std::vector<PancyRenderCommandList*> now_render_list_array;
	for (uint32_t i = 0; i < command_list_num; ++i)
	{
		//组装commandlist
		auto finish_command_list_use = command_list_finish.find(command_list_ID[i]);
		if (finish_command_list_use != command_list_finish.end())
		{
			if (finish_command_list_use->second->GetCommandListType() == command_list_type)
			{
				now_render_list_array.push_back(finish_command_list_use->second);
				commandlist_array[real_command_list_num] = finish_command_list_use->second->GetCommandList().Get();
				real_command_list_num += 1;
				//转换commandlist
				command_list_work.insert(*finish_command_list_use);
				command_list_finish.erase(finish_command_list_use);
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
		HRESULT hr = now_command_queue->Signal(commandlist_engine->second->GPU_thread_fence.Get(), commandlist_engine->second->fence_value_self_add);
		if (FAILED(hr))
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "signel waiting value error in engine ID: " + std::to_string(command_list_type));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("submit command list int Thread Pool GPU", error_message);
			return error_message;
		}
		for (auto render_command_list = now_render_list_array.begin(); render_command_list != now_render_list_array.end(); ++render_command_list)
		{
			(*render_command_list)->SetWaitingValue(commandlist_engine->second->fence_value_self_add);
			/*
			auto check_error = (*render_command_list)->BindFenceToThread(commandlist_engine->second->GPU_thread_fence, commandlist_engine->second->fence_value_self_add);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			*/
			(*render_command_list)->StartProcessing();
		}
		commandlist_engine->second->fence_value_self_add += 1;
		delete[] commandlist_array;
		return PancystarEngine::succeed;
	}
	return PancystarEngine::succeed;
}

ThreadPoolGPUControl::ThreadPoolGPUControl()
{
	main_contex = NULL;
	main_contex = new ThreadPoolGPU(0);
	GPU_thread_pool_self_add = 1;
}
PancystarEngine::EngineFailReason ThreadPoolGPUControl::Create()
{
	auto check_error = main_contex->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
ThreadPoolGPUControl::~ThreadPoolGPUControl()
{
	if (main_contex != NULL)
	{
		delete main_contex;
	}
	for (auto data_working = GPU_thread_pool_working.begin(); data_working != GPU_thread_pool_working.end(); ++data_working)
	{
		delete data_working->second;
	}
	GPU_thread_pool_working.clear();
	for (auto data_working = GPU_thread_pool_empty.begin(); data_working != GPU_thread_pool_empty.end(); ++data_working)
	{
		delete data_working->second;
	}
	GPU_thread_pool_empty.clear();
}
