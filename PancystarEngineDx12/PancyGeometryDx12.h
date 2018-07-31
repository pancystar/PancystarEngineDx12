#pragma once
#include"PancystarEngineBasicDx12.h"
#include"PancyDx12Basic.h"
namespace PancystarEngine
{
	//2D顶点格式
	struct Point2D
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 tex_color;  //用于采样的坐标
		DirectX::XMFLOAT4 tex_range;  //用于限制采样矩形的坐标
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
	//几何体的格式对接类型
	struct PancyVertexBufferDesc
	{
		size_t vertex_desc_classID;
		size_t input_element_num;
		D3D12_INPUT_ELEMENT_DESC *inputElementDescs = NULL;
	};
	//几何体格式管理器(用于注册顶点)
	class GeometryDesc
	{
		std::unordered_map<size_t, PancyVertexBufferDesc> vertex_buffer_desc_map;
	private:
		GeometryDesc();
	public:
		static GeometryDesc* GetInstance()
		{
			static GeometryDesc* this_instance;
			if (this_instance == NULL)
			{
				this_instance = new GeometryDesc();
			}
			return this_instance;
		}
		~GeometryDesc();
		void AddVertexDesc(size_t vertex_desc_classID_in, std::vector<D3D12_INPUT_ELEMENT_DESC> input_element_desc_list);
		inline const PancyVertexBufferDesc* GetVertexDesc(size_t vertex_class_id)
		{
			auto new_vertex_desc = vertex_buffer_desc_map.find(vertex_class_id);
			if (new_vertex_desc != vertex_buffer_desc_map.end())
			{
				return &new_vertex_desc->second;
			}
			return NULL;
		}
	private:
		void InitPoint2D();
		void InitPointCommon();
		void InitPointSkin();
	};
	GeometryDesc::GeometryDesc()
	{
		InitPoint2D();
		InitPointCommon();
		InitPointSkin();
	}
	void GeometryDesc::InitPoint2D()
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> new_input_element_desc_list;
		//2D顶点格式
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,  0 },
			{ "UVCOLOR",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "UVRANGE",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		uint32_t now_size = sizeof(inputElementDescs) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		for (uint32_t i = 0; i < now_size; ++i)
		{
			new_input_element_desc_list.push_back(inputElementDescs[i]);
		}
		AddVertexDesc(typeid(Point2D).hash_code(), new_input_element_desc_list);
		new_input_element_desc_list.clear();
	}
	void GeometryDesc::InitPointCommon()
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> new_input_element_desc_list;
		//2D顶点格式
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,  0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		    { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "UVID",     0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		    { "UVCOLOR",  0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		    { "UVRANGE",  0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 72, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		uint32_t now_size = sizeof(inputElementDescs) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		for (uint32_t i = 0; i < now_size; ++i)
		{
			new_input_element_desc_list.push_back(inputElementDescs[i]);
		}
		AddVertexDesc(typeid(PointCommon).hash_code(), new_input_element_desc_list);
		new_input_element_desc_list.clear();
	}
	void GeometryDesc::InitPointSkin() 
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> new_input_element_desc_list;
		//2D顶点格式
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,     0, 0,   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT,     0, 12,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",    0, DXGI_FORMAT_R32G32B32_FLOAT,     0, 24,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "UVID",       0, DXGI_FORMAT_R32G32B32A32_UINT,   0, 36,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "UVCOLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 52,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "UVRANGE",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 68,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BONEID",     0, DXGI_FORMAT_R32G32B32A32_UINT,   0, 84,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BONEWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_UINT,   0, 100, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		uint32_t now_size = sizeof(inputElementDescs) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		for (uint32_t i = 0; i < now_size; ++i)
		{
			new_input_element_desc_list.push_back(inputElementDescs[i]);
		}
		AddVertexDesc(typeid(PointSkinCommon).hash_code(), new_input_element_desc_list);
		new_input_element_desc_list.clear();
	}
	GeometryDesc::~GeometryDesc()
	{
		for (auto now_vertex_desc = vertex_buffer_desc_map.begin(); now_vertex_desc != vertex_buffer_desc_map.end(); ++now_vertex_desc) 
		{
			delete now_vertex_desc->second.inputElementDescs;
			now_vertex_desc->second.inputElementDescs = NULL;
		}
		vertex_buffer_desc_map.clear();
	}
	void GeometryDesc::AddVertexDesc(size_t vertex_desc_classID_in, std::vector<D3D12_INPUT_ELEMENT_DESC> input_element_desc_list)
	{
		PancyVertexBufferDesc new_vertex_desc;
		new_vertex_desc.vertex_desc_classID = vertex_desc_classID_in;
		new_vertex_desc.input_element_num = input_element_desc_list.size();
		new_vertex_desc.inputElementDescs = new D3D12_INPUT_ELEMENT_DESC[input_element_desc_list.size()];
		uint32_t count_element = 0;
		for (auto now_element = input_element_desc_list.begin(); now_element != input_element_desc_list.end(); ++now_element) 
		{
			new_vertex_desc.inputElementDescs[count_element] = *now_element;
			count_element += 1;
		}
		vertex_buffer_desc_map.insert(std::pair<size_t, PancyVertexBufferDesc>(vertex_desc_classID_in, new_vertex_desc));
	}
	
	//几何体基础类型
	class GeometryBasic
	{
	protected:
		//几何体的渲染buffer
		ComPtr<ID3D12Resource> geometry_vertex_buffer;
		ComPtr<ID3D12Resource> geometry_index_buffer;
		ComPtr<ID3D12Resource> geometry_adjindex_buffer;
		D3D12_VERTEX_BUFFER_VIEW geometry_vertex_buffer_view;
		D3D12_INDEX_BUFFER_VIEW geometry_index_buffer_view;
		uint32_t all_vertex;
		uint32_t all_index;
		uint32_t all_index_adj;
		//输入格式
		size_t vertex_desc_info_hash;
		//几何体的创建信息
		bool if_create_adj;
		bool if_buffer_created;
	public:
		GeometryBasic();
		PancystarEngine::EngineFailReason Create();
		virtual ~GeometryBasic();
		inline ComPtr<ID3D12Resource> GetVertexBuffer() 
		{
			return geometry_vertex_buffer;
		};
		inline ComPtr<ID3D12Resource> GetIndexBuffer()
		{
			return geometry_index_buffer;
		};
		inline ComPtr<ID3D12Resource> GetIndexAdjBuffer()
		{
			return geometry_adjindex_buffer;
		};
	protected:
		virtual PancystarEngine::EngineFailReason InitGeometryDesc(
			size_t &vertex_class_hash, 
			bool &if_create_adj
		) = 0;
		virtual PancystarEngine::EngineFailReason InitGeometry(
			uint32_t &all_vertex_need, 
			uint32_t &all_index_need, 
			uint32_t &all_index_adj_need,
			ComPtr<ID3D12Resource> &geometry_vertex_buffer,
			ComPtr<ID3D12Resource> &geometry_index_buffer,
			ComPtr<ID3D12Resource> &geometry_adjindex_buffer,
			D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
			D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in
		) = 0;
		PancystarEngine::EngineFailReason BuildDefaultBuffer(
			ID3D12GraphicsCommandList* cmdList,
			ComPtr<ID3D12Resource> &default_buffer,
			ComPtr<ID3D12Resource> &upload_buffer,
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
		check_error = InitGeometryDesc(vertex_desc_info_hash, if_create_adj);
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
		ComPtr<ID3D12Resource> &default_buffer,
		ComPtr<ID3D12Resource> &upload_buffer,
		const void* initData,
		const UINT64 BufferSize
	) 
	{
		HRESULT hr;
		//创建一个仅由GPU访问的缓冲区
		hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(BufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&default_buffer)
		);
		if (FAILED(hr))
		{
			PancystarEngine::EngineFailReason error_message(hr, "create default vertex buffer error");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build geometry data", error_message);
			return error_message;
		}
		//创建一个由CPU可访问的缓冲区
		hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(BufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&upload_buffer)
		);
		if (FAILED(hr))
		{
			PancystarEngine::EngineFailReason error_message(hr, "create upload vertex buffer error");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build geometry data", error_message);
			return error_message;
		}
		//向CPU可访问缓冲区添加数据
		D3D12_SUBRESOURCE_DATA vertexData_buffer = {};
		vertexData_buffer.pData = initData;
		vertexData_buffer.RowPitch = BufferSize;
		vertexData_buffer.SlicePitch = vertexData_buffer.RowPitch;
		//资源拷贝
		auto buffer_size = UpdateSubresources<1>(
			cmdList,
			default_buffer.Get(),
			upload_buffer.Get(),
			0,
			0,
			1,
			&vertexData_buffer
			);
		if (buffer_size <= 0)
		{
			PancystarEngine::EngineFailReason error_message(hr, "update vertex buffer data error");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build geometry data", error_message);
			return error_message;
		}
		cmdList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				default_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
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
			size_t &vertex_class_hash,
			bool &if_create_adj
		);
		PancystarEngine::EngineFailReason InitGeometry(
			uint32_t &all_vertex_need,
			uint32_t &all_index_need,
			uint32_t &all_index_adj_need,
			ComPtr<ID3D12Resource> &geometry_vertex_buffer,
			ComPtr<ID3D12Resource> &geometry_index_buffer,
			ComPtr<ID3D12Resource> &geometry_adjindex_buffer,
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
		size_t &vertex_class_hash,
		bool &if_create_adj
	) 
	{
		if_create_adj = if_model_adj;
		vertex_class_hash = typeid(T).hash_code();
		std::string vertex_type_name = typeid(T).name();
		auto vertex_desc = GeometryDesc::GetInstance()->GetVertexDesc(vertex_class_hash);
		if (vertex_desc == NULL) 
		{
			//指定的顶点格式尚未注册,无法根据指定的顶点格式创建几何体
			PancystarEngine::EngineFailReason error_message(E_FAIL,"The vertex type: " + vertex_type_name + " haven't init to vertex desc controler");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("load model data error", error_message);
			return error_message;
		}
		return PancystarEngine::succeed;
	}
	template<typename T>
	PancystarEngine::EngineFailReason GeometryCommonModel<T>::InitGeometry(
		uint32_t &all_vertex_need,
		uint32_t &all_index_need,
		uint32_t &all_index_adj_need,
		ComPtr<ID3D12Resource> &geometry_vertex_buffer_in,
		ComPtr<ID3D12Resource> &geometry_index_buffer_in,
		ComPtr<ID3D12Resource> &geometry_adjindex_buffer_in,
		D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
		D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in
	) 
	{
		HRESULT hr;
		//创建临时的上传缓冲区
		ComPtr<ID3D12Resource> geometry_vertex_buffer_upload;
		ComPtr<ID3D12Resource> geometry_index_buffer_upload;
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
		ThreadPoolGPUControl::GetInstance()->GetMainContex()->FreeAlloctor(D3D12_COMMAND_LIST_TYPE_DIRECT);
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

		return PancystarEngine::succeed;
	}
}