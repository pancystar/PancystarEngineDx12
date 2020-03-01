#pragma once
#define AnimationSize
#include"PancyModelBasic.h"
#include"PancyResourceBasic.h"
#define threadBlockSize 128
namespace PancystarEngine
{
	//普通的绑定描述符
	struct CommonDescriptorPointer
	{
		//是否使用双缓冲
		bool if_multi_buffer;
		//描述符页的起始地址
		std::vector<pancy_object_id> descriptor_offset;
		//描述符类型
		D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type;
		//描述符对应的资源指针
		std::vector<VirtualResourcePointer> resource_data;
		CommonDescriptorPointer()
		{
			//初始化普通描述符指针
			if_multi_buffer = false;
			descriptor_type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		}
	};
	//解绑定的描述符页
	struct BindlessResourceViewPointer
	{
		//描述符页的起始地址
		pancy_object_id resource_view_offset;
		//描述符页所包含的描述符数量
		pancy_object_id resource_view_num;
		//每一个描述符的格式数据
		std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> SRV_desc;
		//每一个描述符所指向的资源
		std::vector<VirtualResourcePointer> describe_memory_data;
		BindlessResourceViewPointer()
		{
			resource_view_offset = 0;
			resource_view_num = 0;
		}
	};
	//描述符段
	class BindlessResourceViewSegmental
	{
		ID3D12DescriptorHeap *descriptor_heap_data;//描述符堆的真实地址
		pancy_object_id per_descriptor_size;       //每个描述符的大小
		pancy_object_id segmental_offset_position; //当前描述符段在全局的偏移
		pancy_object_id max_descriptor_num;        //当前描述符段最大容纳的描述符数量
		pancy_object_id now_pointer_offset;        //当前描述符段已使用数据的偏移
		pancy_object_id now_pointer_refresh;       //当前描述符段如果需要一次整理操作，其合理的起始整理位置
		pancy_object_id now_descriptor_pack_id_self_add;//当前描述符段为每个描述符页分配的ID的自增长号码
		std::queue<pancy_object_id> now_descriptor_pack_id_reuse;//之前被释放掉，现在可以重新使用的描述符ID

		std::unordered_map<pancy_resource_id, BindlessResourceViewPointer> descriptor_data;//每个被分配出来的描述符页
	public:
		BindlessResourceViewSegmental(
			const pancy_object_id &max_descriptor_num,
			const pancy_object_id &segmental_offset_position_in,
			const pancy_object_id &per_descriptor_size_in,
			const ComPtr<ID3D12DescriptorHeap> descriptor_heap_data_in
		);
		inline pancy_object_id GetEmptyDescriptorNum()
		{
			return max_descriptor_num - now_pointer_offset;
		}
		//从描述符段里开辟一组bindless描述符页
		PancystarEngine::EngineFailReason BuildBindlessShaderResourceViewPack(
			const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
			const std::vector<VirtualResourcePointer> &describe_memory_data,
			const pancy_object_id &SRV_pack_size,
			pancy_object_id &SRV_pack_id
		);
		//从描述符段里删除一组bindless描述符页
		PancystarEngine::EngineFailReason DeleteBindlessShaderResourceViewPack(const pancy_object_id &SRV_pack_id);
		//为描述符段进行一次描述符碎片的整理操作
		PancystarEngine::EngineFailReason RefreshBindlessShaderResourceViewPack();
		//从描述符段里删除一组bindless描述符页并执行一次整理操作
		PancystarEngine::EngineFailReason DeleteBindlessShaderResourceViewPackAndRefresh(const pancy_object_id &SRV_pack_id);
		//获取一个描述符页的基础偏移
		const BindlessResourceViewPointer GetDescriptorPageOffset(const pancy_object_id &descriptor_page_id);
	private:
		//根据描述符页的指针信息，在描述符堆开辟描述符
		PancystarEngine::EngineFailReason BuildShaderResourceView(const BindlessResourceViewPointer &resource_view_pointer);
	};

