#include"PancyAnimationBasic.h"
using namespace PancystarEngine;
PancySkinAnimationControl *PancySkinAnimationControl::this_instance = NULL;
//骨骼动画缓冲区
PancySkinAnimationBuffer::PancySkinAnimationBuffer(const pancy_resource_size &animation_buffer_size_in, const pancy_resource_size &bone_buffer_size_in)
{
	//获取动画结果缓冲区以及骨骼矩阵缓冲区的大小，清零当前指针的位置
	now_used_position_animation = 0;
	animation_buffer_size = animation_buffer_size_in;
	now_used_position_bone = 0;
	bone_buffer_size = bone_buffer_size_in;
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
	animation_buffer_resource_desc.buffer_res_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	animation_buffer_resource_desc.buffer_res_desc.Height = 1;
	animation_buffer_resource_desc.buffer_res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	animation_buffer_resource_desc.buffer_res_desc.MipLevels = 1;
	animation_buffer_resource_desc.buffer_res_desc.SampleDesc.Count = 1;
	animation_buffer_resource_desc.buffer_res_desc.SampleDesc.Quality = 0;
	animation_buffer_resource_desc.buffer_res_desc.Width = animation_buffer_size;
	check_error = BuildBufferResource(
		file_name,
		animation_buffer_resource_desc,
		buffer_animation,
		true
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//创建存储骨骼矩阵的缓冲区资源(UAV用于ping-pong操作)
	buffer_bone.resize(2);
	for (int buffer_index = 0; buffer_index < 2; ++buffer_index)
	{
		file_name = "pancy_skin_bone_buffer";
		PancyCommonBufferDesc bone_buffer_resource_desc;
		bone_buffer_resource_desc.buffer_type = Buffer_UnorderedAccess_static;
		bone_buffer_resource_desc.buffer_res_desc.Alignment = 0;
		bone_buffer_resource_desc.buffer_res_desc.DepthOrArraySize = 1;
		bone_buffer_resource_desc.buffer_res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bone_buffer_resource_desc.buffer_res_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		bone_buffer_resource_desc.buffer_res_desc.Height = 1;
		bone_buffer_resource_desc.buffer_res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bone_buffer_resource_desc.buffer_res_desc.MipLevels = 1;
		bone_buffer_resource_desc.buffer_res_desc.SampleDesc.Count = 1;
		bone_buffer_resource_desc.buffer_res_desc.SampleDesc.Quality = 0;
		bone_buffer_resource_desc.buffer_res_desc.Width = bone_buffer_size;
		check_error = BuildBufferResource(
			file_name,
			bone_buffer_resource_desc,
			buffer_bone[buffer_index],
			true
		);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	//创建全局id记录的缓冲区资源
	PancyCommonBufferDesc bone_parent_resource_desc;
	bone_parent_resource_desc.buffer_type = Buffer_UnorderedAccess_static;
	bone_parent_resource_desc.buffer_res_desc.Alignment = 0;
	bone_parent_resource_desc.buffer_res_desc.DepthOrArraySize = 1;
	bone_parent_resource_desc.buffer_res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bone_parent_resource_desc.buffer_res_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	bone_parent_resource_desc.buffer_res_desc.Height = 1;
	bone_parent_resource_desc.buffer_res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bone_parent_resource_desc.buffer_res_desc.MipLevels = 1;
	bone_parent_resource_desc.buffer_res_desc.SampleDesc.Count = 1;
	bone_parent_resource_desc.buffer_res_desc.SampleDesc.Quality = 0;
	bone_parent_resource_desc.buffer_res_desc.Width = bone_buffer_size;
	check_error = BuildBufferResource(
		file_name,
		bone_parent_resource_desc,
		buffer_globel_index,
		true
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
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
	pancy_object_id now_ID = static_cast<pancy_object_id>(animation_block_map.size());
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
	/*
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
	pancy_object_id now_ID = static_cast<pancy_object_id>(bone_block_map.size());
	bone_block_map.insert(std::pair<pancy_object_id, SkinAnimationBlock>(now_ID, new_bone_block));
	block_id = now_ID;
	*/
	return PancystarEngine::succeed;
}
//骨骼动画管理器
PancySkinAnimationControl::PancySkinAnimationControl(
	const pancy_resource_size &animation_buffer_size_in,
	const pancy_resource_size &bone_buffer_size_in
)
{
	animation_buffer_size = animation_buffer_size_in;
	bone_buffer_size = bone_buffer_size_in;
	pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	skin_naimation_buffer.resize(Frame_num);
}
PancySkinAnimationControl::~PancySkinAnimationControl()
{
	pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	for (pancy_object_id i = 0; i < Frame_num; ++i)
	{
		delete skin_naimation_buffer[i];
	}
	skin_naimation_buffer.clear();
}
PancystarEngine::EngineFailReason PancySkinAnimationControl::Create()
{
	PancystarEngine::EngineFailReason check_error;
	//加载PSO
	check_error = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_skinmesh.json", PSO_skinmesh);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//创建骨骼动画缓冲区
	std::vector<VirtualResourcePointer> skin_animation_buffer_data;
	std::vector<VirtualResourcePointer> bone_buffer_data;
	pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	for (pancy_object_id i = 0; i < Frame_num; ++i)
	{
		skin_naimation_buffer[i] = new PancySkinAnimationBuffer(animation_buffer_size, bone_buffer_size);
		check_error = skin_naimation_buffer[i]->Create();
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		VirtualResourcePointer &skin_vertex_resource = skin_naimation_buffer[i]->GetSkinVertexResource();
		VirtualResourcePointer &bone_matrix_resource = skin_naimation_buffer[i]->GetBoneMatrixResource();
		skin_animation_buffer_data.push_back(skin_vertex_resource);
		bone_buffer_data.push_back(bone_matrix_resource);
	}
	//创建骨骼动画缓冲区的描述符
	std::vector<BasicDescriptorDesc> skin_animation_descriptor_desc;
	BasicDescriptorDesc skin_animation_SRV_desc;
	skin_animation_SRV_desc.basic_descriptor_type = PancyDescriptorType::DescriptorTypeShaderResourceView;
	pancy_object_id number_vertex_num = static_cast<pancy_object_id>(animation_buffer_size / sizeof(PancystarEngine::mesh_animation_data));
	skin_animation_SRV_desc.shader_resource_view_desc = {};
	skin_animation_SRV_desc.shader_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	skin_animation_SRV_desc.shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;
	skin_animation_SRV_desc.shader_resource_view_desc.Buffer.StructureByteStride = sizeof(PancystarEngine::mesh_animation_data);
	skin_animation_SRV_desc.shader_resource_view_desc.Buffer.NumElements = number_vertex_num;
	skin_animation_SRV_desc.shader_resource_view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	skin_animation_SRV_desc.shader_resource_view_desc.Buffer.FirstElement = 0;
	for (pancy_object_id i = 0; i < Frame_num; ++i)
	{
		skin_animation_descriptor_desc.push_back(skin_animation_SRV_desc);
	}
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCommonGlobelDescriptor("input_point", skin_animation_descriptor_desc, skin_animation_buffer_data, true);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//创建骨骼动画缓冲区的uav
	skin_animation_descriptor_desc.clear();
	BasicDescriptorDesc skin_animation_UAV_desc;
	skin_animation_UAV_desc.basic_descriptor_type = PancyDescriptorType::DescriptorTypeUnorderedAccessView;
	skin_animation_UAV_desc.unordered_access_view_desc = {};
	skin_animation_UAV_desc.unordered_access_view_desc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER;
	skin_animation_UAV_desc.unordered_access_view_desc.Buffer.StructureByteStride = sizeof(mesh_animation_data);
	pancy_resource_size vertex_num = animation_buffer_size / sizeof(mesh_animation_data);
	skin_animation_UAV_desc.unordered_access_view_desc.Buffer.NumElements = static_cast<UINT>(vertex_num);
	skin_animation_UAV_desc.unordered_access_view_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	skin_animation_UAV_desc.unordered_access_view_desc.Buffer.FirstElement = 0;
	skin_animation_UAV_desc.unordered_access_view_desc.Buffer.CounterOffsetInBytes = 0;
	for (pancy_object_id i = 0; i < Frame_num; ++i)
	{
		skin_animation_descriptor_desc.push_back(skin_animation_UAV_desc);
	}
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCommonGlobelDescriptor("mesh_anim_data", skin_animation_descriptor_desc, skin_animation_buffer_data, true);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//创建骨骼矩阵缓冲区SRV
	skin_animation_descriptor_desc.clear();
	BasicDescriptorDesc skin_bone_srv_desc;
	skin_bone_srv_desc.basic_descriptor_type = PancyDescriptorType::DescriptorTypeShaderResourceView;
	skin_bone_srv_desc.shader_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	skin_bone_srv_desc.shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;
	skin_bone_srv_desc.shader_resource_view_desc.Buffer.StructureByteStride = sizeof(DirectX::XMFLOAT4X4);
	pancy_resource_size matrix_num = bone_buffer_size / sizeof(DirectX::XMFLOAT4X4);
	skin_bone_srv_desc.shader_resource_view_desc.Buffer.NumElements = static_cast<UINT>(matrix_num);
	skin_bone_srv_desc.shader_resource_view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	skin_bone_srv_desc.shader_resource_view_desc.Buffer.FirstElement = 0;
	for (pancy_object_id i = 0; i < Frame_num; ++i)
	{
		skin_animation_descriptor_desc.push_back(skin_bone_srv_desc);
	}
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCommonGlobelDescriptor("bone_matrix_buffer", skin_animation_descriptor_desc, bone_buffer_data, true);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}

	return PancystarEngine::succeed;
}
void PancySkinAnimationControl::ClearUsedBuffer()
{
	pancy_object_id now_frame_use = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	skin_naimation_buffer[now_frame_use]->ClearUsedBuffer();
}
PancystarEngine::EngineFailReason PancySkinAnimationControl::BuildAnimationBlock(
	const pancy_resource_size &vertex_num,
	pancy_object_id &block_id,
	SkinAnimationBlock &new_animation_block
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id now_frame_use = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	check_error = skin_naimation_buffer[now_frame_use]->BuildAnimationBlock(vertex_num, block_id, new_animation_block);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancySkinAnimationControl::BuildBoneBlock(
	const pancy_resource_size &matrix_num,
	const DirectX::XMFLOAT4X4 *matrix_data,
	pancy_object_id &block_id,
	SkinAnimationBlock &new_bone_block
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id now_frame_use = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	check_error = skin_naimation_buffer[now_frame_use]->BuildBoneBlock(matrix_num, matrix_data, block_id, new_bone_block);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
//填充渲染commandlist
PancystarEngine::EngineFailReason PancySkinAnimationControl::BuildCommandList(
	VirtualResourcePointer &mesh_buffer,
	const pancy_object_id &vertex_num,
	const PancyRenderParamID &render_param_id,
	const pancy_resource_size &matrix_num,
	const DirectX::XMFLOAT4X4 *matrix_data,
	SkinAnimationBlock &animation_block_pos,
	PancyRenderCommandList *m_commandList_skin
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id bone_block_ID;
	pancy_object_id skin_animation_block_ID;
	PancystarEngine::SkinAnimationBlock bone_block_pos;
	//根据渲染模型的顶点数据请求一块动画存储显存
	check_error = BuildAnimationBlock(vertex_num, skin_animation_block_ID, animation_block_pos);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//根据渲染模型的骨骼数据请求一块骨骼矩阵显存
	check_error = BuildBoneBlock(matrix_num, matrix_data, bone_block_ID, bone_block_pos);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//获取渲染描述符
	check_error = PancystarEngine::RenderParamSystem::GetInstance()->AddRenderParamToCommandList(render_param_id, m_commandList_skin, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//填充常量缓冲区
	DirectX::XMUINT4 data_offset;
	DirectX::XMUINT4 data_num;
	data_offset.x = static_cast<uint32_t>(bone_block_pos.start_pos);
	data_offset.y = static_cast<uint32_t>(animation_block_pos.start_pos);
	data_num.x = vertex_num;
	data_num.y = static_cast<uint32_t>(matrix_num);
	check_error = PancystarEngine::RenderParamSystem::GetInstance()->SetCbufferUint4(render_param_id, "per_object", "data_offset", data_offset, 0);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancystarEngine::RenderParamSystem::GetInstance()->SetCbufferUint4(render_param_id, "per_object", "data_num", data_num, 0);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//修改当前输入顶点资源的使用格式
	auto now_resource_data = mesh_buffer.GetResourceData();
	auto vertex_input_gpu_buffer = GetBufferResourceData(mesh_buffer, check_error);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = vertex_input_gpu_buffer->ResourceBarrier(m_commandList_skin, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//修改当前输出顶点资源的使用格式
	pancy_object_id now_frame = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	VirtualResourcePointer &bone_matrix_resource = skin_naimation_buffer[now_frame]->GetBoneMatrixResource();
	VirtualResourcePointer &skin_vertex_resource = skin_naimation_buffer[now_frame]->GetSkinVertexResource();
	auto vertex_output_gpu_buffer = GetBufferResourceData(skin_vertex_resource, check_error);
	check_error = vertex_output_gpu_buffer->ResourceBarrier(m_commandList_skin, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//提交计算命令
	pancy_object_id thread_group_size;
	thread_group_size = vertex_num / threadBlockSize;
	if (vertex_num % threadBlockSize != 0)
	{
		thread_group_size += 1;
	}
	m_commandList_skin->GetCommandList()->Dispatch(thread_group_size, 1, 1);
	//还原资源状态
	check_error = vertex_input_gpu_buffer->ResourceBarrier(m_commandList_skin, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = vertex_output_gpu_buffer->ResourceBarrier(m_commandList_skin, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	m_commandList_skin->UnlockPrepare();
	return PancystarEngine::succeed;
}