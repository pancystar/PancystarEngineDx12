#include"PancyBufferDx12.h"
using namespace PancystarEngine;
//基础缓冲区
PancyBasicBuffer::PancyBasicBuffer(const std::string &resource_name_in, Json::Value root_value_in) :PancyBasicVirtualResource(resource_name_in, root_value_in)
{
	WaitFence = 0;
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
	if (buffer_type == PancyBufferType::Buffer_ShaderResource_dynamic || buffer_type == PancyBufferType::Buffer_Constant) 
	{
		now_res_state = ResourceStateType::resource_state_load_GPU_memory_finish;
	}
	else 
	{
		now_res_state = ResourceStateType::resource_state_load_CPU_memory_finish;
	}
	
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicBuffer::UpdateResourceToGPU(
	ResourceStateType &now_res_state,
	void* resource,
	const pancy_resource_size &resource_size_in
) 
{
	ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY)->GetEmptyRenderlist();


	WaitFence = ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY)->GetNextBrokenFence();
}

//缓冲区管理器
PancyBasicBufferControl::PancyBasicBufferControl(const std::string &resource_type_name_in) : PancyBasicResourceControl(resource_type_name_in)
{
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
	pancy_object_id memory_num_per_heap = 0;//堆所开辟的内存块数量
	pancy_object_id block_num_per_memory = 0; //内存所开辟的数据块数量
	//确定缓冲区的对齐大小
	if (buffer_type == Buffer_Constant) 
	{
		//计算常量缓冲区对齐大小
		if (subresources_size > 65536) 
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL,"the constant buffer size need less than 64K,Ask: " + std::to_string(subresources_size));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build constant buffer", error_message);
			return error_message;
		}
		else if (subresources_size > 16384) 
		{
			heap_alize_size = static_cast<pancy_resource_size>(ConstantBufferHeapAliaze256K);
			subresource_alize_size = static_cast<pancy_resource_size>(ConstantBufferSubResourceAliaze256K);
		}
		else 
		{
			heap_alize_size = static_cast<pancy_resource_size>(ConstantBufferHeapAliaze64K);
			subresource_alize_size = static_cast<pancy_resource_size>(ConstantBufferSubResourceAliaze64K);
		}
	}
	else 
	{
		//根据当前资源的大小，决定使用哪种对齐方式
		if (subresources_size > 16777216) 
		{
			//大于16M的内存直接开辟
			if (subresources_size % 65536 != 0) 
			{
				subresources_size = ((subresources_size / 65536) + 1) * 65536;
			}
			//限定开辟的数据，单堆，单数据
			heap_alize_size = subresources_size;
			subresource_alize_size = subresources_size;
			memory_num_per_heap = 1;
			block_num_per_memory = 1;
		}
		else 
		{
			if (subresources_size > 4194304)
			{
				//4M-16M的内存
				heap_alize_size = static_cast<pancy_resource_size>(BufferHeapAliaze64M);
				subresource_alize_size = static_cast<pancy_resource_size>(BufferSubResourceAliaze64M);
			}
			else if (subresources_size > 1048576)
			{
				//1M-4M的内存
				heap_alize_size = static_cast<pancy_resource_size>(BufferHeapAliaze16M);
				subresource_alize_size = static_cast<pancy_resource_size>(BufferSubResourceAliaze16M);
			}
			else if (subresources_size > 262144) 
			{
				//256K-1M的内存
				heap_alize_size = static_cast<pancy_resource_size>(BufferHeapAliaze4M);
				subresource_alize_size = static_cast<pancy_resource_size>(BufferSubResourceAliaze4M);
			}
			else
			{
				//0-256K的内存
				heap_alize_size = static_cast<pancy_resource_size>(BufferHeapAliaze1M);
				subresource_alize_size = static_cast<pancy_resource_size>(BufferSubResourceAliaze1M);
			}
		}
	}
	//根据内存堆的对其大小以及数据区的对齐大小，计算出heap的大小以及数据区的数量
	if (memory_num_per_heap == 0) 
	{
		//根据每块内存的对齐大小确定每个堆可以开辟的内存数量
		memory_num_per_heap = static_cast<pancy_object_id>(static_cast<pancy_resource_size>(MaxWasteSpace) / heap_alize_size);
		if (memory_num_per_heap > 32)
		{
			memory_num_per_heap = 32;
		}
		//根据每块数据区的对齐大小重改数据区的请求大小
		if (subresources_size % subresource_alize_size != 0)
		{
			subresources_size = ((subresources_size / subresource_alize_size) + 1) * subresource_alize_size;
		}
		//根据数据区的对齐大小计算每块内存可以开辟的数据区数量
		block_num_per_memory = static_cast<pancy_object_id>(heap_alize_size / subresources_size);
	}
	//计算存储堆和存储单元的名称
	std::string bufferblock_file_name = "";
	std::string heap_name = "";
	if (buffer_type == Buffer_ShaderResource_static || buffer_type == Buffer_Vertex || buffer_type == Buffer_Index)
	{
		resource_create_state = D3D12_RESOURCE_STATE_COPY_DEST;
		heap_type = D3D12_HEAP_TYPE_DEFAULT;
		//静态缓冲区
		bufferblock_file_name = "json\\resource_view\\StaticBufferSub_";
		std::string heap_name = "json\\resource_heap\\StaticBuffer_";
	}
	else if (buffer_type == Buffer_ShaderResource_dynamic || buffer_type == Buffer_Constant)
	{
		resource_create_state = D3D12_RESOURCE_STATE_GENERIC_READ;
		heap_type = D3D12_HEAP_TYPE_UPLOAD;
		//动态缓冲区
		bufferblock_file_name = "json\\resource_view\\DynamicBufferSub_";
		std::string heap_name = "json\\resource_heap\\DynamicBuffer_";
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
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "commit_block_num", memory_num_per_heap);
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
		return error_message;
	}
	if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(bufferblock_file_name))
	{
		//更新格式文件
		Json::Value json_data_resourceview;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "ResourceType", heap_name);
		Json::Value json_data_res_desc;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Dimension", "D3D12_RESOURCE_DIMENSION_BUFFER");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Alignment", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Width", subresources_size);
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
		return error_message;
	}
	subresource_desc_name = bufferblock_file_name;
	return PancystarEngine::succeed;
}