#pragma once
#include"PancystarEngineBasicDx12.h"
#include"PancyResourceBasic.h"
#include"PancyJsonTool.h"
#include"PancyDx12Basic.h"
#define PancyFenceIdGPU  uint64_t
#define PancyThreadIdGPU uint32_t
#define PancyThreadPoolIdGPU uint16_t
#define PancyEngineIdGPU uint8_t
#define MaxSubmitCommandList 256
//commandlist管理(GPU线程(fence)管理)
class PancyRenderCommandList
{
	std::atomic<bool> if_preparing;                       //是否正在准备
	std::atomic<bool> if_finish;                          //是否已经由GPU处理完毕
	/*
	执行完毕状态：
	preparing = false;
	if_finis = true;
	正在填充命令状态
	preparing = true;
	if_finis = false;
	工作/待工作(已完成命令填充但GPU尚未处理完毕)状态
	preparing = false;
	if_finish = false;
	*/
	PancyThreadIdGPU command_list_ID;                     //commandlist的编号
	ComPtr<ID3D12GraphicsCommandList> command_list_data;  //commandlist指针
	D3D12_COMMAND_LIST_TYPE command_list_type;            //commandlist类型
public:
	PancyRenderCommandList(PancyThreadIdGPU command_list_ID_in);
	PancystarEngine::EngineFailReason Create
	(
		ComPtr<ID3D12CommandAllocator> allocator_use_in,
		ComPtr<ID3D12PipelineState> pso_use_in,
		D3D12_COMMAND_LIST_TYPE command_list_type
	);
	inline D3D12_COMMAND_LIST_TYPE GetCommandListType()
	{
		return command_list_type;
	}
	inline ComPtr<ID3D12GraphicsCommandList> GetCommandList()
	{
		return command_list_data;
	}
	//检测当前的commandlist是否正处于准备状态(是否正在占据alloctor进行资源分配)
	inline bool CheckIfPrepare()
	{
		return if_preparing.load();
	};
	//检测当前的commandlist是否已经处理完毕(是否可以释放或重新使用)
	inline bool CheckIfFinish()
	{
		return if_finish.load();
	}
	//锁住资源分配器，开启commandlist的资源分配(进入填充命令状态,前置状态为->执行完毕状态)
	inline void LockPrepare(ComPtr<ID3D12CommandAllocator> allocator_use_in, ComPtr<ID3D12PipelineState> pso_use_in)
	{
		if (if_preparing.load() == false && if_finish.load() == true)
		{
			if_finish.store(false);
			if_preparing.store(true);
			command_list_data->Reset(allocator_use_in.Get(), pso_use_in.Get());
		}
	}
	//解锁资源分配器，关闭commandlist的资源分配(进入工作/待工作状态,前置状态为->填充命令状态)
	inline void UnlockPrepare()
	{
		if (if_preparing.load() == true)
		{
			command_list_data->Close();
			if_preparing.store(false);
		}
	}
	//标志资源处理完毕(进入执行完毕状态,前置状态为->工作/待工作状态)
	inline void EndProcessing() 
	{
		if (if_preparing.load() == false && if_finish.load() == false) 
		{
			if_finish.store(true);
		}
	}
};
class CommandListEngine
{
	bool if_alloctor_locked;//资源分配器是否正在使用
	D3D12_COMMAND_LIST_TYPE engine_type_id;//GPU处理单元的编号(图形处理/通用计算/资源上传/视频解析)
	PancyThreadIdGPU now_prepare_commandlist;//当前正在被分配的commandlist(如果资源分配器未在使用则此项无效)
	PancyThreadIdGPU command_list_ID_selfadd;//自增长的commandlist的id号
	ComPtr<ID3D12CommandAllocator> allocator_use;//commandlist分配器
	HANDLE wait_thread_ID;
	ComPtr<ID3D12Fence> GPU_thread_fence;//用于在接收当前线程处理GPU同步消息的fence
	PancyFenceIdGPU fence_value_self_add;//自增长的fencevalue
	std::unordered_map<PancyFenceIdGPU, std::vector<PancyThreadIdGPU>> GPU_broken_point;//记录每个请求的GPU断点所影响到的commandlist
	//正在工作的commandlist
	std::unordered_map<PancyThreadIdGPU, PancyRenderCommandList*> command_list_work;
	//制作完毕的commandlist
	std::unordered_map<PancyThreadIdGPU, PancyRenderCommandList*> command_list_finish;
	//工作完毕的空闲commandlist
	std::unordered_map<PancyThreadIdGPU, PancyRenderCommandList*> command_list_empty;
	ID3D12CommandList* commandlist_array[MaxSubmitCommandList];
public:
	CommandListEngine(const D3D12_COMMAND_LIST_TYPE &type_need);
	~CommandListEngine();
	PancystarEngine::EngineFailReason Create();
	//获取一个空闲的commandlist
	PancystarEngine::EngineFailReason GetEmptyRenderlist
	(
		ComPtr<ID3D12PipelineState> pso_use_in,
		PancyRenderCommandList** command_list_data,
		PancyThreadIdGPU &command_list_ID
	);
	//设置一个GPU断点
	PancystarEngine::EngineFailReason SetGpuBrokenFence(PancyFenceIdGPU &broken_point_id);
	//查询一个GPU断点
	bool CheckGpuBrokenFence(const PancyFenceIdGPU &broken_point_id);
	//等待一个GPU断点
	PancystarEngine::EngineFailReason WaitGpuBrokenFence(const PancyFenceIdGPU &broken_point_id);
	//提交一个准备完毕的commandlist
	PancystarEngine::EngineFailReason SubmitRenderlist
	(
		const uint32_t command_list_num,
		const PancyThreadIdGPU *command_list_ID
	);
	//释放commandalloctor的内存
	PancystarEngine::EngineFailReason FreeAlloctor();
private:
	//更新当前正在被分配的commandlist，检测其是否已经分配完毕
	void UpdateLastRenderList();
	template<class T>
	void ReleaseList(T &list_in)
	{
		for (auto data_release = list_in.begin(); data_release != list_in.end(); ++data_release)
		{
			delete data_release->second;
		}
		list_in.clear();
	}

};

class ThreadPoolGPU
{
	PancyThreadPoolIdGPU GPUThreadPoolID;
	//multi engine 变量
	std::unordered_map<PancyEngineIdGPU, CommandListEngine*> multi_engine_list;
	//获取对应类型的线程池
	
public:
	ThreadPoolGPU(uint32_t GPUThreadPoolID_in);
	~ThreadPoolGPU();
	inline CommandListEngine* GetThreadPool(D3D12_COMMAND_LIST_TYPE engine_type)
	{
		PancyEngineIdGPU engine_id = static_cast<PancyEngineIdGPU>(D3D12_COMMAND_LIST_TYPE_DIRECT);
		auto engine_data = multi_engine_list.find(engine_id);
		if (engine_data == multi_engine_list.end()) 
		{
			return NULL;
		}
		return engine_data->second;
	}
	PancystarEngine::EngineFailReason Create();
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
