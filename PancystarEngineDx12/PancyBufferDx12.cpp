#include"PancyBufferDx12.h"
using namespace PancystarEngine;
static PancyBasicBufferControl* this_instance = NULL;
//基础缓冲区
PancyBasicBuffer::PancyBasicBuffer(const std::string &resource_name_in, Json::Value root_value_in) :PancyBasicVirtualResource(resource_name_in, root_value_in)
{
	upload_buffer_id = -1;
	WaitFence = -1;
}
PancystarEngine::EngineFailReason PancyBasicBuffer::InitResource(const Json::Value &root_value, const std::string &resource_name, ResourceStateType &now_res_state)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_json_value rec_value;
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, root_value, "BufferType", pancy_json_data_type::json_data_enum, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	buffer_type = static_cast<PancyBufferType>(rec_value.int_value);

	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, root_value, "SubResourceFile", pancy_json_data_type::json_data_string, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = SubresourceControl::GetInstance()->BuildSubresourceFromFile(rec_value.string_value, buffer_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	now_res_state = ResourceStateType::resource_state_load_GPU_memory_finish;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicBuffer::UpdateResourceToGPU(
	ResourceStateType &now_res_state,
	void* resource,
	const pancy_resource_size &resource_size_in,
	const pancy_resource_size &resource_offset_in
)
{
	//先检查当前的资源是否处于GPU空闲状态

	PancystarEngine::EngineFailReason check_error;
	if (buffer_type == Buffer_ShaderResource_dynamic || buffer_type == Buffer_Constant) 
	{
		//动态缓冲区,直接拷贝
		check_error = SubresourceControl::GetInstance()->WriteFromCpuToBuffer(buffer_data, resource_offset_in, resource, resource_size_in);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		now_res_state = ResourceStateType::resource_state_load_GPU_memory_finish;
	}
	else if (buffer_type == Buffer_ShaderResource_static || buffer_type == Buffer_Vertex || buffer_type == Buffer_Index)
	{
		//静态缓冲区,需要借助额外的动态缓冲区进行拷贝
		std::string subresource_name;
		//先创建一个临时的动态缓冲区
		check_error = PancyBasicBufferControl::GetInstance()->BuildBufferTypeJson(Buffer_ShaderResource_dynamic, resource_size_in, subresource_name);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		//填充资源格式
		Json::Value json_root;
		PancyJsonTool::GetInstance()->SetJsonValue(json_root, "BufferType", "Buffer_ShaderResource_dynamic");
		PancyJsonTool::GetInstance()->SetJsonValue(json_root, "SubResourceFile", subresource_name);
		//创建临时缓冲区资源
		check_error = PancyBasicBufferControl::GetInstance()->LoadResource("Dynamic_buffer", json_root, upload_buffer_id,true);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//将需要拷贝的资源先拷贝到临时缓冲区
		check_error = PancyBasicBufferControl::GetInstance()->CopyCpuResourceToGpu(upload_buffer_id, resource, resource_size_in, 0);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//获取用于拷贝的commond list
		PancyRenderCommandList *copy_render_list;
		uint32_t copy_render_list_ID;
		check_error = ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->GetEmptyRenderlist(NULL, &copy_render_list, copy_render_list_ID);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//获取拷贝资源
		SubMemoryPointer upload_submemory_pointer;
		ResourceStateType copy_resource_state = ResourceStateType::resource_state_not_init;
		check_error = PancyBasicBufferControl::GetInstance()->GetBufferSubResource(upload_buffer_id, upload_submemory_pointer);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = PancyBasicBufferControl::GetInstance()->GetResourceState(upload_buffer_id, copy_resource_state);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		if (copy_resource_state != ResourceStateType::resource_state_load_GPU_memory_finish) 
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL,"The upload memory haven't init success");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Upload Buffer resource From Cpu To GPU",error_message);
			return error_message;
		}
		//组织资源拷贝命令
		check_error = SubresourceControl::GetInstance()->CopyResource(copy_render_list, upload_submemory_pointer, buffer_data,0, resource_offset_in, resource_size_in);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		copy_render_list->UnlockPrepare();
		//提交渲染命令
		ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->SubmitRenderlist(1, &copy_render_list_ID);
		//预测等待眼位
		WaitFence = ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->GetNextBrokenFence();
		now_res_state = ResourceStateType::resource_state_load_CPU_memory_finish;
	}
	return PancystarEngine::succeed;
}
void PancyBasicBuffer::CheckIfResourceLoadToGpu(ResourceStateType &now_res_state)
{
	if (now_res_state == ResourceStateType::resource_state_load_CPU_memory_finish) 
	{
		//资源加载进入CPU但是未完全载入GPU，检测资源是否已经加载完毕
		bool if_GPU_finished = ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->CheckGpuBrokenFence(WaitFence);
		if (if_GPU_finished) 
		{
			//资源加载完毕
			now_res_state = ResourceStateType::resource_state_load_GPU_memory_finish;
			//删除临时的缓冲区
			auto check_error = PancyBasicBufferControl::GetInstance()->DeleteResurceReference(upload_buffer_id);
			WaitFence = -1;
			upload_buffer_id = -1;
		}
	}
}
PancyBasicBuffer::~PancyBasicBuffer() 
{
	SubresourceControl::GetInstance()->FreeSubResource(buffer_data);
}
//缓冲区管理器
PancyBasicBufferControl::PancyBasicBufferControl(const std::string &resource_type_name_in) : PancyBasicResourceControl(resource_type_name_in)
{
	PancyJsonTool::GetInstance()->SetGlobelVraiable("Buffer_ShaderResource_static", static_cast<int32_t>(Buffer_ShaderResource_static), typeid(Buffer_ShaderResource_static).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("Buffer_ShaderResource_dynamic", static_cast<int32_t>(Buffer_ShaderResource_dynamic), typeid(Buffer_ShaderResource_dynamic).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("Buffer_Constant", static_cast<int32_t>(Buffer_Constant), typeid(Buffer_Constant).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("Buffer_Vertex", static_cast<int32_t>(Buffer_Vertex), typeid(Buffer_Vertex).name());
	PancyJsonTool::GetInstance()->SetGlobelVraiable("Buffer_Index", static_cast<int32_t>(Buffer_Index), typeid(Buffer_Index).name());
}
PancystarEngine::EngineFailReason PancyBasicBufferControl::BuildResource(
	const Json::Value &root_value,
	const std::string &name_resource_in,
	PancyBasicVirtualResource** resource_out)
{
	*resource_out = new PancyBasicBuffer(name_resource_in, root_value);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicBufferControl::BuildBufferTypeJson(
	const PancyBufferType &buffer_type,
	const pancy_resource_size &data_size,
	std::string &subresource_desc_name
) 
{
	D3D12_HEAP_TYPE heap_type;                  //资源堆的类型
	D3D12_RESOURCE_STATES resource_create_state;//资源创建时的状态
	pancy_resource_size subresources_size = data_size;
	pancy_resource_size subresource_alize_size = 0;//缓冲区所使用资源对齐的内存大小
	pancy_resource_size heap_alize_size = 0;//缓冲区所在的堆资源对齐的内存大小
	pancy_resource_size all_heap_size = 0;
	pancy_object_id memory_num_per_heap = 0;//堆所开辟的内存块数量
	//确定缓冲区的对齐大小
	if (buffer_type == Buffer_Constant) 
	{
		//每个开辟出的堆内存的总大小
		all_heap_size = BufferHeapAliaze4M;
		//计算常量缓冲区对齐大小
		if (subresources_size > 65536)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL,"the constant buffer size need less than 64K,Ask: " + std::to_string(subresources_size));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build constant buffer", error_message);
			return error_message;
		}
		else if (subresources_size > ConstantBufferSubResourceAliaze256K*4)
		{
			heap_alize_size = static_cast<pancy_resource_size>(ConstantBufferHeapAliaze256K);
			subresource_alize_size = static_cast<pancy_resource_size>(ConstantBufferSubResourceAliaze256K);
		}
		else 
		{
			heap_alize_size = static_cast<pancy_resource_size>(ConstantBufferHeapAliaze64K);
			subresource_alize_size = static_cast<pancy_resource_size>(ConstantBufferSubResourceAliaze64K);
		}
		if (subresources_size % subresource_alize_size != 0)
		{
			subresources_size = ((subresources_size / subresource_alize_size) + 1) * subresource_alize_size;
		}
	}
	else if (buffer_type == Buffer_ShaderResource_dynamic)
	{
		//根据当前资源的大小，决定使用哪种对齐方式
		if (subresources_size > BufferHeapAliaze4M)
		{
			memory_num_per_heap = 1;
			//大于4M的内存直接开辟，1M对齐
			if (subresources_size % 1048576 != 0)
			{
				subresources_size = ((subresources_size / 1048576) + 1) * 1048576;
			}
			//限定开辟的数据，单堆，单数据
			heap_alize_size = subresources_size;
			subresource_alize_size = subresources_size;
		}
		else
		{
			if (subresources_size > 1048576)
			{
				//1M-4M的内存
				heap_alize_size = static_cast<pancy_resource_size>(BufferHeapAliaze16M);
				subresource_alize_size = static_cast<pancy_resource_size>(BufferSubResourceAliaze16M);
			}
			else if (subresources_size > BufferSubResourceAliaze4M)
			{
				//128k-1M的内存
				heap_alize_size = static_cast<pancy_resource_size>(BufferHeapAliaze4M);
				subresource_alize_size = static_cast<pancy_resource_size>(BufferSubResourceAliaze4M);
			}
			else
			{
				//0-128K的内存
				heap_alize_size = static_cast<pancy_resource_size>(BufferHeapAliaze1M);
				subresource_alize_size = static_cast<pancy_resource_size>(BufferSubResourceAliaze1M);
			}
		}
		if (subresources_size % subresource_alize_size != 0)
		{
			subresources_size = ((subresources_size / subresource_alize_size) + 1) * subresource_alize_size;
		}
		all_heap_size = MaxWasteSpace;
	}
	else if (buffer_type == Buffer_ShaderResource_static || buffer_type == Buffer_Vertex || buffer_type == Buffer_Index) 
	{
		all_heap_size = 0;
		//根据当前资源的大小，决定使用哪种对齐方式
		if (subresources_size > BufferHeapAliaze4M)
		{
			//大于4M的内存直接开辟，1M对齐
			if (subresources_size % 1048576 != 0)
			{
				subresources_size = ((subresources_size / 1048576) + 1) * 1048576;
			}
			//限定开辟的数据，单堆，单数据
			heap_alize_size = subresources_size;
			subresource_alize_size = subresources_size;
		}
		else
		{
			if (subresources_size > 1048576)
			{
				//1M-4M的内存
				all_heap_size = static_cast<pancy_resource_size>(BufferHeapAliaze16M);
				subresource_alize_size = static_cast<pancy_resource_size>(BufferSubResourceAliaze16M);
			}
			else if (subresources_size > BufferSubResourceAliaze4M)
			{
				//128k-1M的内存
				all_heap_size = static_cast<pancy_resource_size>(BufferHeapAliaze4M);
				subresource_alize_size = static_cast<pancy_resource_size>(BufferSubResourceAliaze4M);
			}
			else
			{
				//0-128K的内存
				all_heap_size = static_cast<pancy_resource_size>(BufferHeapAliaze1M);
				subresource_alize_size = static_cast<pancy_resource_size>(BufferSubResourceAliaze1M);
			}
		}
		//根据每块数据区的对齐大小重改数据区的请求大小
		if (subresources_size % subresource_alize_size != 0)
		{
			subresources_size = ((subresources_size / subresource_alize_size) + 1) * subresource_alize_size;
		}
		heap_alize_size = subresources_size;
		if (all_heap_size == 0) 
		{
			all_heap_size = heap_alize_size;
		}
	}
	//计算存储堆和存储单元的名称
	std::string bufferblock_file_name = "";
	std::string heap_name = "";
	if (buffer_type == Buffer_ShaderResource_static || buffer_type == Buffer_Vertex || buffer_type == Buffer_Index)
	{
		resource_create_state = D3D12_RESOURCE_STATE_COMMON;
		heap_type = D3D12_HEAP_TYPE_DEFAULT;
		//静态缓冲区
		bufferblock_file_name = "json\\resource_view\\StaticBufferSub_";
		heap_name = "json\\resource_heap\\StaticBuffer_";
	}
	else if (buffer_type == Buffer_ShaderResource_dynamic || buffer_type == Buffer_Constant)
	{
		resource_create_state = D3D12_RESOURCE_STATE_GENERIC_READ;
		heap_type = D3D12_HEAP_TYPE_UPLOAD;
		//动态缓冲区
		bufferblock_file_name = "json\\resource_view\\DynamicBufferSub_";
		heap_name = "json\\resource_heap\\DynamicBuffer_";
	}
	heap_name += std::to_string(heap_alize_size);
	bufferblock_file_name += std::to_string(subresources_size);
	heap_name += ".json";
	bufferblock_file_name += ".json";
	//检查并创建资源存储堆
	if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(heap_name))
	{
		//文件未创建，创建文件
		Json::Value json_data_out;
		//填充资源格式
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "heap_size", all_heap_size);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "per_block_size", heap_alize_size);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "heap_type_in", PancyJsonTool::GetInstance()->GetEnumName(typeid(heap_type).name(), heap_type));
		std::vector<D3D12_HEAP_FLAGS> heap_flags;
		heap_flags.push_back(D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
		for (int i = 0; i < heap_flags.size(); ++i)
		{
			PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_out, "heap_flag_in", PancyJsonTool::GetInstance()->GetEnumName(typeid(heap_flags[i]).name(), heap_flags[i]));
		}
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, heap_name);
		//将文件标记为已经创建
		FileBuildRepeatCheck::GetInstance()->AddFileName(heap_name);
	}
	else
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "repeat load json file: " + heap_name, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build new buffer resource desc", error_message);
	}
	if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(bufferblock_file_name))
	{
		//更新格式文件
		Json::Value json_data_resourceview;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "ResourceType", heap_name);
		Json::Value json_data_res_desc;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Dimension", "D3D12_RESOURCE_DIMENSION_BUFFER");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Alignment", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Width", heap_alize_size);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Height", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "DepthOrArraySize", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "MipLevels", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Format", "DXGI_FORMAT_UNKNOWN");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Layout", "D3D12_TEXTURE_LAYOUT_ROW_MAJOR");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Flags", "D3D12_RESOURCE_FLAG_NONE");
		Json::Value json_data_sample_desc;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_sample_desc, "Count", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_sample_desc, "Quality", 0);
		//递归回调
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "SampleDesc", json_data_sample_desc);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_DESC", json_data_res_desc);
		//继续填充主干
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_STATES", PancyJsonTool::GetInstance()->GetEnumName(typeid(resource_create_state).name(), resource_create_state));
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "per_block_size", subresources_size);
		//写入文件并标记为已创建
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_resourceview, bufferblock_file_name);
		FileBuildRepeatCheck::GetInstance()->AddFileName(bufferblock_file_name);
	}
	else
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "repeat load json file: " + bufferblock_file_name, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build new buffer resource desc", error_message);
	}
	subresource_desc_name = bufferblock_file_name;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicBufferControl::WriteFromCpuToBuffer(
	const pancy_object_id  &resource_id,
	const pancy_resource_size &pointer_offset,
	std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
	UINT64* pRowSizesInBytes,
	UINT* pNumRows
) 
{
	PancystarEngine::EngineFailReason check_error;
	auto resource_data = GetResource(resource_id);
	if (resource_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource, check log for detail");
		return error_message;
	}
	PancyBasicBuffer *real_data_pointer = dynamic_cast<PancyBasicBuffer*>(resource_data);
	SubMemoryPointer now_pointer = real_data_pointer->GetBufferSubResource();
	check_error = SubresourceControl::GetInstance()->WriteFromCpuToBuffer(now_pointer, pointer_offset, subresources, pLayouts, pRowSizesInBytes, pNumRows);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicBufferControl::GetBufferSubResource(const pancy_object_id  &resource_id, SubMemoryPointer &submemory)
{
	auto resource_data = GetResource(resource_id);
	if (resource_data == NULL) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource, check log for detail");
		return error_message;
	}
	PancyBasicBuffer *real_data_pointer = dynamic_cast<PancyBasicBuffer*>(resource_data);
	submemory = real_data_pointer->GetBufferSubResource();
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicBufferControl::GetBufferCPUPointer(const pancy_object_id  &resource_id, UINT8** map_pointer_out)
{
	PancystarEngine::EngineFailReason check_error;
	auto resource_data = GetResource(resource_id);
	if (resource_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource, check log for detail");
		return error_message;
	}
	PancyBasicBuffer *real_data_pointer = dynamic_cast<PancyBasicBuffer*>(resource_data);
	SubMemoryPointer submemory = real_data_pointer->GetBufferSubResource();
	check_error = SubresourceControl::GetInstance()->GetBufferCpuPointer(submemory, map_pointer_out);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
//常量缓冲区
PancyConstantBuffer::PancyConstantBuffer(const std::string &cbuffer_name_in, const std::string &cbuffer_effect_name_in)
{
	cbuffer_name = cbuffer_name_in;
	cbuffer_effect_name = cbuffer_effect_name_in;
	cbuffer_size = 0;
	map_pointer_out = NULL;
}
PancystarEngine::EngineFailReason PancyConstantBuffer::SetMatrix(const std::string &variable, const DirectX::XMFLOAT4X4 &mat_data, const pancy_resource_size &offset)
{
	auto start_pos = member_variable.find(variable);
	if (start_pos == member_variable.end())
	{
		return ErrorVariableNotFind(variable);
	}
	//传入Cbuffer中的矩阵需要做转置操作
	DirectX::XMFLOAT4X4 transpose_mat;
	DirectX::XMStoreFloat4x4(&transpose_mat, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mat_data)));
	memcpy(map_pointer_out + start_pos->second.start_offset + offset * sizeof(DirectX::XMFLOAT4X4), &transpose_mat, sizeof(transpose_mat));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyConstantBuffer::SetFloat4(const std::string &variable, const DirectX::XMFLOAT4 &vector_data, const pancy_resource_size &offset)
{
	auto start_pos = member_variable.find(variable);
	if (start_pos == member_variable.end())
	{
		return ErrorVariableNotFind(variable);
	}
	memcpy(map_pointer_out + start_pos->second.start_offset + offset * sizeof(DirectX::XMFLOAT4), &vector_data, sizeof(vector_data));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyConstantBuffer::SetUint4(const std::string &variable, const DirectX::XMUINT4 &vector_data, const pancy_resource_size &offset)
{
	auto start_pos = member_variable.find(variable);
	if (start_pos == member_variable.end())
	{
		return ErrorVariableNotFind(variable);
	}
	memcpy(map_pointer_out + start_pos->second.start_offset + offset * sizeof(DirectX::XMUINT4), &vector_data, sizeof(vector_data));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyConstantBuffer::SetStruct(const std::string &variable, const void* struct_data, const pancy_resource_size &data_size, const pancy_resource_size &offset)
{
	auto start_pos = member_variable.find(variable);
	if (start_pos == member_variable.end())
	{
		return ErrorVariableNotFind(variable);
	}
	memcpy(map_pointer_out + start_pos->second.start_offset + offset * data_size, struct_data, data_size);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyConstantBuffer::ErrorVariableNotFind(const std::string &variable_name)
{
	PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find shader variable:" + variable_name + " in Cbuffer: " + cbuffer_name + " PSO:" + cbuffer_effect_name);
	PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer Variable", error_message);
	return error_message;
}
PancystarEngine::EngineFailReason PancyConstantBuffer::Create()
{
	std::string file_name = "json\\buffer\\" + cbuffer_effect_name + "_" + cbuffer_name+".json";
	PancystarEngine::EngineFailReason check_error;
	Json::Value root_value;
	check_error = PancyJsonTool::GetInstance()->LoadJsonFile(file_name, root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = GetCbufferDesc(file_name,root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancyBasicBufferControl::GetInstance()->LoadResource(file_name, buffer_ID,true);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	//从buffer中获取直接指向CPU资源的指针，保证更新资源不需要访问虚拟资源管理器
	PancyBasicBufferControl::GetInstance()->GetBufferCPUPointer(buffer_ID, &map_pointer_out);
	return PancystarEngine::succeed;
}
/*
PancystarEngine::EngineFailReason PancyConstantBuffer::Create(const std::string &hash_name,const Json::Value &root_value)
{
	PancystarEngine::EngineFailReason check_error;
	check_error = GetCbufferDesc(hash_name,root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancyBasicBufferControl::GetInstance()->LoadResource(hash_name, root_value, buffer_ID, true);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
*/
PancystarEngine::EngineFailReason PancyConstantBuffer::GetCbufferDesc(const std::string &file_name,const Json::Value &root_value)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_json_value now_value;
	Json::Value value_cbuffer_desc = root_value.get("CbufferDesc", Json::Value::null);
	if (value_cbuffer_desc == Json::Value::null)
	{
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable value_cbuffer_desc");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Create cbuffer :" + file_name + " error", error_mesage);
		return error_mesage;
	}
	//读取缓冲区大小
	check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_cbuffer_desc, "BufferSize", pancy_json_data_type::json_data_int, now_value);
	cbuffer_size = static_cast<pancy_resource_size>(now_value.int_value);
	//读取常量缓冲区的所有变量
	Json::Value value_cbuffer_member = value_cbuffer_desc.get("VariableMember", Json::Value::null);
	if (value_cbuffer_member == Json::Value::null)
	{
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable value_cbuffer_member");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Create cbuffer :" + file_name + " error", error_mesage);
		return error_mesage;
	}
	for (int32_t i = 0; i < value_cbuffer_member.size(); ++i) 
	{
		CbufferVariable new_variable_data;
		std::string variable_name;
		Json::Value value_cbuffer_variable;
		value_cbuffer_variable = value_cbuffer_member.get(i, Json::nullValue);
		//读取变量名称
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_cbuffer_variable, "Name", pancy_json_data_type::json_data_string, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		variable_name = now_value.string_value;
		//读取变量大小
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_cbuffer_variable, "Size", pancy_json_data_type::json_data_int, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_variable_data.variable_size = now_value.int_value;
		//读取变量偏移
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_cbuffer_variable, "StartOffset", pancy_json_data_type::json_data_int, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_variable_data.start_offset = now_value.int_value;
		//记录当前的新变量
		member_variable.insert(std::pair<std::string, CbufferVariable>(variable_name, new_variable_data));
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyConstantBuffer::GetBufferSubResource(SubMemoryPointer &submemory)
{
	PancystarEngine::EngineFailReason check_error;
	check_error = PancyBasicBufferControl::GetInstance()->GetBufferSubResource(buffer_ID, submemory);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancyConstantBuffer::~PancyConstantBuffer()
{
	PancyBasicBufferControl::GetInstance()->DeleteResurceReference(buffer_ID);
}