	//解绑定描述符页的id号
	struct BindlessDescriptorID
	{
		//全局ID
		pancy_object_id bindless_id;
		//空余资源的数量
		pancy_object_id empty_resource_size;
		//重载小于运算符
		bool operator<(const BindlessDescriptorID& other)  const;
	};
	//解绑定描述符段的id号
	struct BindlessResourceViewID
	{
		pancy_object_id segmental_id;
		pancy_object_id page_id;
	};

	//用于处理描述符堆
	class PancyDescriptorHeap
	{
		D3D12_DESCRIPTOR_HEAP_DESC descriptor_desc;                                        //描述符堆的格式
		std::string descriptor_heap_name;                                                  //描述符堆的名称
		pancy_object_id per_descriptor_size;                                               //每个描述符的大小
		ComPtr<ID3D12DescriptorHeap> descriptor_heap_data;                                 //描述符堆的真实数据

		//管理三种不同的描述符(全局描述符，bindless描述符以及普通的绑定描述符)
		std::unordered_map<std::string, pancy_object_id> descriptor_globel_map;//描述符堆的全局描述符的数据集合

		pancy_object_id bind_descriptor_num;                                               //描述符堆最大支持的绑定描述符数量
		std::queue<pancy_object_id> bind_descriptor_offset_reuse;                          //绑定描述符的回收利用的ID号
		std::unordered_map<pancy_object_id, CommonDescriptorPointer> descriptor_bind_map;  //描述符堆的绑定描述符的数据集合

		pancy_object_id bindless_descriptor_num;                                                 //描述符堆最大支持的bindless描述符数量
		pancy_object_id per_segmental_size;                                                      //每一个描述符段的大小
		std::unordered_map<pancy_object_id, BindlessDescriptorID> bindless_descriptor_id_map;    //描述符堆的所有bindless描述符段的id集合
		std::map<BindlessDescriptorID, BindlessResourceViewSegmental*> descriptor_segmental_map; //描述符堆的所有bindless描述符段的数据集合

