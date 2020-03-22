#include"PancyRenderParam.h"
using namespace PancystarEngine;
PancystarEngine::EngineFailReason BasicRenderParam::SetCbufferMatrix(
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
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in DescriptorObject of PSO: " + PSO_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer Matrix", error_message);
		return error_message;
	}
	pancy_object_id now_frame_id = PancyDx12DeviceBasic::GetInstance()->GetLastFrame();
	check_error = cbuffer_data->second[now_frame_id]->SetMatrix(variable_name, data_in, offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason BasicRenderParam::GetPsoData(ID3D12PipelineState  **pso_data)
{
	if (PSO_pointer == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Render Param Havn't Init Pipeline State Object Or Render Param Init Failed");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("BasicRenderParam::GetPsoData", error_message);
		return error_message;
	}
	*pso_data = PSO_pointer;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason BasicRenderParam::SetCbufferFloat4(
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
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in DescriptorObject of PSO: " + PSO_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer float4", error_message);
		return error_message;
	}
	//写操作作用在隔一帧的缓冲区
	pancy_object_id now_frame_id = PancyDx12DeviceBasic::GetInstance()->GetLastFrame();
	check_error = cbuffer_data->second[now_frame_id]->SetFloat4(variable_name, data_in, offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason BasicRenderParam::SetCbufferUint4(
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
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in DescriptorObject of PSO: " + PSO_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer uint4", error_message);
		return error_message;
	}
	//写操作作用在隔一帧的缓冲区
	pancy_object_id now_frame_id = PancyDx12DeviceBasic::GetInstance()->GetLastFrame();
	check_error = cbuffer_data->second[now_frame_id]->SetUint4(variable_name, data_in, offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason BasicRenderParam::SetCbufferStructData(
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
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in DescriptorObject of PSO: " + PSO_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer struct", error_message);
		return error_message;
	}
	//写操作作用在隔一帧的缓冲区
	pancy_object_id now_frame_id = PancyDx12DeviceBasic::GetInstance()->GetLastFrame();
	check_error = cbuffer_data->second[now_frame_id]->SetStruct(variable_name, data_in, data_size, offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
bool BasicRenderParam::CheckIfInitFinished()
{
	if (if_render_param_inited)
	{
		return if_render_param_inited;
	}
	if (globel_cbuffer_num == globel_constant_buffer.size() &&
		private_cbuffer_num == private_constant_buffer.size() &&
		globel_shader_resource_num == globel_shader_resource.size() &&
		bind_shader_resource_num == bind_shader_resource.size() &&
		bindless_shader_resource_num == bindless_shader_resource.size())
	{
		if_render_param_inited = true;
	}
	return if_render_param_inited;
}
BasicRenderParam::BasicRenderParam(const std::string &render_param_name_in)
{
	render_param_name = render_param_name_in;
}
BasicRenderParam::~BasicRenderParam()
{
	//删除私有描述符
	//删除cbuffer资源
	for (auto release_data = per_object_cbuffer.begin(); release_data != per_object_cbuffer.end(); ++release_data)
	{
		for (int i = 0; i < release_data->second.size(); ++i)
		{
			delete release_data->second[i];
		}
		release_data->second.clear();
	}
	per_object_cbuffer.clear();
}
PancystarEngine::EngineFailReason BasicRenderParam::CommonCreate(
	const std::string &PSO_name,
	const std::unordered_map<std::string, BindDescriptorPointer> &bind_shader_resource_in,
	const std::unordered_map<std::string, BindlessDescriptorPointer> &bindless_shader_resource_in
)
{
	PancystarEngine::EngineFailReason check_error;
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
	//描述符堆数据
	check_error = PancyDescriptorHeapControl::GetInstance()->GetBasicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, &descriptor_heap_use);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//获取所有的全局绑定Cbuffer信息
	const std::vector<PancyDescriptorPSODescription> *globel_cbuffer_desc;
	check_error = PancyEffectGraphic::GetInstance()->GetDescriptorDesc(PSO_id_need, PancyShaderDescriptorType::CbufferGlobel, globel_cbuffer_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	for (int i = 0; i < globel_cbuffer_desc->size(); ++i)
	{
		BindDescriptorPointer now_cbuffer_descriptor;
		const std::string &cbuffer_name = (*globel_cbuffer_desc)[i].descriptor_name;
		const pancy_object_id &now_bind_id = (*globel_cbuffer_desc)[i].rootsignature_slot;
		check_error = PancyDescriptorHeapControl::GetInstance()->GetCommonGlobelDescriptorID(PancyDescriptorType::DescriptorTypeConstantBufferView, cbuffer_name, now_cbuffer_descriptor);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		globel_constant_buffer.insert(std::pair<std::string, BindDescriptorPointer>(cbuffer_name, now_cbuffer_descriptor));
		//为全局常量缓冲区描述符记录其slot地址
		globel_constant_buffer_root_signature_offset.insert(std::pair<std::string, pancy_object_id>(cbuffer_name, now_bind_id));
	}
	//开始注册所有的私有常量缓冲区
	const std::vector<PancyDescriptorPSODescription> *private_cbuffer_desc;
	check_error = PancyEffectGraphic::GetInstance()->GetDescriptorDesc(PSO_id_need, PancyShaderDescriptorType::CbufferPrivate, private_cbuffer_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	for (int i = 0; i < private_cbuffer_desc->size(); ++i)
	{
		const std::string &cbuffer_name = (*private_cbuffer_desc)[i].descriptor_name;
		const pancy_object_id &now_bind_id = (*private_cbuffer_desc)[i].rootsignature_slot;
		//首先为所有的私有cbuffer开辟空间
		std::vector<PancyConstantBuffer*> cbuffer_double_list;
		for (UINT i = 0; i < PancyDx12DeviceBasic::GetInstance()->GetFrameNum(); ++i)
		{
			std::string pso_divide_path;
			std::string pso_divide_name;
			std::string pso_divide_tail;
			PancystarEngine::DivideFilePath(PSO_name, pso_divide_path, pso_divide_name, pso_divide_tail);
			PancyConstantBuffer *new_cbuffer = new PancyConstantBuffer();
			check_error = PancyEffectGraphic::GetInstance()->BuildCbufferByName(PSO_id_need,cbuffer_name, *new_cbuffer);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			cbuffer_double_list.push_back(new_cbuffer);
		}
		per_object_cbuffer.insert(std::pair<std::string, std::vector<PancyConstantBuffer*>>(cbuffer_name, cbuffer_double_list));
		//然后为私有的cbuffer创建描述符
		std::vector<BasicDescriptorDesc> descriptor_desc_list;
		std::vector<VirtualResourcePointer> cbuffer_memory_data;
		for (UINT i = 0; i < PancyDx12DeviceBasic::GetInstance()->GetFrameNum(); ++i)
		{
			BasicDescriptorDesc cbuffer_desc;
			cbuffer_desc.basic_descriptor_type = PancyDescriptorType::DescriptorTypeConstantBufferView;
			cbuffer_desc.block_size = cbuffer_double_list[i]->GetCbufferSize();
			cbuffer_desc.offset = cbuffer_double_list[i]->GetCbufferOffsetFromBufferHead();
			descriptor_desc_list.push_back(cbuffer_desc);
			VirtualResourcePointer cbuffer_submemory;
			cbuffer_submemory = cbuffer_double_list[i]->GetBufferResource();
			cbuffer_memory_data.push_back(cbuffer_submemory);
		}
		BindDescriptorPointer now_cbuffer_descriptor;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildCommonDescriptor(descriptor_desc_list, cbuffer_memory_data, true, now_cbuffer_descriptor);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		private_constant_buffer.insert(std::pair<std::string, BindDescriptorPointer>(cbuffer_name, now_cbuffer_descriptor));
		//为私有常量缓冲区描述符记录其slot地址
		private_constant_buffer_root_signature_offset.insert(std::pair<std::string, pancy_object_id>(cbuffer_name, now_bind_id));
	}
	//获取所有的全局绑定ShaderResource信息
	const std::vector<PancyDescriptorPSODescription> *globel_shader_resource_desc;
	check_error = PancyEffectGraphic::GetInstance()->GetDescriptorDesc(PSO_id_need, PancyShaderDescriptorType::SRVGlobel, globel_shader_resource_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	for (int i = 0; i < globel_shader_resource_desc->size(); ++i)
	{
		BindDescriptorPointer now_shader_resource_descriptor;
		const std::string &shader_resource_name = (*globel_shader_resource_desc)[i].descriptor_name;
		const pancy_object_id &now_shader_resource_bind_id = (*globel_shader_resource_desc)[i].rootsignature_slot;
		check_error = PancyDescriptorHeapControl::GetInstance()->GetCommonGlobelDescriptorID(PancyDescriptorType::DescriptorTypeShaderResourceView, shader_resource_name, now_shader_resource_descriptor);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		globel_shader_resource.insert(std::pair<std::string, BindDescriptorPointer>(shader_resource_name, now_shader_resource_descriptor));
		//为全局resource view描述符记录其slot地址
		globel_shader_resource_root_signature_offset.insert(std::pair<std::string, pancy_object_id>(shader_resource_name, now_shader_resource_bind_id));
	}
	//拷贝所有的绑定描述符
	const std::vector<PancyDescriptorPSODescription> *bind_shader_resource_desc;
	check_error = PancyEffectGraphic::GetInstance()->GetDescriptorDesc(PSO_id_need, PancyShaderDescriptorType::SRVPrivate, bind_shader_resource_desc);
	for (int i = 0; i < bind_shader_resource_desc->size(); ++i)
	{
		const std::string &shader_resource_name = (*bind_shader_resource_desc)[i].descriptor_name;
		const pancy_object_id &now_shader_resource_bind_id = (*bind_shader_resource_desc)[i].rootsignature_slot;
		//在输入的描述符数据里查找当前slot所需要的描述符
		auto check_if_have_resource = bind_shader_resource_in.find(shader_resource_name);
		if (check_if_have_resource == bind_shader_resource_in.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find descriptor slot: " + shader_resource_name + " in pso: " + PSO_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("build bind descriptor & slot", error_message);
			return error_message;
		}
		bind_shader_resource.insert(std::pair<std::string, BindDescriptorPointer>(check_if_have_resource->first, check_if_have_resource->second));
		//为私有绑定描述符记录其slot地址
		bind_shader_resource_root_signature_offset.insert(std::pair<std::string, pancy_object_id>(shader_resource_name, now_shader_resource_bind_id));
	}
	//拷贝所有的bindless描述符
	const std::vector<PancyDescriptorPSODescription> *bindless_shader_resource_desc;
	check_error = PancyEffectGraphic::GetInstance()->GetDescriptorDesc(PSO_id_need, PancyShaderDescriptorType::SRVBindless, bindless_shader_resource_desc);
	for (int i = 0; i < bindless_shader_resource_desc->size(); ++i)
	{
		const std::string &shader_resource_name = (*bindless_shader_resource_desc)[i].descriptor_name;
		const pancy_object_id &now_shader_resource_bind_id = (*bindless_shader_resource_desc)[i].rootsignature_slot;
		//在输入的描述符数据里查找当前slot所需要的描述符
		auto check_if_have_resource = bindless_shader_resource_in.find(shader_resource_name);
		if (check_if_have_resource == bindless_shader_resource_in.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find descriptor slot: " + shader_resource_name + " in pso: " + PSO_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("build bindless descriptor & slot", error_message);
			return error_message;
		}
		bindless_shader_resource.insert(std::pair<std::string, BindlessDescriptorPointer>(check_if_have_resource->first, check_if_have_resource->second));
		//为私有绑定描述符记录其slot地址
		bindless_shader_resource_root_signature_offset.insert(std::pair<std::string, pancy_object_id>(shader_resource_name, now_shader_resource_bind_id));
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason BasicRenderParam::AddToCommandList(PancyRenderCommandList *m_commandList, const D3D12_COMMAND_LIST_TYPE &render_param_type)
{
	PancystarEngine::EngineFailReason check_error;
	//设置rootsignature
	switch (render_param_type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		m_commandList->GetCommandList()->SetGraphicsRootSignature(rootsignature);
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		m_commandList->GetCommandList()->SetComputeRootSignature(rootsignature);
		break;
	default:
		PancystarEngine::EngineFailReason error_message(E_FAIL, "commond list type is not graph/compute, could not bind shader resource data");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("BasicRenderParam::AddToCommandList", error_message);
		return error_message;
		break;
	}
	//设置描述符堆(SrvUavCbv)
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &descriptor_heap_use);
	//绑定普通的描述符堆的数据(todo:先按照字符串进行绑定，之后再优化)
	check_error = BindDescriptorToRootsignature(PancyDescriptorType::DescriptorTypeConstantBufferView, globel_constant_buffer, globel_constant_buffer_root_signature_offset, render_param_type, m_commandList);
	if (!check_error.CheckIfSucceed())
	{
		return PancystarEngine::succeed;
	}
	check_error = BindDescriptorToRootsignature(PancyDescriptorType::DescriptorTypeConstantBufferView, private_constant_buffer, private_constant_buffer_root_signature_offset, render_param_type, m_commandList);
	if (!check_error.CheckIfSucceed())
	{
		return PancystarEngine::succeed;
	}
	check_error = BindDescriptorToRootsignature(PancyDescriptorType::DescriptorTypeShaderResourceView, globel_shader_resource, globel_shader_resource_root_signature_offset, render_param_type, m_commandList);
	if (!check_error.CheckIfSucceed())
	{
		return PancystarEngine::succeed;
	}
	check_error = BindDescriptorToRootsignature(PancyDescriptorType::DescriptorTypeShaderResourceView, bind_shader_resource, bind_shader_resource_root_signature_offset, render_param_type, m_commandList);
	if (!check_error.CheckIfSucceed())
	{
		return PancystarEngine::succeed;
	}
	//绑定解绑顶的描述符堆的数据
	check_error = BindBindlessDescriptorToRootsignature(PancyDescriptorType::DescriptorTypeShaderResourceView, bindless_shader_resource, bindless_shader_resource_root_signature_offset, render_param_type, m_commandList);
	if (!check_error.CheckIfSucceed())
	{
		return PancystarEngine::succeed;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason BasicRenderParam::BindDescriptorToRootsignature(
	const PancyDescriptorType &descriptor_type,
	const std::unordered_map<std::string, BindDescriptorPointer> &descriptor_data,
	const std::unordered_map<std::string, pancy_object_id> &root_signature_slot_data,
	const D3D12_COMMAND_LIST_TYPE &render_param_type,
	PancyRenderCommandList *m_commandList
)
{
	PancystarEngine::EngineFailReason check_error;
	if (descriptor_data.size() != root_signature_slot_data.size())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "descriptor resource number: " + std::to_string(descriptor_data.size()) + " dismatch descriptor slot number: " + std::to_string(root_signature_slot_data.size()) + " in pso: " + PSO_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("bind descriptor resource to slot", error_message);
		return error_message;
	}
	for (auto now_descriptor_data : descriptor_data)
	{
		auto now_descriptor_slot = root_signature_slot_data.find(now_descriptor_data.first);
		if (now_descriptor_slot == root_signature_slot_data.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find descriptor resource slot: " + now_descriptor_data.first + " in pso: " + PSO_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("bind descriptor resource to slot", error_message);
			return error_message;
		}
		check_error = PancyDescriptorHeapControl::GetInstance()->BindCommonDescriptor(now_descriptor_data.second, render_param_type, m_commandList, now_descriptor_slot->second);
		if (!check_error.CheckIfSucceed())
		{
			return PancystarEngine::succeed;
		}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason BasicRenderParam::BindBindlessDescriptorToRootsignature(
	const PancyDescriptorType &bind_descriptor_type,
	const std::unordered_map<std::string, BindlessDescriptorPointer> &descriptor_data,
	const std::unordered_map<std::string, pancy_object_id> &root_signature_slot_data,
	const D3D12_COMMAND_LIST_TYPE &render_param_type,
	PancyRenderCommandList *m_commandList
)
{
	PancystarEngine::EngineFailReason check_error;
	if (descriptor_data.size() != root_signature_slot_data.size())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "descriptor resource number: " + std::to_string(descriptor_data.size()) + " dismatch descriptor slot number: " + std::to_string(root_signature_slot_data.size()) + " in pso: " + PSO_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("bind descriptor resource to slot", error_message);
		return error_message;
	}
	for (auto now_descriptor_data : descriptor_data)
	{
		auto now_descriptor_slot = root_signature_slot_data.find(now_descriptor_data.first);
		if (now_descriptor_slot == root_signature_slot_data.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find descriptor resource slot: " + now_descriptor_data.first + " in pso: " + PSO_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("bind descriptor resource to slot", error_message);
			return error_message;
		}
		check_error = PancyDescriptorHeapControl::GetInstance()->BindBindlessDescriptor(now_descriptor_data.second, render_param_type, m_commandList, now_descriptor_slot->second);
		if (!check_error.CheckIfSucceed())
		{
			return PancystarEngine::succeed;
		}
	}
	return PancystarEngine::succeed;
}
RenderParamSystem::RenderParamSystem()
{
}
RenderParamSystem::~RenderParamSystem()
{
	for (auto render_param_list = render_param_table.begin(); render_param_list != render_param_table.end(); ++render_param_list)
	{
		for (auto render_param_data = render_param_list->second.begin(); render_param_data != render_param_list->second.end(); ++render_param_data)
		{
			delete render_param_data->second;
		}
	}
}
PancystarEngine::EngineFailReason RenderParamSystem::GetCommonRenderParam(
	const std::string &PSO_name,
	const std::string &render_param_name,
	const std::unordered_map<std::string, BindDescriptorPointer> &bind_shader_resource_in,
	const std::unordered_map<std::string, BindlessDescriptorPointer> &bindless_shader_resource_in,
	PancyRenderParamID &render_param_id
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id PSO_id_need;
	//PSO数据
	check_error = PancyEffectGraphic::GetInstance()->GetPSO(PSO_name, PSO_id_need);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if (render_param_id_self_add.find(PSO_id_need) == render_param_id_self_add.end())
	{
		//对应pso的渲染单元存储区尚未开辟，先为其开辟渲染单元存储区
		render_param_id_self_add[PSO_id_need] = 0;
		render_param_id_reuse_table.insert(std::pair<pancy_object_id, std::queue<pancy_object_id>>(PSO_id_need, std::queue<pancy_object_id>()));
		render_param_table.insert(std::pair<pancy_object_id, std::unordered_map<pancy_object_id, BasicRenderParam*>>(PSO_id_need, std::unordered_map<pancy_object_id, BasicRenderParam*>()));
		render_param_name_table.insert(std::pair<pancy_object_id, std::unordered_map<std::string, pancy_object_id>>(PSO_id_need, std::unordered_map<std::string, pancy_object_id>()));
	}
	render_param_id.PSO_id = PSO_id_need;
	auto now_used_name_table = render_param_name_table.find(PSO_id_need);
	auto now_used_render_param_table = render_param_table.find(PSO_id_need);
	if (now_used_name_table->second.find(render_param_name) == now_used_name_table->second.end())
	{
		//未发现渲染单元的模板数据，新创建一个渲染单元
		BasicRenderParam *new_render_param = new BasicRenderParam(render_param_name);
		check_error = new_render_param->CommonCreate(PSO_name, bind_shader_resource_in, bindless_shader_resource_in);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		auto now_used_id_self_add = render_param_id_self_add.find(PSO_id_need);
		auto now_used_id_reuse = render_param_id_reuse_table.find(PSO_id_need);
		pancy_object_id new_id;
		if (!now_used_id_reuse->second.empty())
		{
			new_id = now_used_id_reuse->second.front();
			now_used_id_reuse->second.pop();
		}
		else
		{
			new_id = now_used_id_self_add->second;
			now_used_id_self_add->second += 1;
		}
		now_used_name_table->second.insert(std::pair<std::string, pancy_object_id>(render_param_name, new_id));
		now_used_render_param_table->second.insert(std::pair<pancy_object_id, BasicRenderParam*>(new_id, new_render_param));
		render_param_id.render_param_id = new_id;
	}
	else
	{
		render_param_id.render_param_id = now_used_name_table->second[render_param_name];
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason RenderParamSystem::AddRenderParamToCommandList(
	const PancyRenderParamID &renderparam_id,
	PancyRenderCommandList *m_commandList,
	const D3D12_COMMAND_LIST_TYPE &render_param_type
)
{
	PancystarEngine::EngineFailReason check_error;
	BasicRenderParam* data_pointer;
	check_error = GetResource(renderparam_id, &data_pointer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = data_pointer->AddToCommandList(m_commandList, render_param_type);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason RenderParamSystem::GetPsoData(
	const PancyRenderParamID &renderparam_id,
	ID3D12PipelineState  **pso_data
)
{
	PancystarEngine::EngineFailReason check_error;
	BasicRenderParam* data_pointer;
	check_error = GetResource(renderparam_id, &data_pointer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = data_pointer->GetPsoData(pso_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason RenderParamSystem::SetCbufferMatrix(
	const PancyRenderParamID &renderparam_id,
	const std::string &cbuffer_name,
	const std::string &variable_name,
	const DirectX::XMFLOAT4X4 &data_in,
	const pancy_resource_size &offset
)
{
	PancystarEngine::EngineFailReason check_error;
	BasicRenderParam* data_pointer;
	check_error = GetResource(renderparam_id, &data_pointer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = data_pointer->SetCbufferMatrix(cbuffer_name, variable_name, data_in, offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason RenderParamSystem::SetCbufferFloat4(
	const PancyRenderParamID &renderparam_id,
	const std::string &cbuffer_name,
	const std::string &variable_name,
	const DirectX::XMFLOAT4 &data_in,
	const pancy_resource_size &offset
)
{
	PancystarEngine::EngineFailReason check_error;
	BasicRenderParam* data_pointer;
	check_error = GetResource(renderparam_id, &data_pointer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = data_pointer->SetCbufferFloat4(cbuffer_name, variable_name, data_in, offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason RenderParamSystem::SetCbufferUint4(
	const PancyRenderParamID &renderparam_id,
	const std::string &cbuffer_name,
	const std::string &variable_name,
	const DirectX::XMUINT4 &data_in,
	const pancy_resource_size &offset
)
{
	PancystarEngine::EngineFailReason check_error;
	BasicRenderParam* data_pointer;
	check_error = GetResource(renderparam_id, &data_pointer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = data_pointer->SetCbufferUint4(cbuffer_name, variable_name, data_in, offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason RenderParamSystem::SetCbufferStructData(
	const PancyRenderParamID &renderparam_id,
	const std::string &cbuffer_name,
	const std::string &variable_name,
	const void* data_in,
	const pancy_resource_size &data_size,
	const pancy_resource_size &offset
)
{
	PancystarEngine::EngineFailReason check_error;
	BasicRenderParam* data_pointer;
	check_error = GetResource(renderparam_id, &data_pointer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = data_pointer->SetCbufferStructData(cbuffer_name, variable_name, data_in, data_size, offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason RenderParamSystem::GetResource(const PancyRenderParamID &renderparam_id, BasicRenderParam** data_pointer)
{
	PancystarEngine::EngineFailReason check_error;
	*data_pointer = NULL;
	auto render_param_list_now = render_param_table.find(renderparam_id.PSO_id);
	if (render_param_list_now == render_param_table.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find PSO ID: " + std::to_string(renderparam_id.PSO_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("RenderParamSystem::AddRenderParamToCommandList", error_message);
		return error_message;
	}
	auto render_param_data = render_param_list_now->second.find(renderparam_id.render_param_id);
	if (render_param_data == render_param_list_now->second.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find renderparam ID: " + std::to_string(renderparam_id.render_param_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("RenderParamSystem::AddRenderParamToCommandList", error_message);
		return error_message;
	}
	*data_pointer = render_param_data->second;
	return PancystarEngine::succeed;
}