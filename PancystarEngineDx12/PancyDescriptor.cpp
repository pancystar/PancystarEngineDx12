#include"PancyDescriptor.h"
using namespace PancystarEngine;
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
	pancy_object_id check_descriptor_size = cbuffer_name_per_object.size() + cbuffer_per_frame.size() + resource_data_per_frame.size() + resource_data_per_object.size();
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
/*
//描述符链
DescriptorObjectList::DescriptorObjectList(
	const std::string &PSO_name_in,
	const std::string &descriptor_name_in
)
{
	PSO_name = PSO_name_in;
	descriptor_name = descriptor_name_in;
}
DescriptorObjectList::~DescriptorObjectList()
{
	//删除所有的描述符备份
	while (!empty_list.empty())
	{
		auto data = empty_list.front();
		delete data;
		empty_list.pop();

	}
	while (!used_list.empty())
	{
		auto data = used_list.front();
		delete data;
		used_list.pop();

	}
}
PancystarEngine::EngineFailReason DescriptorObjectList::Create(
	const std::vector<std::string> &cbuffer_name_per_object_in,
	const std::vector<PancystarEngine::PancyConstantBuffer *> &cbuffer_per_frame_in,
	const std::vector<SubMemoryPointer> &resource_data_per_frame_in,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_in,
	const std::vector<SubMemoryPointer> &resource_data_per_object_in,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_object_in
)
{
	PancystarEngine::EngineFailReason check_error;
	//将资源信息拷贝
	for (int i = 0; i < cbuffer_name_per_object_in.size(); ++i)
	{
		//检验传入的每个常量缓冲区名称是否合法
		pancy_object_id PSO_id;
		check_error = PancyEffectGraphic::GetInstance()->GetPSO(PSO_name, PSO_id);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = PancyEffectGraphic::GetInstance()->CheckCbuffer(PSO_id, cbuffer_name_per_object_in[i]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		cbuffer_name_per_object.push_back(cbuffer_name_per_object_in[i]);
	}
	for (int i = 0; i < cbuffer_per_frame_in.size(); ++i)
	{
		cbuffer_per_frame.push_back(cbuffer_per_frame_in[i]);
	}
	for (int i = 0; i < resource_data_per_frame_in.size(); ++i)
	{
		resource_data_per_frame.push_back(resource_data_per_frame_in[i]);
	}
	for (int i = 0; i < resource_data_per_object_in.size(); ++i)
	{
		resource_data_per_object.push_back(resource_data_per_object_in[i]);
	}
	for (int i = 0; i < resource_desc_per_frame_in.size(); ++i)
	{
		resource_desc_per_frame.push_back(resource_desc_per_frame_in[i]);
	}
	for (int i = 0; i < resource_desc_per_object_in.size(); ++i)
	{
		resource_desc_per_per_object.push_back(resource_desc_per_object_in[i]);
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorObjectList::GetEmptyList(DescriptorObject** descripto_res)
{
	if (empty_list.size() > 0)
	{
		auto empty_descriptor = empty_list.front();
		empty_list.pop();
		used_list.push(empty_descriptor);
		*descripto_res = empty_descriptor;
	}
	else
	{
		DescriptorObject *new_descriptor_obj;
		new_descriptor_obj = new DescriptorObject();
		auto check_error = new_descriptor_obj->Create(
			PSO_name,
			descriptor_name,
			cbuffer_name_per_object,
			cbuffer_per_frame,
			resource_data_per_frame,
			resource_desc_per_frame,
			resource_data_per_object,
			resource_desc_per_per_object
		);
		if (!check_error.CheckIfSucceed())
		{
			*descripto_res = NULL;
			return check_error;
		}
		used_list.push(new_descriptor_obj);
		*descripto_res = new_descriptor_obj;
	}
	return PancystarEngine::succeed;
}
void DescriptorObjectList::Reset()
{
	//将已经使用完毕的描述符还原
	while (!used_list.empty())
	{
		auto empty_descriptor = used_list.front();
		used_list.pop();
		empty_list.push(empty_descriptor);
	}
}

*/
DescriptorControl::DescriptorControl()
{
	now_object_id_top = 0;
	pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	descriptor_data_map.resize(Frame_num);
}
PancystarEngine::EngineFailReason DescriptorControl::BuildDescriptorCompute(
	const pancy_object_id &PSO_id,
	const std::vector<std::string> &cbuffer_name_per_object_in,
	const std::vector<PancystarEngine::PancyConstantBuffer *> &cbuffer_per_frame_in,
	const std::vector<SubMemoryPointer> &SRV_per_frame_in,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc_per_frame_in,
	const std::vector<SubMemoryPointer> &UAV_per_frame_in,
	const std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC> &UAV_desc_per_frame_in,
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
			cbuffer_per_frame_in,
			SRV_per_frame_in,
			SRV_desc_per_frame_in,
			UAV_per_frame_in,
			UAV_desc_per_frame_in,
			resource_data_per_object,
			resource_desc_per_per_object
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
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorControl::BuildDescriptorGraph(
	const pancy_object_id &model_id,
	const pancy_object_id &PSO_id,
	const std::vector<std::string> &cbuffer_name_per_object_in,
	const std::vector<PancystarEngine::PancyConstantBuffer *> &cbuffer_per_frame_in,
	const std::vector<SubMemoryPointer> &resource_data_per_frame_in,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_in,
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
		std::vector<SubMemoryPointer> UAV_per_frame_in;
		std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC> UAV_desc_per_frame_in;
		std::vector<SubMemoryPointer> resource_data_per_object;
		std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> resource_desc_per_per_object;
		check_error = PancystarEngine::PancyModelControl::GetInstance()->GetShaderResourcePerObject(model_id, resource_data_per_object, resource_desc_per_per_object);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = new_descriptor_obj->Create(
			PSO_name,
			descriptor_name,
			cbuffer_name_per_object_in,
			cbuffer_per_frame_in,
			resource_data_per_frame_in,
			resource_desc_per_frame_in,
			UAV_per_frame_in,
			UAV_desc_per_frame_in,
			resource_data_per_object,
			resource_desc_per_per_object
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