	public:
		PancyDescriptorHeap();
		~PancyDescriptorHeap();
		PancystarEngine::EngineFailReason Create(
			const D3D12_DESCRIPTOR_HEAP_DESC &descriptor_heap_desc,
			const std::string &descriptor_heap_name_in,
			const pancy_object_id &bind_descriptor_num_in,
			const pancy_object_id &bindless_descriptor_num_in,
			const pancy_object_id &per_segmental_size_in
		);
		inline PancystarEngine::EngineFailReason GetDescriptorHeapData(ID3D12DescriptorHeap **descriptor_heap_out)
		{
			*descriptor_heap_out = descriptor_heap_data.Get();
			return PancystarEngine::succeed;
		}
		//创建全局描述符
		PancystarEngine::EngineFailReason BuildGlobelDescriptor(
			const std::string &globel_name,
			const std::vector<BasicDescriptorDesc> &SRV_desc,
			const std::vector <VirtualResourcePointer>& memory_data,
			const bool if_build_multi_buffer
		);
		PancystarEngine::EngineFailReason DeleteGlobelDescriptor(const std::string &globel_name);
		PancystarEngine::EngineFailReason GetGlobelDesciptorID(const std::string &globel_name, pancy_object_id &descriptor_id);
		PancystarEngine::EngineFailReason BindGlobelDescriptor(
			const std::string &globel_name,
			const D3D12_COMMAND_LIST_TYPE &render_param_type,
			PancyRenderCommandList *m_commandList,
			const pancy_object_id &root_signature_offset
		);
		//创建私有的绑定描述符
		PancystarEngine::EngineFailReason BuildBindDescriptor(
			const std::vector<BasicDescriptorDesc> &descriptor_desc,
			const std::vector<VirtualResourcePointer>& memory_data,
			const bool if_build_multi_buffer,
			pancy_object_id &descriptor_id
		);
		PancystarEngine::EngineFailReason DeleteBindDescriptor(const pancy_object_id &descriptor_id);
		PancystarEngine::EngineFailReason BindCommonDescriptor(
			const pancy_object_id &descriptor_id,
			const D3D12_COMMAND_LIST_TYPE &render_param_type,
			PancyRenderCommandList *m_commandList,
			const pancy_object_id &root_signature_offset
		);
		PancystarEngine::EngineFailReason GetCommonDescriptorCpuOffset(const pancy_object_id &descriptor_id, CD3DX12_CPU_DESCRIPTOR_HANDLE &Cpu_Handle);
		//创建私有的bindless描述符页
		PancystarEngine::EngineFailReason BuildBindlessShaderResourceViewPage(
			const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
			const std::vector<VirtualResourcePointer> &describe_memory_data,
			const pancy_object_id &SRV_pack_size,
			BindlessResourceViewID &descriptor_id
		);
		//删除私有的bindless描述符页(可以指定是否删除完毕后对页碎片进行整理)
		PancystarEngine::EngineFailReason DeleteBindlessShaderResourceViewPage(
			const BindlessResourceViewID &descriptor_id,
			bool is_refresh_segmental = true
		);
		//整理所有的描述符段，消耗较大，在切换地图资源的时候配合不整理碎片的删除使用
		PancystarEngine::EngineFailReason RefreshBindlessShaderResourceViewSegmental();
		//将解绑定描述符绑定至rootsignature
		PancystarEngine::EngineFailReason BindBindlessDescriptor(
			const BindlessResourceViewID &descriptor_id,
			const D3D12_COMMAND_LIST_TYPE &render_param_type,
			PancyRenderCommandList *m_commandList,
			const pancy_object_id &root_signature_offset
		);
	private:
		//重新刷新解绑定资源描述符段的大小，当描述符段增删查改的时候被调用
		PancystarEngine::EngineFailReason RefreshBindlessResourcesegmentalSize(const pancy_object_id &resourc_id);
		//预创建描述符数据
		PancystarEngine::EngineFailReason PreBuildBindDescriptor(
			const D3D12_DESCRIPTOR_HEAP_TYPE &descriptor_type,
			const bool if_build_multi_buffer,
			std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> &descriptor_cpu_handle,
			CommonDescriptorPointer &new_descriptor_data
		);
		//获取描述符堆格式
		D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorHeapTypeOfDescriptor(const BasicDescriptorDesc &descriptor_desc);
	};

