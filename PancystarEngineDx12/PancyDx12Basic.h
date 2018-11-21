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


