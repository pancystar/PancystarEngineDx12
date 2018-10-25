#pragma once
#include"PancystarEngineBasicDx12.h"
#include"PancyDx12Basic.h"
#include"PancyMemoryBasic.h"
namespace PancystarEngine
{
	//2D顶点格式
	struct Point2D
	{
		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4 tex_color;  //用于采样的坐标
		//DirectX::XMFLOAT4 tex_range;  //用于限制采样矩形的坐标
	};
	//标准3D顶点格式
	struct PointCommon
	{
		DirectX::XMFLOAT3 position;   //位置
		DirectX::XMFLOAT3 normal;     //法线
		DirectX::XMFLOAT3 tangent;    //切线
		DirectX::XMUINT4  tex_id;     //使用的纹理ID号
		DirectX::XMFLOAT4 tex_color;  //用于采样的坐标
		DirectX::XMFLOAT4 tex_range;  //用于限制采样矩形的坐标
	};
	//带骨骼的顶点格式
	struct PointSkinCommon
	{
		DirectX::XMFLOAT3 position;   //位置
		DirectX::XMFLOAT3 normal;     //法线
		DirectX::XMFLOAT3 tangent;    //切线
		DirectX::XMUINT4  tex_id;     //使用的纹理ID号
		DirectX::XMFLOAT4 tex_color;  //用于采样的坐标
		DirectX::XMFLOAT4 tex_range;  //用于限制采样矩形的坐标
		DirectX::XMUINT4  bone_id;    //骨骼ID号
		DirectX::XMFLOAT4 bone_weight;//骨骼权重
	};
	
	
	//几何体基础类型
	class GeometryBasic
	{
	protected:
		//几何体的渲染buffer
		VirtualMemoryPointer geometry_vertex_buffer;
		VirtualMemoryPointer geometry_index_buffer;
		VirtualMemoryPointer geometry_adjindex_buffer;
		D3D12_VERTEX_BUFFER_VIEW geometry_vertex_buffer_view;
		D3D12_INDEX_BUFFER_VIEW geometry_index_buffer_view;
		uint32_t all_vertex;
		uint32_t all_index;
		uint32_t all_index_adj;
		//几何体的创建信息
		bool if_create_adj;
		bool if_buffer_created;
	public:
		GeometryBasic();
		PancystarEngine::EngineFailReason Create();
		virtual ~GeometryBasic();
		inline ComPtr<ID3D12Resource> GetVertexBuffer() 
		{
			return MemoryHeapGpuControl::GetInstance()->GetMemoryResource(geometry_vertex_buffer)->GetResource();
		};
		inline ComPtr<ID3D12Resource> GetIndexBuffer()
		{
			return MemoryHeapGpuControl::GetInstance()->GetMemoryResource(geometry_index_buffer)->GetResource();
		};
		inline ComPtr<ID3D12Resource> GetIndexAdjBuffer()
		{
			return MemoryHeapGpuControl::GetInstance()->GetMemoryResource(geometry_adjindex_buffer)->GetResource();
		};
		inline D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView()
		{
			return geometry_vertex_buffer_view;
		};
		inline D3D12_INDEX_BUFFER_VIEW GetIndexBufferView()
		{
			return geometry_index_buffer_view;
		};
	protected:
		virtual PancystarEngine::EngineFailReason InitGeometryDesc(
			bool &if_create_adj
		) = 0;
		virtual PancystarEngine::EngineFailReason InitGeometry(
			uint32_t &all_vertex_need, 
			uint32_t &all_index_need, 
			uint32_t &all_index_adj_need,
			VirtualMemoryPointer &geometry_vertex_buffer,
			VirtualMemoryPointer &geometry_index_buffer,
			VirtualMemoryPointer &geometry_adjindex_buffer,
			D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
			D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in
		) = 0;
		PancystarEngine::EngineFailReason BuildDefaultBuffer(
			ID3D12GraphicsCommandList* cmdList,
			//ComPtr<ID3D12Resource> &default_buffer,
			//ComPtr<ID3D12Resource> &upload_buffer,
			VirtualMemoryPointer &default_buffer,
			VirtualMemoryPointer &upload_buffer,
			const void* initData,
			const UINT64 BufferSize
		);
	};
	GeometryBasic::GeometryBasic()
	{
		all_vertex = 0;
		all_index = 0;
		all_index_adj = 0;
		if_buffer_created = false;
	}
	PancystarEngine::EngineFailReason GeometryBasic::Create()
	{
		PancystarEngine::EngineFailReason check_error;
		//检验是否资源已经创建
		if (if_buffer_created)
		{
			return PancystarEngine::succeed;
		}
		//注册几何体格式
		check_error = InitGeometryDesc(if_create_adj);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//注册几何体资源
		check_error = InitGeometry(
			all_vertex, 
			all_index, 
			all_index_adj,
			geometry_vertex_buffer,
			geometry_index_buffer,
			geometry_adjindex_buffer,
			geometry_vertex_buffer_view,
			geometry_index_buffer_view
		);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		if_buffer_created = true;
		return PancystarEngine::succeed;
	}
	GeometryBasic::~GeometryBasic()
	{
	}
	PancystarEngine::EngineFailReason GeometryBasic::BuildDefaultBuffer(
		ID3D12GraphicsCommandList* cmdList,
		VirtualMemoryPointer &default_buffer,
		VirtualMemoryPointer &upload_buffer,
		const void* initData,
		const UINT64 BufferSize
	) 
	{
		HRESULT hr;
		PancystarEngine::EngineFailReason check_error;
		check_error = MemoryHeapGpuControl::GetInstance()->BuildResourceCommit(
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_HEAP_FLAG_NONE,
			CD3DX12_RESOURCE_DESC::Buffer(BufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST, default_buffer
		);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		
		check_error = MemoryHeapGpuControl::GetInstance()->BuildResourceFromHeap(
			"json\\resource_heap\\heap_vertex.json",
			CD3DX12_RESOURCE_DESC::Buffer(BufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ, upload_buffer
		);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = MemoryHeapGpuControl::GetInstance()->BuildResourceCommit(
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_HEAP_FLAG_NONE,
			CD3DX12_RESOURCE_DESC::Buffer(BufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ, upload_buffer
		);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//向CPU可访问缓冲区添加数据
		D3D12_SUBRESOURCE_DATA vertexData_buffer = {};
		vertexData_buffer.pData = initData;
		vertexData_buffer.RowPitch = BufferSize;
		vertexData_buffer.SlicePitch = vertexData_buffer.RowPitch;
		//资源拷贝
		auto buffer_size = UpdateSubresources<1>(
			cmdList,
			MemoryHeapGpuControl::GetInstance()->GetMemoryResource(default_buffer)->GetResource().Get(),
			MemoryHeapGpuControl::GetInstance()->GetMemoryResource(upload_buffer)->GetResource().Get(),
			0,
			0,
			1,
			&vertexData_buffer
			);
		if (buffer_size <= 0)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "update vertex buffer data error");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build geometry data", error_message);
			return error_message;
		}
		cmdList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				MemoryHeapGpuControl::GetInstance()->GetMemoryResource(default_buffer)->GetResource().Get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_GENERIC_READ
			)
		);
		return PancystarEngine::succeed;
	}
	//基本模型几何体
	template<typename T>
	class GeometryCommonModel : public GeometryBasic
	{
		T *vertex_data;
		UINT *index_data;
		bool if_save_CPU_data;//是否保留cpu备份
		
		//模型的基本数据
		bool if_model_adj;
		uint32_t all_model_vertex;
		uint32_t all_model_index;
		
	public:
		GeometryCommonModel(
			T *vertex_data_in,
			UINT *index_data_in,
			const uint32_t &input_vert_num,
			const uint32_t &input_index_num,
			bool if_adj_in = false,
			bool if_save_cpu_data_in = false
		);
		~GeometryCommonModel();
	private:
		PancystarEngine::EngineFailReason InitGeometryDesc(
			bool &if_create_adj
		);
		PancystarEngine::EngineFailReason InitGeometry(
			uint32_t &all_vertex_need,
			uint32_t &all_index_need,
			uint32_t &all_index_adj_need,
			VirtualMemoryPointer &geometry_vertex_buffer,
			VirtualMemoryPointer &geometry_index_buffer,
			VirtualMemoryPointer &geometry_adjindex_buffer,
			D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
			D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in
		);
	};
	template<typename T>
	GeometryCommonModel<T>::GeometryCommonModel(
		T *vertex_data_in,
		UINT *index_data_in,
		const uint32_t &input_vert_num,
		const uint32_t &input_index_num,
		bool if_adj_in,
		bool if_save_cpu_data_in
	) 
	{
		//拷贝CPU数据
		vertex_data = NULL;
		index_data = NULL;
		if (input_vert_num != 0) 
		{
			all_model_vertex = input_vert_num;
			vertex_data = new T[input_vert_num];
			memcpy(vertex_data, vertex_data_in, input_vert_num * sizeof(T));
		}
		if (input_index_num != 0) 
		{
			all_model_index = input_index_num;
			index_data = new UINT[input_index_num];
			memcpy(index_data, index_data_in, input_index_num * sizeof(UINT));
		}
		if_save_CPU_data = if_save_cpu_data_in;
		if_model_adj = if_adj_in;
	}
	template<typename T>
	GeometryCommonModel<T>::~GeometryCommonModel()
	{
		if (vertex_data != NULL) 
		{
			delete[] vertex_data;
			vertex_data = NULL;

		}
		if (index_data != NULL) 
		{
			delete[] index_data;
			index_data = NULL;
		}
	}
	template<typename T>
	PancystarEngine::EngineFailReason GeometryCommonModel<T>::InitGeometryDesc(
		bool &if_create_adj
	) 
	{
		if_create_adj = if_model_adj;
		return PancystarEngine::succeed;
	}
	template<typename T>
	PancystarEngine::EngineFailReason GeometryCommonModel<T>::InitGeometry(
		uint32_t &all_vertex_need,
		uint32_t &all_index_need,
		uint32_t &all_index_adj_need,
		VirtualMemoryPointer &geometry_vertex_buffer_in,
		VirtualMemoryPointer &geometry_index_buffer_in,
		VirtualMemoryPointer &geometry_adjindex_buffer_in,
		D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
		D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in
	) 
	{
		//创建临时的上传缓冲区
		VirtualMemoryPointer geometry_vertex_buffer_upload;
		VirtualMemoryPointer geometry_index_buffer_upload;
		//获取临时的拷贝commandlist
		PancyRenderCommandList *copy_render_list;
		uint32_t copy_render_list_ID;
		auto copy_contex = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetEmptyRenderlist(NULL, D3D12_COMMAND_LIST_TYPE_DIRECT, &copy_render_list, copy_render_list_ID);
		all_vertex_need = all_model_vertex;
		all_index_need = all_model_index;
		const UINT64 VertexBufferSize = all_vertex_need * sizeof(T);
		const UINT64 IndexBufferSize = all_index_need * sizeof(UINT);
		//创建顶点缓冲区
		if (vertex_data != NULL) 
		{
			PancystarEngine::EngineFailReason check_error = BuildDefaultBuffer(copy_render_list->GetCommandList().Get(), geometry_vertex_buffer_in, geometry_vertex_buffer_upload, vertex_data, VertexBufferSize);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		//创建索引缓冲区
		if (index_data != NULL) 
		{
			PancystarEngine::EngineFailReason check_error = BuildDefaultBuffer(copy_render_list->GetCommandList().Get(), geometry_index_buffer_in, geometry_index_buffer_upload, index_data, IndexBufferSize);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		//完成渲染队列并提交拷贝
		copy_render_list->UnlockPrepare();
		ThreadPoolGPUControl::GetInstance()->GetMainContex()->SubmitRenderlist(D3D12_COMMAND_LIST_TYPE_DIRECT,1, &copy_render_list_ID);
		//等待线程同步
		ThreadPoolGPUControl::GetInstance()->GetMainContex()->WaitWorkRenderlist(copy_render_list_ID);
		//删除临时缓冲区
		MemoryHeapGpuControl::GetInstance()->FreeResource(geometry_vertex_buffer_upload);
		MemoryHeapGpuControl::GetInstance()->FreeResource(geometry_index_buffer_upload);
		//ThreadPoolGPUControl::GetInstance()->GetMainContex()->FreeAlloctor(D3D12_COMMAND_LIST_TYPE_DIRECT);
		//删除CPU备份
		if (!if_save_CPU_data) 
		{
			if (vertex_data != NULL) 
			{
				delete[] vertex_data;
				vertex_data = NULL;
			}
			if (index_data != NULL) 
			{
				delete[] index_data;
				index_data = NULL;
			}
		}
		//todo 邻接三角面计算
		if (if_model_adj) 
		{
		}
		geometry_vertex_buffer_view_in.BufferLocation = MemoryHeapGpuControl::GetInstance()->GetMemoryResource(geometry_vertex_buffer_in)->GetResource()->GetGPUVirtualAddress();
		geometry_vertex_buffer_view_in.StrideInBytes = sizeof(T);
		geometry_vertex_buffer_view_in.SizeInBytes = VertexBufferSize;
		return PancystarEngine::succeed;
	}
	class ModelResourceBasic 
	{
	};
}