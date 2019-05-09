#include"PancyDescriptor.h"
using namespace PancystarEngine;
PancySkinAnimationControl *PancySkinAnimationControl::this_instance = NULL;
//描述符类
DescriptorObject::DescriptorObject()
{
	PSO_pointer = NULL;
	rootsignature = NULL;
	descriptor_heap_use = NULL;
}
DescriptorObject::~DescriptorObject()
{
	PancyDescriptorHeapControl::GetInstance()->FreeResourceView(descriptor_block_id);
	for (auto release_data = per_object_cbuffer.begin(); release_data != per_object_cbuffer.end(); ++release_data)
	{
		delete release_data->second;
	}
	per_object_cbuffer.clear();
}
PancystarEngine::EngineFailReason DescriptorObject::Create(
	const std::string &PSO_name,
	const std::string &descriptor_name,
	const std::vector<std::string> &cbuffer_name_per_object,
	const std::vector<PancystarEngine::PancyConstantBuffer *> &cbuffer_per_frame,
	const std::vector<SubMemoryPointer> &resource_data_per_frame,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_in,
	const std::vector<SubMemoryPointer> &output_data_per_frame,
	const std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC> &output_desc_per_frame_in,
	const std::vector<SubMemoryPointer> &resource_data_per_object,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_object_in
)
{
	PancystarEngine::EngineFailReason check_error;
	PSO_name_descriptor = PSO_name;
	//创建一个对应类型的描述符块
	ResourceViewPointer new_point;
	pancy_object_id globel_offset = 0;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(descriptor_name, descriptor_block_id, resource_view_num);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//将渲染需要的绑定资源指针一次性全部获取并保存
	pancy_object_id PSO_id_need;
	//PSO数据
	check_error = PancyEffectGraphic::GetInstance()->GetPSO(PSO_name, PSO_id_need);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancyEffectGraphic::GetInstance()->GetPSOResource(PSO_id_need, &PSO_pointer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//rootsignature数据
	check_error = PancyEffectGraphic::GetInstance()->GetRootSignatureResource(PSO_id_need, &rootsignature);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//绑定的描述符堆数据
	check_error = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(descriptor_block_id, &descriptor_heap_use);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//绑定的描述符堆的偏移
	std::vector<pancy_object_id> descriptor_distribute;
	check_error = PancyEffectGraphic::GetInstance()->GetDescriptorDistribute(PSO_id_need, descriptor_distribute);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	pancy_object_id now_start_offset = 0;
	for (int i = 0; i < descriptor_distribute.size(); ++i)
	{
		ResourceViewPointer distribute_point;
		distribute_point.resource_view_pack_id = descriptor_block_id;
		CD3DX12_GPU_DESCRIPTOR_HANDLE now_gpu_handle;
		distribute_point.resource_view_offset_id = now_start_offset;
		check_error = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(distribute_point, now_gpu_handle);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		descriptor_offset.push_back(now_gpu_handle);
		now_start_offset += descriptor_distribute[i];
	}

	//填充描述符的信息
	new_point.resource_view_pack_id = descriptor_block_id;
	//检验传入的资源数量和描述符的数量是否匹配(如果有bindless texture则要求资源数量小于等于数组上限)
	pancy_object_id check_descriptor_size = cbuffer_name_per_object.size() + cbuffer_per_frame.size() + resource_data_per_frame.size()+ output_data_per_frame.size() + resource_data_per_object.size();
	if (resource_data_per_object.size() == 0 && check_descriptor_size != resource_view_num)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource num: " +
			std::to_string(check_descriptor_size) +
			" dismatch resource view num: " +
			std::to_string(resource_view_num) +
			" in PSO: " + PSO_name
		);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor object", error_message);
		return error_message;
	}
	else if (resource_view_num < check_descriptor_size)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource num: " +
			std::to_string(check_descriptor_size) +
			" bigger than resource view num: " +
			std::to_string(resource_view_num) +
			" in PSO: " + PSO_name
		);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor object", error_message);
		return error_message;
	}
	//先根据常量缓冲区的名称，绑定object独有的常量缓冲区。
	for (int i = 0; i < cbuffer_name_per_object.size(); ++i)
	{
		auto cbuffer_check = per_object_cbuffer.find(cbuffer_name_per_object[i]);
		if (cbuffer_check == per_object_cbuffer.end())
		{
			std::string pso_divide_path;
			std::string pso_divide_name;
			std::string pso_divide_tail;
			PancystarEngine::DivideFilePath(PSO_name, pso_divide_path, pso_divide_name, pso_divide_tail);
			PancystarEngine::PancyConstantBuffer *new_cbuffer = new PancystarEngine::PancyConstantBuffer(cbuffer_name_per_object[i], pso_divide_name);
			check_error = new_cbuffer->Create();
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			SubMemoryPointer submemory;
			check_error = new_cbuffer->GetBufferSubResource(submemory);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			new_point.resource_view_offset_id = globel_offset + i;
			check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(new_point, submemory);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			per_object_cbuffer.insert(std::pair<std::string, PancystarEngine::PancyConstantBuffer*>(cbuffer_name_per_object[i], new_cbuffer));
		}

	}
	globel_offset += cbuffer_name_per_object.size();
	//绑定每帧独有的常量缓冲区
	for (int i = 0; i < cbuffer_per_frame.size(); ++i)
	{
		SubMemoryPointer submemory;
		check_error = cbuffer_per_frame[i]->GetBufferSubResource(submemory);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_point.resource_view_offset_id = globel_offset + i;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(new_point, submemory);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	globel_offset += cbuffer_per_frame.size();
	//绑定每帧独有的shader资源
	for (int i = 0; i < resource_data_per_frame.size(); ++i)
	{
		new_point.resource_view_offset_id = globel_offset + i;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_point, resource_data_per_frame[i], resource_desc_per_frame_in[i]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	globel_offset += resource_data_per_frame.size();
	//绑定用于shader输出的UAV资源
	for (int i = 0; i < output_data_per_frame.size(); ++i)
	{
		new_point.resource_view_offset_id = globel_offset + i;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildUAV(new_point,output_data_per_frame[i], output_desc_per_frame_in[i]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	globel_offset += output_data_per_frame.size();
	//绑定每个object独有的shader资源
	for (int i = 0; i < resource_data_per_object.size(); ++i)
	{
		new_point.resource_view_offset_id = globel_offset + i;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_point, resource_data_per_object[i], resource_desc_per_object_in[i]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorObject::SetCbufferMatrix(
	const std::string &cbuffer_name,
	const std::string &variable_name,
	const DirectX::XMFLOAT4X4 &data_in,
	const pancy_resource_size &offset
)
{
	PancystarEngine::EngineFailReason check_error;
	auto cbuffer_data = per_object_cbuffer.find(cbuffer_name);
	if (cbuffer_data == per_object_cbuffer.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in DescriptorObject of PSO: " + PSO_name_descriptor);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer Matrix", error_message);
		return error_message;
	}
	check_error = cbuffer_data->second->SetMatrix(variable_name, data_in, offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorObject::SetCbufferFloat4(
	const std::string &cbuffer_name,
	const std::string &variable_name,
	const DirectX::XMFLOAT4 &data_in,
	const pancy_resource_size &offset
)
{
	PancystarEngine::EngineFailReason check_error;
	auto cbuffer_data = per_object_cbuffer.find(cbuffer_name);
	if (cbuffer_data == per_object_cbuffer.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in DescriptorObject of PSO: " + PSO_name_descriptor);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer float4", error_message);
		return error_message;
	}
	check_error = cbuffer_data->second->SetFloat4(variable_name, data_in, offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorObject::SetCbufferUint4(
	const std::string &cbuffer_name,
	const std::string &variable_name,
	const DirectX::XMUINT4 &data_in,
	const pancy_resource_size &offset
)
{
	PancystarEngine::EngineFailReason check_error;
	auto cbuffer_data = per_object_cbuffer.find(cbuffer_name);
	if (cbuffer_data == per_object_cbuffer.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in DescriptorObject of PSO: " + PSO_name_descriptor);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer uint4", error_message);
		return error_message;
	}
	check_error = cbuffer_data->second->SetUint4(variable_name, data_in, offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorObject::SetCbufferStructData(
	const std::string &cbuffer_name,
	const std::string &variable_name,
	const void* data_in,
	const pancy_resource_size &data_size,
	const pancy_resource_size &offset
)
{
	PancystarEngine::EngineFailReason check_error;
	auto cbuffer_data = per_object_cbuffer.find(cbuffer_name);
	if (cbuffer_data == per_object_cbuffer.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in DescriptorObject of PSO: " + PSO_name_descriptor);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer struct", error_message);
		return error_message;
	}
	check_error = cbuffer_data->second->SetStruct(variable_name, data_in, data_size, offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
DescriptorControl::DescriptorControl()
{
	now_object_id_top = 0;
	pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	descriptor_data_map.resize(Frame_num);
}
PancystarEngine::EngineFailReason DescriptorControl::BuildDescriptorCompute(
	const pancy_object_id &PSO_id,
	const std::vector<std::string> &cbuffer_name_per_object_in,
	const std::vector<std::vector<PancystarEngine::PancyConstantBuffer *>> &cbuffer_per_frame_in,
	const std::vector<std::vector<SubMemoryPointer>> &SRV_per_frame_in,
	const std::vector<std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>> &SRV_desc_per_frame_in,
	const std::vector<std::vector<SubMemoryPointer>> &UAV_per_frame_in,
	const std::vector<std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC>> &UAV_desc_per_frame_in,
	pancy_object_id &descriptor_ID
)
{
	PancystarEngine::EngineFailReason check_error;
	//根据PSO的ID号获取PSO的名称和描述符的格式名称
	std::string PSO_name;
	std::string descriptor_name;
	check_error = PancyEffectGraphic::GetInstance()->GetPSOName(PSO_id, PSO_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancyEffectGraphic::GetInstance()->GetPSODescriptorName(PSO_id, descriptor_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//分配一个ID号
	pancy_object_id now_id_use;
	if (!empty_object_id.empty())
	{
		now_id_use = empty_object_id.front();
	}
	else
	{
		now_id_use = now_object_id_top;
	}
	pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	for (int i = 0; i < Frame_num; ++i)
	{
		//创建一个描述符表
		DescriptorObject *new_descriptor_obj;
		new_descriptor_obj = new DescriptorObject();
		//计算着色器的bindeless texture赋空值
		std::vector<SubMemoryPointer> resource_data_per_object;
		std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> resource_desc_per_per_object;
		check_error = new_descriptor_obj->Create(
			PSO_name,
			descriptor_name,
			cbuffer_name_per_object_in,
			cbuffer_per_frame_in[i],
			SRV_per_frame_in[i],
			SRV_desc_per_frame_in[i],
			UAV_per_frame_in[i],
			UAV_desc_per_frame_in[i],
			resource_data_per_object,
			resource_desc_per_per_object
		);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		descriptor_data_map[i].insert(std::pair<pancy_object_id, DescriptorObject *>(now_id_use, new_descriptor_obj));
	}
	descriptor_ID = now_id_use;
	if (!empty_object_id.empty())
	{
		empty_object_id.pop();
	}
	else
	{
		now_object_id_top += 1;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorControl::BuildDescriptorGraph(
	const pancy_object_id &model_id,
	const pancy_object_id &PSO_id,
	const std::vector<std::string> &cbuffer_name_per_object_in,
	const std::vector<std::vector<PancystarEngine::PancyConstantBuffer *>> &cbuffer_per_frame_in,
	const std::vector<std::vector<SubMemoryPointer>> &resource_data_per_frame_in,
	const std::vector<std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>> &resource_desc_per_frame_in,
	pancy_object_id &descriptor_ID
)
{
	PancystarEngine::EngineFailReason check_error;
	//根据PSO的ID号获取PSO的名称和描述符的格式名称
	std::string PSO_name;
	std::string descriptor_name;
	check_error = PancyEffectGraphic::GetInstance()->GetPSOName(PSO_id, PSO_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancyEffectGraphic::GetInstance()->GetPSODescriptorName(PSO_id, descriptor_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//分配一个ID号
	pancy_object_id now_id_use;
	if (!empty_object_id.empty())
	{
		now_id_use = empty_object_id.front();
	}
	else
	{
		now_id_use = now_object_id_top;
	}
	pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	const std::vector<std::vector<SubMemoryPointer>> resource_data_per_frame;
	const std::vector<std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>> resource_desc_per_frame;
	for (int i = 0; i < Frame_num; ++i)
	{
		//创建一个描述符表
		DescriptorObject *new_descriptor_obj;
		new_descriptor_obj = new DescriptorObject();
		//计算着色器的bindeless texture赋空值
		std::vector<SubMemoryPointer> UAV_per_frame_in;
		std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC> UAV_desc_per_frame_in;
		std::vector<SubMemoryPointer> resource_data_per_object;
		std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> resource_desc_per_object;
		check_error = PancystarEngine::PancyModelControl::GetInstance()->GetShaderResourcePerObject(
			model_id,
			resource_data_per_object,
			resource_desc_per_object
		);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = new_descriptor_obj->Create(
			PSO_name,
			descriptor_name,
			cbuffer_name_per_object_in,
			cbuffer_per_frame_in[i],
			resource_data_per_frame_in[i],
			resource_desc_per_frame_in[i],
			UAV_per_frame_in,
			UAV_desc_per_frame_in,
			resource_data_per_object,
			resource_desc_per_object
		);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		descriptor_data_map[i].insert(std::pair<pancy_object_id, DescriptorObject *>(now_id_use, new_descriptor_obj));
	}
	if (!empty_object_id.empty())
	{
		empty_object_id.pop();
	}
	else
	{
		now_object_id_top += 1;
	}
	descriptor_ID = now_id_use;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorControl::GetDescriptor(const pancy_object_id &descriptor_id, DescriptorObject **descriptor_data)
{
	pancy_object_id now_frame = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	auto now_descriptor_data = descriptor_data_map[now_frame].find(descriptor_id);
	if (now_descriptor_data == descriptor_data_map[now_frame].end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find descriptor ID: " + std::to_string(descriptor_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Descriptor data", error_message);
		return error_message;
	}
	*descriptor_data = now_descriptor_data->second;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorControl::DeleteDescriptor(const pancy_object_id &descriptor_id)
{
	pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	for (int i = 0; i < Frame_num; ++i)
	{
		auto now_descriptor_data = descriptor_data_map[i].find(descriptor_id);
		if (now_descriptor_data == descriptor_data_map[i].end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find descriptor ID: " + std::to_string(descriptor_id));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Descriptor data", error_message);
			return error_message;
		}
		delete now_descriptor_data->second;
		descriptor_data_map[i].erase(now_descriptor_data);
	}
	return PancystarEngine::succeed;
}
DescriptorControl::~DescriptorControl()
{
	pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	for (int i = 0; i < Frame_num; ++i)
	{
		for (auto now_data = descriptor_data_map[i].begin(); now_data != descriptor_data_map[i].end(); ++now_data)
		{
			delete now_data->second;
		}
		descriptor_data_map[i].clear();
	}
}

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
	for (int i = 0; i < Frame_num; ++i)
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
	pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	for (int i = 0; i < Frame_num; ++i)
	{
		skin_naimation_buffer[i] = new PancySkinAnimationBuffer(animation_buffer_size, bone_buffer_size);
		check_error = skin_naimation_buffer[i]->Create();
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancySkinAnimationControl::BuildDescriptor(
	const pancy_object_id &mesh_buffer,
	const UINT &vertex_num,
	const UINT &per_vertex_size,
	pancy_object_id &descriptor_id
)
{
	PancystarEngine::EngineFailReason check_error;
	//根据缓冲区的ID号获取其对应的显存资源
	SubMemoryPointer now_buffer_data;
	check_error = PancystarEngine::PancyBasicBufferControl::GetInstance()->GetBufferSubResource(mesh_buffer, now_buffer_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//为输入缓冲区创建描述符
	std::vector<std::string> Cbuffer_name_per_object;
	Cbuffer_name_per_object.push_back("per_object");
	pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	std::vector<std::vector<PancystarEngine::PancyConstantBuffer *>> Cbuffer_per_frame;
	Cbuffer_per_frame.resize(Frame_num);
	//全局的缓冲区/纹理输入
	std::vector<std::vector<SubMemoryPointer>> SRV_per_frame;
	std::vector<std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>> SRV_desc_per_frame;
	SRV_per_frame.resize(Frame_num);
	SRV_desc_per_frame.resize(Frame_num);
	//全局输出缓冲区/纹理(UAV)
	std::vector<std::vector<SubMemoryPointer>> UAV_per_frame;
	std::vector<std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC>> UAV_desc_per_frame;
	UAV_per_frame.resize(Frame_num);
	UAV_desc_per_frame.resize(Frame_num);
	for (int i = 0; i < Frame_num; ++i)
	{
		//当前的输入模型顶点信息
		D3D12_SHADER_RESOURCE_VIEW_DESC  now_buffer_desc = {};
		now_buffer_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		now_buffer_desc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;
		now_buffer_desc.Buffer.StructureByteStride = per_vertex_size;
		now_buffer_desc.Buffer.NumElements = vertex_num;
		now_buffer_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		now_buffer_desc.Buffer.FirstElement = 0;

		SRV_per_frame[i].push_back(now_buffer_data);
		SRV_desc_per_frame[i].push_back(now_buffer_desc);
		//骨骼矩阵缓冲区
		SubMemoryPointer bone_matrix_buffer_res;
		D3D12_SHADER_RESOURCE_VIEW_DESC  bone_matrix_buffer_desc = {};
		skin_naimation_buffer[i]->GetBoneMatrixResource(bone_matrix_buffer_res);
		SRV_per_frame[i].push_back(bone_matrix_buffer_res);

		bone_matrix_buffer_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		bone_matrix_buffer_desc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;
		bone_matrix_buffer_desc.Buffer.StructureByteStride = sizeof(DirectX::XMFLOAT4X4);
		pancy_resource_size matrix_num = bone_buffer_size / sizeof(DirectX::XMFLOAT4X4);
		bone_matrix_buffer_desc.Buffer.NumElements = matrix_num;
		bone_matrix_buffer_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		bone_matrix_buffer_desc.Buffer.FirstElement = 0;

		SRV_desc_per_frame[i].push_back(bone_matrix_buffer_desc);
		//蒙皮结果缓冲区
		SubMemoryPointer skin_vertex_buffer_res;
		skin_naimation_buffer[i]->GetSkinVertexResource(skin_vertex_buffer_res);
		UAV_per_frame[i].push_back(skin_vertex_buffer_res);
		D3D12_UNORDERED_ACCESS_VIEW_DESC skin_vertex_buffer_desc = {};
		skin_vertex_buffer_desc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER;
		skin_vertex_buffer_desc.Buffer.StructureByteStride = sizeof(mesh_animation_data);
		pancy_resource_size vertex_num = animation_buffer_size / sizeof(mesh_animation_data);
		skin_vertex_buffer_desc.Buffer.NumElements = vertex_num;
		skin_vertex_buffer_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		skin_vertex_buffer_desc.Buffer.FirstElement = 0;
		skin_vertex_buffer_desc.Buffer.CounterOffsetInBytes = 0;
		UAV_desc_per_frame[i].push_back(skin_vertex_buffer_desc);
	}
	check_error = PancystarEngine::DescriptorControl::GetInstance()->BuildDescriptorCompute(
		PSO_skinmesh,
		Cbuffer_name_per_object,
		Cbuffer_per_frame,
		SRV_per_frame,
		SRV_desc_per_frame,
		UAV_per_frame,
		UAV_desc_per_frame,
		descriptor_id
	); 
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancySkinAnimationControl::GetSkinAnimationBuffer(std::vector<SubMemoryPointer> &skin_animation_data, pancy_resource_size &animation_buffer_size_in)
{
	pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	for (int i = 0; i < Frame_num; ++i) 
	{
		SubMemoryPointer now_resource;
		skin_naimation_buffer[i]->GetSkinVertexResource(now_resource);
		skin_animation_data.push_back(now_resource);
	}
	animation_buffer_size_in = animation_buffer_size;
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
	const pancy_object_id &mesh_buffer,
	const pancy_object_id &vertex_num,
	const pancy_object_id &descriptor_id,
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
	PancystarEngine::DescriptorObject *data_descriptor_skinmesh;
	check_error = PancystarEngine::DescriptorControl::GetInstance()->GetDescriptor(
		descriptor_id,
		&data_descriptor_skinmesh
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//填充常量缓冲区
	DirectX::XMUINT4 data_offset;
	DirectX::XMUINT4 data_num;
	data_offset.x = bone_block_pos.start_pos;
	data_offset.y = animation_block_pos.start_pos;
	data_num.x = vertex_num;
	data_num.y = matrix_num;
	check_error = data_descriptor_skinmesh->SetCbufferUint4("per_object", "data_offset", data_offset, 0);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = data_descriptor_skinmesh->SetCbufferUint4("per_object", "data_num", data_num, 0);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	auto check = data_descriptor_skinmesh->GetRootSignature();
	m_commandList_skin->GetCommandList()->SetComputeRootSignature(check);
	ID3D12DescriptorHeap *descriptor_heap_id = data_descriptor_skinmesh->GetDescriptoHeap();
	m_commandList_skin->GetCommandList()->SetDescriptorHeaps(1, &descriptor_heap_id);
	std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> descriptor_offset = data_descriptor_skinmesh->GetDescriptorOffset();
	for (int i = 0; i < descriptor_offset.size(); ++i)
	{
		m_commandList_skin->GetCommandList()->SetComputeRootDescriptorTable(i, descriptor_offset[i]);
	}
	//修改当前资源的使用格式
	SubMemoryPointer now_vertex_buffer;
	check_error = PancystarEngine::PancyBasicBufferControl::GetInstance()->GetBufferSubResource(mesh_buffer, now_vertex_buffer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = SubresourceControl::GetInstance()->ResourceBarrier(m_commandList_skin, now_vertex_buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	pancy_object_id now_frame = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	SubMemoryPointer bone_matrix_resource;
	SubMemoryPointer skin_vertex_resource;
	skin_naimation_buffer[now_frame]->GetBoneMatrixResource(bone_matrix_resource);
	skin_naimation_buffer[now_frame]->GetSkinVertexResource(skin_vertex_resource);
	check_error = SubresourceControl::GetInstance()->ResourceBarrier(m_commandList_skin, skin_vertex_resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
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
	check_error = SubresourceControl::GetInstance()->ResourceBarrier(m_commandList_skin, now_vertex_buffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = SubresourceControl::GetInstance()->ResourceBarrier(m_commandList_skin, skin_vertex_resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	m_commandList_skin->UnlockPrepare();
	return PancystarEngine::succeed;
}