	//绑定描述符的虚拟指针
	struct BindDescriptorPointer
	{
		//描述符堆ID
		pancy_resource_id descriptor_heap_id;
		//描述符ID
		pancy_object_id descriptor_id;
	};
	//解绑定描述符的虚拟指针
	struct BindlessDescriptorPointer
	{
		//描述符堆ID
		pancy_resource_id descriptor_heap_id;
		//描述符ID
		BindlessResourceViewID descriptor_pack_id;
	};
	//用于管理所有的描述符堆
	class PancyDescriptorHeapControl
	{
		pancy_resource_id descriptor_heap_id_self_add;
		std::queue<pancy_resource_id> descriptor_heap_id_reuse;
		pancy_resource_id common_descriptor_heap_shader_resource;
		pancy_resource_id common_descriptor_heap_render_target;
		pancy_resource_id common_descriptor_heap_depth_stencil;
		std::unordered_map<pancy_resource_id, PancyDescriptorHeap*> descriptor_heap_map;
		PancyDescriptorHeapControl();
	public:
		~PancyDescriptorHeapControl();
		static PancyDescriptorHeapControl* GetInstance()
		{
			static PancyDescriptorHeapControl* this_instance;
			if (this_instance == NULL)
			{
				this_instance = new PancyDescriptorHeapControl();
			}
			return this_instance;
		}
		//获取基础的描述符堆
		PancystarEngine::EngineFailReason GetBasicDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_TYPE &descriptor_desc, ID3D12DescriptorHeap **descriptor_heap_out);
		//全局描述符
		PancystarEngine::EngineFailReason BuildCommonGlobelDescriptor(
			const std::string &globel_srv_name,
			const std::vector<BasicDescriptorDesc> &now_descriptor_desc_in,
			const std::vector<VirtualResourcePointer>& memory_data,
			const bool if_build_multi_buffer
		);
		PancystarEngine::EngineFailReason GetCommonGlobelDescriptorID(
			PancyDescriptorType basic_descriptor_type,
			const std::string &globel_srv_name,
			BindDescriptorPointer &descriptor_id
		);
		PancystarEngine::EngineFailReason BindCommonGlobelDescriptor(
			PancyDescriptorType basic_descriptor_type,
			const std::string &globel_name,
			const D3D12_COMMAND_LIST_TYPE &render_param_type,
			PancyRenderCommandList *m_commandList,
			const pancy_object_id &root_signature_offset
		);
		PancystarEngine::EngineFailReason BindCommonRenderTargetUncontiguous(
			const std::vector<pancy_object_id> rendertarget_list,
			const pancy_object_id depthstencil_descriptor,
			PancyRenderCommandList *m_commandList,
			const bool &if_have_rendertarget = true,
			const bool &if_have_depthstencil = true
		);
		//todo:目前由于RTV在交换链中取出来无法管理，暂时给出取出depthstencil的方法用于测试，在资源管理器重做后要删除
		PancystarEngine::EngineFailReason GetCommonDepthStencilBufferOffset(
			const pancy_object_id depthstencil_descriptor,
			CD3DX12_CPU_DESCRIPTOR_HANDLE &dsvHandle
		);
		//绑定描述符
		PancystarEngine::EngineFailReason BuildCommonDescriptor(
			const std::vector<BasicDescriptorDesc> &now_descriptor_desc_in,
			const std::vector<VirtualResourcePointer>& memory_data,
			const bool if_build_multi_buffer,
			BindDescriptorPointer &descriptor_id
		);
		PancystarEngine::EngineFailReason BindCommonDescriptor(
			const BindDescriptorPointer &descriptor_id,
			const D3D12_COMMAND_LIST_TYPE &render_param_type,
			PancyRenderCommandList *m_commandList,
			const pancy_object_id &root_signature_offset
		);
		//解绑定描述符
		PancystarEngine::EngineFailReason BuildCommonBindlessShaderResourceView(
			const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
			const std::vector<VirtualResourcePointer> &describe_memory_data,
			const pancy_object_id &SRV_pack_size,
			BindlessDescriptorPointer &descriptor_id
		);
		PancystarEngine::EngineFailReason BindBindlessDescriptor(
			const BindlessDescriptorPointer &descriptor_id,
			const D3D12_COMMAND_LIST_TYPE &render_param_type,
			PancyRenderCommandList *m_commandList,
			const pancy_object_id &root_signature_offset
		);
		PancystarEngine::EngineFailReason ClearRenderTarget();
		//添加与删除一个描述符堆
		PancystarEngine::EngineFailReason BuildNewDescriptorHeapFromJson(const std::string &json_name, const Json::Value &root_value, pancy_resource_id &descriptor_heap_id);
		PancystarEngine::EngineFailReason BuildNewDescriptorHeapFromJson(const std::string &json_file_name, pancy_resource_id &descriptor_heap_id);
		PancystarEngine::EngineFailReason DeleteDescriptorHeap(const pancy_resource_id &descriptor_heap_id);
	private:
		pancy_resource_id GetCommonDescriptorHeapID(const BasicDescriptorDesc &descriptor_desc);
		pancy_resource_id GetCommonDescriptorHeapID(const PancyDescriptorType &descriptor_type);
	};

	

}
