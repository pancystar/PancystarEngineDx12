#include"PancyGeometryDx12.h"
using namespace PancystarEngine;
GeometryBasic::GeometryBasic()
{
	all_vertex = 0;
	all_index = 0;
	all_index_adj = 0;
	if_buffer_created = false;
	if_build_finish = false;
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
	int64_t memory_alignment_size,
	int64_t memory_block_alignment_size,
	SubMemoryPointer &default_buffer,
	SubMemoryPointer &upload_buffer,
	const void* initData,
	const UINT BufferSize,
	D3D12_RESOURCE_STATES buffer_type
)
{
	HRESULT hr;
	PancystarEngine::EngineFailReason check_error;
	//先创建4M对齐的存储块
	UINT alignment_buffer_size = (BufferSize + memory_alignment_size) & ~(memory_alignment_size - 1);//4M对齐
	std::string heapdesc_file_name = "json\\resource_heap\\PointBuffer" + std::to_string(alignment_buffer_size) + ".json";
	std::string dynamic_heapdesc_file_name = "json\\resource_heap\\DynamicBuffer" + std::to_string(alignment_buffer_size) + ".json";
	//检验资源堆格式创建
	if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(heapdesc_file_name))
	{
		//更新格式文件
		Json::Value json_data_out;
		UINT resource_block_num = (4194304 * 20) / alignment_buffer_size;
		if (resource_block_num < 1)
		{
			resource_block_num = 1;
		}
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "commit_block_num", resource_block_num);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "per_block_size", alignment_buffer_size);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "heap_type_in", "D3D12_HEAP_TYPE_DEFAULT");
		PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_out, "heap_flag_in", "D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS");
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, heapdesc_file_name);
		//将文件标记为已经创建
		FileBuildRepeatCheck::GetInstance()->AddFileName(heapdesc_file_name);
	}
	//检验上传的临时资源堆格式创建
	if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(dynamic_heapdesc_file_name))
	{
		//更新格式文件
		Json::Value json_data_out;
		UINT resource_block_num = (4194304 * 5) / alignment_buffer_size;
		if (resource_block_num < 1)
		{
			resource_block_num = 1;
		}
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "commit_block_num", resource_block_num);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "per_block_size", alignment_buffer_size);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "heap_type_in", "D3D12_HEAP_TYPE_UPLOAD");
		PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_out, "heap_flag_in", "D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS");
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, dynamic_heapdesc_file_name);
		//将文件标记为已经创建
		FileBuildRepeatCheck::GetInstance()->AddFileName(dynamic_heapdesc_file_name);
	}
	//创建存储单元的分区大小
	UINT buffer_block_size = (BufferSize + memory_block_alignment_size) & ~(memory_block_alignment_size - 1);//128k对齐
	std::string bufferblock_file_name = "json\\resource_view\\PointBufferSub" + std::to_string(buffer_block_size) + ".json";
	std::string dynamic_bufferblock_file_name = "json\\resource_view\\DynamicBufferSub" + std::to_string(buffer_block_size) + ".json";
	//检验资源堆存储单元格式创建
	if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(bufferblock_file_name))
	{
		//更新格式文件
		Json::Value json_data_resourceview;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "ResourceType", heapdesc_file_name);
		Json::Value json_data_res_desc;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Dimension", "D3D12_RESOURCE_DIMENSION_BUFFER");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Alignment", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Width", alignment_buffer_size);
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
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_STATES", "D3D12_RESOURCE_STATE_COPY_DEST");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "per_block_size", buffer_block_size);
		//写入文件并标记为已创建
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_resourceview, bufferblock_file_name);
		FileBuildRepeatCheck::GetInstance()->AddFileName(bufferblock_file_name);
	}
	//检验上传的临时资源堆存储单元格式创建
	if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(dynamic_bufferblock_file_name))
	{
		//更新格式文件
		Json::Value json_data_resourceview;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "ResourceType", dynamic_heapdesc_file_name);
		Json::Value json_data_res_desc;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Dimension", "D3D12_RESOURCE_DIMENSION_BUFFER");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Alignment", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Width", alignment_buffer_size);
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
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_STATES", "D3D12_RESOURCE_STATE_GENERIC_READ");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "per_block_size", buffer_block_size);
		//写入文件并标记为已创建
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_resourceview, dynamic_bufferblock_file_name);
		FileBuildRepeatCheck::GetInstance()->AddFileName(dynamic_bufferblock_file_name);
	}
	check_error = SubresourceControl::GetInstance()->BuildSubresourceFromFile(bufferblock_file_name, default_buffer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = SubresourceControl::GetInstance()->BuildSubresourceFromFile(dynamic_bufferblock_file_name, upload_buffer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//向CPU可访问缓冲区添加数据
	D3D12_SUBRESOURCE_DATA vertexData_buffer = {};
	vertexData_buffer.pData = initData;
	vertexData_buffer.RowPitch = BufferSize;
	vertexData_buffer.SlicePitch = vertexData_buffer.RowPitch;
	int64_t per_memory_size;
	auto dest_res = SubresourceControl::GetInstance()->GetResourceData(default_buffer, per_memory_size);
	auto copy_res = SubresourceControl::GetInstance()->GetResourceData(upload_buffer, per_memory_size);
	
	/*
	BYTE* pDataBegin;
	hr = copy_res->GetResource()->Map(0, NULL, reinterpret_cast<void**>(&pDataBegin));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "could not map the point buffer data for model load");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load model vertex/index data", error_message);
		return error_message;
	}
	memcpy(pDataBegin + upload_buffer.offset*per_memory_size, initData, BufferSize);
	//todo:此处不调用unmap会导致程序不可调试，同类型的commitresource则不受影响，原因不明
	copy_res->GetResource()->Unmap(0, NULL);
	*/
	//先将cpu上的数据拷贝到上传缓冲区
	check_error = copy_res->WriteFromCpuToBuffer(upload_buffer.offset*per_memory_size, initData, BufferSize);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//将上传缓冲区的数据拷贝到显存缓冲区
	cmdList->CopyBufferRegion(dest_res->GetResource().Get(), default_buffer.offset * per_memory_size, copy_res->GetResource().Get(), upload_buffer.offset*per_memory_size, buffer_block_size);
	//修改缓冲区格式
	cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			dest_res->GetResource().Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			buffer_type
		)
	);
	return PancystarEngine::succeed;
}