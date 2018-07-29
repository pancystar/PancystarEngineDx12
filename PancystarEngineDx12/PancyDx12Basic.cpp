#include"PancyDx12Basic.h"
PancyDx12DeviceBasic* PancyDx12DeviceBasic::d3dbasic_instance = NULL;
PancyDx12DeviceBasic::PancyDx12DeviceBasic(HWND hwnd_window_in, uint32_t width_in, uint32_t height_in)
{
	FrameCount = 2;
	width = width_in;
	height = height_in;
	hwnd_window = hwnd_window_in;
	Direct_queue_fence_value.store(0);
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
	//创建同步渲染对象
	//创建fence
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Direct_queue_fence));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Fence Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Init dx12 basic state", error_message);
		return error_message;
	}
	Direct_queue_fence_value = 1;
	Direct_queue_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (Direct_queue_fence_event == nullptr)
	{
		PancystarEngine::EngineFailReason error_message(HRESULT_FROM_WIN32(GetLastError()), "Create FenceEvent Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Init dx12 basic state", error_message);
		return error_message;
	}
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