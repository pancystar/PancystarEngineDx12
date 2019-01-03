#pragma once
#include"PancystarEngineBasicDx12.h"
#include"PancyDx12Basic.h"
#include"PancyMemoryBasic.h"
#include"PancyThreadBasic.h"
#define IndexType uint32_t
namespace PancystarEngine
{
	//纯顶点格式(用于网格显示)
	struct PointPositionSingle
	{
		DirectX::XMFLOAT4 position;
	};
	//2D顶点格式
	struct Point2D
	{
		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4 tex_color;  //用于采样的坐标
		//DirectX::XMFLOAT4 tex_range;  //用于限制采样矩形的坐标
	};
	struct PointUI
	{
		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4 tex_color;  //用于采样的坐标
		DirectX::XMUINT4  tex_id;     //UI的ID号
	};
	//标准3D顶点格式
	struct PointCommon
	{
		DirectX::XMFLOAT3 position;   //位置
		DirectX::XMFLOAT3 normal;     //法线
		DirectX::XMFLOAT3 tangent;    //切线
		DirectX::XMUINT4  tex_id;     //使用的纹理ID号
		DirectX::XMFLOAT4 tex_uv;     //用于采样的坐标
	};
	//带骨骼的顶点格式
	struct PointSkinCommon
	{
		DirectX::XMFLOAT3 position;   //位置
		DirectX::XMFLOAT3 normal;     //法线
		DirectX::XMFLOAT3 tangent;    //切线
		DirectX::XMUINT4  tex_id;     //使用的纹理ID号
		DirectX::XMFLOAT4 tex_uv;     //用于采样的坐标
		DirectX::XMUINT4  bone_id;    //骨骼ID号
		DirectX::XMFLOAT4 bone_weight;//骨骼权重
	};
	
	
	//几何体基础类型
	class GeometryBasic
	{
	protected:
		bool if_build_finish;//是否已经上传完毕
		PancyFenceIdGPU upload_fence_value;
		//几何体的渲染buffer
		SubMemoryPointer geometry_vertex_buffer;
		SubMemoryPointer geometry_index_buffer;
		SubMemoryPointer geometry_adjindex_buffer;
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
			int64_t res_size;
			return SubresourceControl::GetInstance()->GetResourceData(geometry_vertex_buffer, res_size)->GetResource();
		};
		inline ComPtr<ID3D12Resource> GetIndexBuffer()
		{
			int64_t res_size;
			return SubresourceControl::GetInstance()->GetResourceData(geometry_index_buffer, res_size)->GetResource();
		};
		inline ComPtr<ID3D12Resource> GetIndexAdjBuffer()
		{
			int64_t res_size;
			return SubresourceControl::GetInstance()->GetResourceData(geometry_adjindex_buffer, res_size)->GetResource();
		};
		inline uint32_t GetVetexNum()
		{
			return all_vertex;
		}
		inline uint32_t GetIndexNum()
		{
			return all_index;
		}
		inline D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView()
		{
			return geometry_vertex_buffer_view;
		};
		inline D3D12_INDEX_BUFFER_VIEW GetIndexBufferView()
		{
			return geometry_index_buffer_view;
		};
		inline bool CheckIfCopyFinish() 
		{
			if (!if_build_finish) 
			{
				if (ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_DIRECT)->CheckGpuBrokenFence(upload_fence_value))
				{
					if_build_finish = true;
				}
			}
			return if_build_finish;
		}
		inline void WaitCopyFinish() 
		{
			if (!if_build_finish)
			{
				ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_DIRECT)->WaitGpuBrokenFence(upload_fence_value);
				if_build_finish = true;
			}
		}
	protected:
		virtual PancystarEngine::EngineFailReason InitGeometryDesc(
			bool &if_create_adj
		) = 0;
		virtual PancystarEngine::EngineFailReason InitGeometry(
			uint32_t &all_vertex_need, 
			uint32_t &all_index_need, 
			uint32_t &all_index_adj_need,
			SubMemoryPointer &geometry_vertex_buffer,
			SubMemoryPointer &geometry_index_buffer,
			SubMemoryPointer &geometry_adjindex_buffer,
			D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
			D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in
		) = 0;
		PancystarEngine::EngineFailReason BuildDefaultBuffer(
			ID3D12GraphicsCommandList* cmdList,
			int64_t memory_alignment_size,
			int64_t memory_block_alignment_size,
			SubMemoryPointer &default_buffer,
			SubMemoryPointer &upload_buffer,
			const void* initData,
			const UINT BufferSize,
			D3D12_RESOURCE_STATES buffer_type
		);
	};
	
	//基本模型几何体
	template<typename T>
	class GeometryCommonModel : public GeometryBasic
	{
		T *vertex_data;
		IndexType *index_data;
		bool if_save_CPU_data;//是否保留cpu备份
		
		//模型的基本数据
		bool if_model_adj;
		uint32_t all_model_vertex;
		uint32_t all_model_index;
		
	public:
		GeometryCommonModel(
			const T *vertex_data_in,
			const IndexType *index_data_in,
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
			SubMemoryPointer &geometry_vertex_buffer,
			SubMemoryPointer &geometry_index_buffer,
			SubMemoryPointer &geometry_adjindex_buffer,
			D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
			D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in
		);
	};
	template<typename T>
	GeometryCommonModel<T>::GeometryCommonModel(
		const T *vertex_data_in,
		const IndexType *index_data_in,
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
			index_data = new IndexType[input_index_num];
			memcpy(index_data, index_data_in, input_index_num * sizeof(IndexType));
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
		SubMemoryPointer &geometry_vertex_buffer_in,
		SubMemoryPointer &geometry_index_buffer_in,
		SubMemoryPointer &geometry_adjindex_buffer_in,
		D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
		D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in
	) 
	{
		//创建临时的上传缓冲区
		SubMemoryPointer geometry_vertex_buffer_upload;
		SubMemoryPointer geometry_index_buffer_upload;
		//获取临时的拷贝commandlist
		PancyRenderCommandList *copy_render_list;
		uint32_t copy_render_list_ID;
		
		auto copy_contex = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(NULL, &copy_render_list, copy_render_list_ID);
		all_vertex_need = all_model_vertex;
		all_index_need = all_model_index;
		const UINT VertexBufferSize = all_vertex_need * sizeof(T);
		UINT IndexBufferSize;
		if (sizeof(IndexType) == sizeof(UINT)) 
		{
			IndexBufferSize = all_index_need * sizeof(UINT);
		}
		else if (sizeof(IndexType) == sizeof(uint16_t))
		{
			IndexBufferSize = all_index_need * sizeof(uint16_t);
		}
		else 
		{
			IndexBufferSize = 0;
			PancystarEngine::EngineFailReason check_error(E_FAIL,"unsurpported index buffer type: "+std::to_string(sizeof(IndexType)));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build geometry data ", check_error);
			return check_error;
		}
		
		//创建顶点缓冲区
		if (vertex_data != NULL) 
		{
			PancystarEngine::EngineFailReason check_error = BuildDefaultBuffer(copy_render_list->GetCommandList().Get(), 4194304, 131072, geometry_vertex_buffer_in, geometry_vertex_buffer_upload, vertex_data, VertexBufferSize, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		//创建索引缓冲区
		if (index_data != NULL) 
		{
			PancystarEngine::EngineFailReason check_error = BuildDefaultBuffer(copy_render_list->GetCommandList().Get(),1048576,16384, geometry_index_buffer_in, geometry_index_buffer_upload, index_data, IndexBufferSize, D3D12_RESOURCE_STATE_INDEX_BUFFER);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		//完成渲染队列并提交拷贝
		copy_render_list->UnlockPrepare();
		
		ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(1, &copy_render_list_ID);
		//在GPU上插一个监控眼位
		ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_DIRECT)->SetGpuBrokenFence(upload_fence_value);
		//ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_DIRECT)->WaitGpuBrokenFence(upload_fence_value);
		//等待拷贝结束，todo::创建资源不再等待
		WaitCopyFinish();
		//删除临时缓冲区
		SubresourceControl::GetInstance()->FreeSubResource(geometry_vertex_buffer_upload);
		SubresourceControl::GetInstance()->FreeSubResource(geometry_index_buffer_upload);
		copy_render_list->UnlockPrepare();
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
		int64_t res_size;
		//创建顶点缓存视图
		auto res_vert_data = SubresourceControl::GetInstance()->GetResourceData(geometry_vertex_buffer_in, res_size)->GetResource();
		geometry_vertex_buffer_view_in.BufferLocation = res_vert_data->GetGPUVirtualAddress() + geometry_vertex_buffer_in.offset * res_size;
		geometry_vertex_buffer_view_in.StrideInBytes = sizeof(T);
		geometry_vertex_buffer_view_in.SizeInBytes = res_size;
		//创建索引缓存视图
		
		auto res_index_data = SubresourceControl::GetInstance()->GetResourceData(geometry_index_buffer_in, res_size)->GetResource();
		geometry_index_buffer_view_in.BufferLocation = res_index_data->GetGPUVirtualAddress() + geometry_index_buffer_in.offset * res_size;
		geometry_index_buffer_view_in.SizeInBytes = res_size;
		if (sizeof(IndexType) == sizeof(UINT))
		{
			geometry_index_buffer_view_in.Format = DXGI_FORMAT_R32_UINT;
		}
		else if (sizeof(IndexType) == sizeof(uint16_t))
		{
			geometry_index_buffer_view_in.Format = DXGI_FORMAT_R16_UINT;
		}
		return PancystarEngine::succeed;
	}
	class ModelResourceBasic 
	{
	};
}