#include"PancyBufferDx12.h"
using namespace PancystarEngine;
//基础缓冲区
PancyBasicBuffer::PancyBasicBuffer(const bool &if_could_reload) :PancyBasicVirtualResource(if_could_reload)
{
}
void PancyBasicBuffer::BuildJsonReflect(PancyJsonReflect **pointer_data)
{
	*pointer_data = new CommonBufferJsonReflect();
}
PancystarEngine::EngineFailReason PancyBasicBuffer::InitResource()
{
	PancystarEngine::EngineFailReason check_error;
	ComPtr<ID3D12Resource> resource_data;
	PancyCommonBufferDesc resource_desc;
	//将资源的格式信息从反射类内拷贝出来
	check_error = resource_desc_value->CopyMemberData(&resource_desc, typeid(resource_desc).name(), sizeof(resource_desc));
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//在d3d层级上创建一个单独堆的buffer资源
	D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	switch (resource_desc.buffer_type)
	{
	case Buffer_ShaderResource_static:
	{
		heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		break;
	}
	case Buffer_ShaderResource_dynamic:
	{
		heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
		break;
	}
	case Buffer_Constant:
	{
		heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
		break;
	}
	case Buffer_Vertex:
	{
		heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		break;
	}
	case Buffer_Index:
	{
		heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		break;
	}
	case Buffer_UnorderedAccess_static:
	{
		heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		break;
	}
	default:
		break;
	}
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(heap_type),
		D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS,
		&resource_desc.buffer_res_desc,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&resource_data)
	);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Create commit resource error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicBuffer::InitResource", error_message);
		return error_message;
	}
	//计算缓冲区的大小，创建资源块
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetCopyableFootprints(&resource_desc.buffer_res_desc, 0, 1, 0, nullptr, nullptr, nullptr, &subresources_size);
	if (resource_desc.buffer_res_desc.Width != subresources_size)
	{
		PancystarEngine::EngineFailReason error_message(0, "buffer resource size dismatch, maybe it's not a buffer resource");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicBuffer::InitResource", error_message);
		return error_message;
	}
	buffer_data = new ResourceBlockGpu(subresources_size, resource_data, heap_type, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);
	if (heap_type == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
	{
		check_error = buffer_data->GetCpuMapPointer(&map_pointer);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}

	//如果需要拷贝数据，将数据拷贝到buffer中
	if (resource_desc.buffer_type == Buffer_Index || resource_desc.buffer_type == Buffer_Vertex || resource_desc.buffer_type == Buffer_ShaderResource_static)
	{
		char* buffer_memory = NULL;
		//todo:从文件中读取buffer
		//获取用于拷贝的commond list
		PancyRenderCommandList *copy_render_list;
		PancyThreadIdGPU copy_render_list_ID;
		check_error = ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->GetEmptyRenderlist(NULL, &copy_render_list, copy_render_list_ID);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//拷贝资源数据
		check_error = PancyDynamicRingBuffer::GetInstance()->CopyDataToGpu(copy_render_list, buffer_memory, subresources_size, *buffer_data);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		copy_render_list->UnlockPrepare();
		//提交渲染命令
		ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->SubmitRenderlist(1, &copy_render_list_ID);
		//分配等待眼位
		PancyFenceIdGPU WaitFence;
		ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->SetGpuBrokenFence(WaitFence);
		check_error = buffer_data->SetResourceCopyBrokenFence(WaitFence);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
}
bool PancyBasicBuffer::CheckIfResourceLoadFinish()
{
	PancyResourceLoadState now_load_state = buffer_data->GetResourceLoadingState();
	if (now_load_state == PancyResourceLoadState::RESOURCE_LOAD_CPU_FINISH || now_load_state == PancyResourceLoadState::RESOURCE_LOAD_GPU_FINISH)
	{
		return true;
	}
	return false;
}
PancyBasicBuffer::~PancyBasicBuffer() 
{
	if (buffer_data != NULL) 
	{
		delete buffer_data;
	}
}

//骨骼动画缓冲区
PancySkinAnimationBuffer::PancySkinAnimationBuffer(const pancy_resource_size &animation_buffer_size_in, const pancy_resource_size &bone_buffer_size_in)
{
	//获取动画结果缓冲区以及骨骼矩阵缓冲区的大小，清零当前指针的位置
	now_used_position_animation = 0;
	animation_buffer_size = animation_buffer_size_in;
	now_used_position_bone = 0;
	bone_buffer_size = bone_buffer_size_in;
	bone_data_pointer = NULL;
}
PancySkinAnimationBuffer::~PancySkinAnimationBuffer() 
{
}
PancystarEngine::EngineFailReason PancySkinAnimationBuffer::Create()
{
	PancystarEngine::EngineFailReason check_error;
	std::string file_name = "pancy_skin_mesh_buffer";
	Json::Value root_value;
	std::string buffer_subresource_name;
	//创建存储蒙皮结果的缓冲区资源(静态缓冲区)
	PancyCommonBufferDesc animation_buffer_resource_desc;
	animation_buffer_resource_desc.buffer_type = Buffer_UnorderedAccess_static;
	animation_buffer_resource_desc.buffer_res_desc.Alignment = 0;
	animation_buffer_resource_desc.buffer_res_desc.DepthOrArraySize = 1;
	animation_buffer_resource_desc.buffer_res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	animation_buffer_resource_desc.buffer_res_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	animation_buffer_resource_desc.buffer_res_desc.Height = 1;
	animation_buffer_resource_desc.buffer_res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	animation_buffer_resource_desc.buffer_res_desc.MipLevels = 1;
	animation_buffer_resource_desc.buffer_res_desc.SampleDesc.Count = 1;
	animation_buffer_resource_desc.buffer_res_desc.SampleDesc.Quality = 0;
	animation_buffer_resource_desc.buffer_res_desc.Width = animation_buffer_size;
	auto check_error = PancyGlobelResourceControl::GetInstance()->LoadResource<PancyBasicBuffer>(
		file_name,
		&animation_buffer_resource_desc,
		typeid(animation_buffer_resource_desc).name(),
		sizeof(animation_buffer_resource_desc),
		buffer_animation,
		true
		);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//加载存储骨骼矩阵的缓冲区资源(动态缓冲区)
	file_name = "pancy_skin_bone_buffer";
	PancyCommonBufferDesc bone_buffer_resource_desc;
	bone_buffer_resource_desc.buffer_type = Buffer_ShaderResource_dynamic;
	bone_buffer_resource_desc.buffer_res_desc.Alignment = 0;
	bone_buffer_resource_desc.buffer_res_desc.DepthOrArraySize = 1;
	bone_buffer_resource_desc.buffer_res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bone_buffer_resource_desc.buffer_res_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	bone_buffer_resource_desc.buffer_res_desc.Height = 1;
	bone_buffer_resource_desc.buffer_res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bone_buffer_resource_desc.buffer_res_desc.MipLevels = 1;
	bone_buffer_resource_desc.buffer_res_desc.SampleDesc.Count = 1;
	bone_buffer_resource_desc.buffer_res_desc.SampleDesc.Quality = 0;
	bone_buffer_resource_desc.buffer_res_desc.Width = bone_buffer_size;
	auto check_error = PancyGlobelResourceControl::GetInstance()->LoadResource<PancyBasicBuffer>(
		file_name,
		&animation_buffer_resource_desc,
		typeid(animation_buffer_resource_desc).name(),
		sizeof(animation_buffer_resource_desc),
		buffer_bone,
		true
		);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//将存储骨骼矩阵的缓冲区在CPU端的指针进行保留
	const PancyBasicBuffer *buffer_pointer = dynamic_cast<const PancyBasicBuffer*>(buffer_bone.GetResourceData());
	bone_data_pointer = buffer_pointer->GetBufferCPUPointer();
	if (bone_data_pointer == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL,"failed to get skinmesh bone buffer cpu pointer from resource");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancySkinAnimationBuffer::Create", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
void PancySkinAnimationBuffer::ClearUsedBuffer()
{
	now_used_position_animation = 0;
	animation_block_map.clear();
	now_used_position_bone = 0;
	bone_block_map.clear();
}
//从当前缓冲区中请求一块骨骼动画数据
PancystarEngine::EngineFailReason PancySkinAnimationBuffer::BuildAnimationBlock(
	const pancy_resource_size &vertex_num, 
	pancy_object_id &block_id,
	SkinAnimationBlock &new_animation_block)
{
	pancy_resource_size now_ask_size = vertex_num * sizeof(mesh_animation_data);
	new_animation_block.start_pos = now_used_position_animation;
	new_animation_block.block_size = now_ask_size;
	//判断当前的动画缓冲区是否能够开出请求的顶点存储块
	if ((now_used_position_animation + now_ask_size) >= animation_buffer_size)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "The skin mesh animation buffer is full");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Animation Block", error_message);
		return error_message;
	}
	//更新当前的缓冲区指针位置
	now_used_position_animation = now_used_position_animation + now_ask_size;
	//将内存块导入到map中
	pancy_object_id now_ID = animation_block_map.size();
	animation_block_map.insert(std::pair<pancy_object_id, SkinAnimationBlock>(now_ID, new_animation_block));
	block_id = now_ID;
	return PancystarEngine::succeed;
}
//从当前骨骼矩阵缓冲区中请求一块数据区
PancystarEngine::EngineFailReason PancySkinAnimationBuffer::BuildBoneBlock(
	const pancy_resource_size &matrix_num, 
	const DirectX::XMFLOAT4X4 *matrix_data,
	pancy_object_id &block_id,
	SkinAnimationBlock &new_bone_block
)
{
	pancy_resource_size now_ask_size = matrix_num * sizeof(DirectX::XMFLOAT4X4);
	new_bone_block.start_pos = now_used_position_bone;
	new_bone_block.block_size = now_ask_size;
	//判断当前的动画缓冲区是否能够开出请求的顶点存储块
	if ((now_used_position_bone + now_ask_size) >= bone_buffer_size)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "The Bone Matrix buffer is full");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Bone Matrix Block", error_message);
		return error_message;
	}
	//将数据拷贝到当前的缓冲区位置
	memcpy(bone_data_pointer + (new_bone_block.start_pos / sizeof(UINT8)), matrix_data, new_bone_block.block_size);
	//更新当前的缓冲区指针位置
	now_used_position_bone = now_used_position_bone + now_ask_size;
	//将内存块导入到map中
	pancy_object_id now_ID = bone_block_map.size();
	bone_block_map.insert(std::pair<pancy_object_id, SkinAnimationBlock>(now_ID, new_bone_block));
	block_id = now_ID;
	return PancystarEngine::succeed;
}