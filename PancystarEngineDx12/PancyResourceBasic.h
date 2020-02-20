#pragma once
#include"PancyMemoryBasic.h"
#include"PancyResourceJsonReflect.h"
namespace PancystarEngine
{
	//资源加载状态
	enum PancyResourceLoadState
	{
		RESOURCE_LOAD_FAILED = 0,
		RESOURCE_LOAD_CPU_FINISH,
		RESOURCE_LOAD_GPU_LOADING,
		RESOURCE_LOAD_GPU_FINISH
	};
	//GPU资源块
	class ResourceBlockGpu
	{
		bool if_start_copying_gpu;
		PancyFenceIdGPU  wait_fence;               //当前资源的加载等待信号量(仅用于显存资源)
		PancyResourceLoadState now_res_load_state; //当前资源的加载状态
		pancy_resource_size memory_size;           //存储块的大小
		ComPtr<ID3D12Resource> resource_data;      //存储块的数据
		D3D12_HEAP_TYPE resource_usage;
		UINT8* map_pointer;
		D3D12_RESOURCE_STATES now_subresource_state;//当前资源的使用格式
	public:
		ResourceBlockGpu(
			const uint64_t &memory_size_in,
			ComPtr<ID3D12Resource> resource_data_in,
			const D3D12_HEAP_TYPE &resource_usage_in,
			const D3D12_RESOURCE_STATES &resource_state
		);
		~ResourceBlockGpu();
		inline ID3D12Resource* GetResource() const
		{
			return resource_data.Get();
		}
		inline uint64_t GetSize() const
		{
			return memory_size;
		}
		//查看当前资源的加载状态
		PancyResourceLoadState GetResourceLoadingState();
		//查看当前资源的使用格式
		D3D12_RESOURCE_STATES GetResourceState()
		{
			return now_subresource_state;
		}
		PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
			const pancy_resource_size &pointer_offset,
			const void* copy_data,
			const pancy_resource_size &data_size
		);
		PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
			const pancy_resource_size &pointer_offset,
			std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
			UINT64* pRowSizesInBytes,
			UINT* pNumRows
		);
		PancystarEngine::EngineFailReason CopyFromDynamicBufferToGpu(
			PancyRenderCommandList *commandlist,
			ResourceBlockGpu &dynamic_buffer,
			const pancy_resource_size &src_offset,
			const pancy_resource_size &dst_offset,
			const pancy_resource_size &data_size
		);
		PancystarEngine::EngineFailReason CopyFromDynamicBufferToGpu(
			PancyRenderCommandList *commandlist,
			ResourceBlockGpu &dynamic_buffer,
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
			const pancy_object_id &Layout_num
		);
		PancystarEngine::EngineFailReason SetResourceCopyBrokenFence(const PancyFenceIdGPU &broken_fence_id);
		PancystarEngine::EngineFailReason GetCpuMapPointer(UINT8** map_pointer_out)
		{
			if (now_res_load_state == RESOURCE_LOAD_FAILED)
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "resource load failed, could not copy data to memory");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::WriteFromCpuToBuffer", error_message);
				return error_message;
			}
			if (resource_usage != D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "resource type is not upload, could not copy data to memory");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get CPU Pointer of memory block gpu", error_message);
				map_pointer_out = NULL;
				return error_message;
			}
			*map_pointer_out = map_pointer;
			return PancystarEngine::succeed;
		}
		PancystarEngine::EngineFailReason ReadFromBufferToCpu(
			const pancy_resource_size &pointer_offset,
			void* copy_data,
			const pancy_resource_size data_size
		);
	private:
		PancystarEngine::EngineFailReason ResourceBarrier(
			PancyRenderCommandList *commandlist,
			ID3D12Resource *src_submemory,
			const D3D12_RESOURCE_STATES &last_state,
			const D3D12_RESOURCE_STATES &now_state
		);
	};
	//upload缓冲区资源块
	struct UploadResourceBlock 
	{
		//存储开辟空间前后的指针位置，由于并非每次开辟都是首尾相连，所以开辟前的位置+存储区的大小并不一定等于开辟后的位置
		pancy_resource_size pointer_before_alloc;
		pancy_resource_size pointer_after_alloc;
		ResourceBlockGpu dynamic_buffer_resource;
		ResourceBlockGpu *static_gpu_resource;
		UploadResourceBlock(
			const pancy_resource_size &pointer_before_alloc_in,
			const pancy_resource_size &pointer_after_alloc_in,
			const uint64_t &memory_size_in,
			ComPtr<ID3D12Resource> resource_data_in,
			const D3D12_HEAP_TYPE &resource_usage_in,
			const D3D12_RESOURCE_STATES &resource_state,
			ResourceBlockGpu *static_gpu_resource_input
		):dynamic_buffer_resource(
			memory_size_in,
			resource_data_in,
			resource_usage_in,
			resource_state
		)
		{
			pointer_before_alloc = pointer_before_alloc_in;
			pointer_after_alloc = pointer_after_alloc_in;
			static_gpu_resource = static_gpu_resource_input;
		}
	};
	class PancyDynamicRingBuffer 
	{
		ComPtr<ID3D12Heap> ringbuffer_heap_data;
		pancy_resource_size buffer_size;
		pancy_resource_size pointer_head_offset;
		pancy_resource_size pointer_tail_offset;
		std::queue<UploadResourceBlock*> ResourceUploadingMap;
		PancyDynamicRingBuffer();
	public:
		static PancyDynamicRingBuffer* GetInstance()
		{
			static PancyDynamicRingBuffer* this_instance;
			if (this_instance == NULL)
			{
				this_instance = new PancyDynamicRingBuffer();
			}
			return this_instance;
		}
		~PancyDynamicRingBuffer();
		//拷贝数据到显存
		PancystarEngine::EngineFailReason CopyDataToGpu(
			PancyRenderCommandList *commandlist,
			void* data_pointer,
			const pancy_resource_size &data_size,
			ResourceBlockGpu &gpu_resource_pointer
		);
		PancystarEngine::EngineFailReason CopyDataToGpu(
			PancyRenderCommandList *commandlist,
			std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
			UINT64* pRowSizesInBytes,
			UINT* pNumRows,
			const pancy_resource_size &data_size,
			ResourceBlockGpu &gpu_resource_pointer
		);
	private:
		PancystarEngine::EngineFailReason AllocNewDynamicData(
			pancy_resource_size data_size,
			ResourceBlockGpu &gpu_resource_pointer,
			UploadResourceBlock **new_block
		);
		//加载初始化信息
		PancystarEngine::EngineFailReason LoadInitData();
		//刷新老的缓冲区，释放不需要的资源所占的空间
		PancystarEngine::EngineFailReason RefreshOldDynamicData();
	};
	//虚拟资源
	class PancyBasicVirtualResource
	{
	protected:
		bool if_could_reload;//资源是否允许重复加载
		PancyJsonReflect *resource_desc_value = NULL;
	protected:
		std::string resource_name;
		std::atomic<pancy_object_id> reference_count;
	public:
		PancyBasicVirtualResource(const bool &if_could_reload_in);
		virtual ~PancyBasicVirtualResource();
		PancystarEngine::EngineFailReason Create(const std::string &resource_name_in);
		PancystarEngine::EngineFailReason Create(const std::string &resource_name_in, const Json::Value &root_value_in);
		//修改引用计数
		void AddReference();
		void DeleteReference();
		inline int32_t GetReferenceCount()
		{
			return reference_count;
		}
		inline std::string GetResourceName()
		{
			return resource_name;
		}
	private:
		virtual void BuildJsonReflect(PancyJsonReflect **pointer_data) = 0;
		//注册并加载资源
		virtual PancystarEngine::EngineFailReason InitResource() = 0;
		//检测当前的资源是否已经被载入GPU
		virtual bool CheckIfResourceLoadFinish() = 0;
	};
	//虚拟资源的模拟智能指针
	class VirtualResourcePointer
	{
		bool if_NULL;
		pancy_object_id resource_id;
		PancyBasicVirtualResource *data_pointer = NULL;
	public:
		VirtualResourcePointer();
		VirtualResourcePointer(const pancy_object_id &resource_id_in);
		VirtualResourcePointer(const VirtualResourcePointer & copy_data);
		~VirtualResourcePointer();
		VirtualResourcePointer& operator=(const VirtualResourcePointer& b);
		PancystarEngine::EngineFailReason MakeShared(const pancy_object_id &resource_id_in);
		inline pancy_object_id GetResourceId() const
		{
			return resource_id;
		}
		inline  PancyBasicVirtualResource *GetResourceData()
		{
			if (if_NULL)
			{
				return NULL;
			}
			else
			{
				return data_pointer;
			}
		}
	};
	class PancyGlobelResourceControl
	{
		std::unordered_map<pancy_object_id, PancyBasicVirtualResource*> basic_resource_array;
	private:
		std::unordered_map<std::string, pancy_object_id> resource_name_list;
		std::unordered_set<pancy_object_id> free_id_list;
		PancyGlobelResourceControl();
	public:
		static PancyGlobelResourceControl* GetInstance()
		{
			static PancyGlobelResourceControl* this_instance;
			if (this_instance == NULL)
			{
				this_instance = new PancyGlobelResourceControl();
			}
			return this_instance;
		}
		virtual ~PancyGlobelResourceControl();
		PancystarEngine::EngineFailReason GetResourceById(const pancy_object_id &resource_id, PancyBasicVirtualResource **data_pointer);

		PancystarEngine::EngineFailReason AddResurceReference(const pancy_object_id &resource_id);
		PancystarEngine::EngineFailReason DeleteResurceReference(const pancy_object_id &resource_id);

		template<typename ResourceType>
		PancystarEngine::EngineFailReason LoadResource(
			const std::string &desc_file_in,
			VirtualResourcePointer &id_need,
			bool if_allow_repeat = false
		);
		template<class ResourceType>
		PancystarEngine::EngineFailReason LoadResource(
			const std::string &name_resource_in,
			const Json::Value &root_value,
			VirtualResourcePointer &id_need,
			bool if_allow_repeat = false
		);
	};
	template<class ResourceType>
	PancystarEngine::EngineFailReason PancyGlobelResourceControl::LoadResource(
		const std::string &name_resource_in,
		const Json::Value &root_value,
		VirtualResourcePointer &res_pointer,
		bool if_allow_repeat
	)
	{
		PancystarEngine::EngineFailReason check_error;
		//资源加载判断重复
		if (!if_allow_repeat)
		{
			auto check_data = resource_name_list.find(name_resource_in);
			if (check_data != resource_name_list.end())
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "repeat load resource : " + name_resource_in, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Resource", error_message);
				return error_message;
			}
		}
		//创建一个新的资源
		PancyBasicVirtualResource *new_data = new ResourceType(if_allow_repeat);
		check_error = new_data->Create(name_resource_in, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		int id_now;
		//判断是否有空闲的id编号
		if (free_id_list.size() > 0)
		{
			id_now = *free_id_list.begin();
			free_id_list.erase(id_now);
		}
		else
		{
			id_now = basic_resource_array.size();
		}
		if (!if_allow_repeat)
		{
			//添加名称-id表用于判重
			resource_name_list.insert(std::pair<std::string, pancy_object_id>(name_resource_in, id_now));
		}
		//插入到资源列表
		basic_resource_array.insert(std::pair<pancy_object_id, PancyBasicVirtualResource*>(id_now, new_data));
		check_error = res_pointer.MakeShared(id_now);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
	template<typename ResourceType>
	PancystarEngine::EngineFailReason PancyGlobelResourceControl::LoadResource(
		const std::string &desc_file_in,
		VirtualResourcePointer &res_pointer,
		bool if_allow_repeat
	)
	{
		PancystarEngine::EngineFailReason check_error;
		Json::Value root_value;
		check_error = PancyJsonTool::GetInstance()->LoadJsonFile(desc_file_in, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = LoadResource(desc_file_in, root_value, res_pointer, if_allow_repeat);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
};

