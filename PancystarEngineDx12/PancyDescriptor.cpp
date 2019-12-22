#include"PancyDescriptor.h"
using namespace PancystarEngine;
PancySkinAnimationControl *PancySkinAnimationControl::this_instance = NULL;
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
		if_render_param_inited == true;
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
		const Json::Value *cbuffer_desc_root;
		//首先为所有的私有cbuffer开辟空间
		std::vector<PancystarEngine::PancyConstantBuffer*> cbuffer_double_list;
		for (int i = 0; i < PancyDx12DeviceBasic::GetInstance()->GetFrameNum(); ++i)
		{
			std::string pso_divide_path;
			std::string pso_divide_name;
			std::string pso_divide_tail;
			PancystarEngine::DivideFilePath(PSO_name, pso_divide_path, pso_divide_name, pso_divide_tail);
			PancystarEngine::PancyConstantBuffer *new_cbuffer = new PancystarEngine::PancyConstantBuffer(cbuffer_name, pso_divide_name);
			check_error = PancyEffectGraphic::GetInstance()->GetCbuffer(PSO_id_need, cbuffer_name, cbuffer_desc_root);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			check_error = new_cbuffer->Create(*cbuffer_desc_root);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			cbuffer_double_list.push_back(new_cbuffer);
		}
		per_object_cbuffer.insert(std::pair<std::string, std::vector<PancystarEngine::PancyConstantBuffer*>>(cbuffer_name, cbuffer_double_list));
		//然后为私有的cbuffer创建描述符
		std::vector<BasicDescriptorDesc> descriptor_desc_list;
		std::vector<SubMemoryPointer> cbuffer_memory_data;
		for (int i = 0; i < PancyDx12DeviceBasic::GetInstance()->GetFrameNum(); ++i)
		{
			BasicDescriptorDesc cbuffer_desc;
			cbuffer_desc.basic_descriptor_type = PancyDescriptorType::DescriptorTypeConstantBufferView;
			descriptor_desc_list.push_back(cbuffer_desc);
			SubMemoryPointer cbuffer_submemory;
			check_error = cbuffer_double_list[i]->GetBufferSubResource(cbuffer_submemory);
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
		PancystarEngine::EngineFailReason error_message(E_FAIL,"commond list type is not graph/compute, could not bind shader resource data");
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




/*
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
	__int64 t1, t2;
	double dt;
	t1 = GlobelTimeCount::GetInstance()->GetSystemTime();
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
	t2 = GlobelTimeCount::GetInstance()->GetSystemTime();
	dt = (t2 - t1) * (double)GlobelTimeCount::GetInstance()->GetSystemFreq();
	//MessageBox(0, std::to_wstring(dt * 1000).c_str(), L"check", MB_OK);
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
		
		for (int k= 0; k <100; ++k) 
		{
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
		}
		
		
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
*/
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
	std::vector<SubMemoryPointer> skin_animation_buffer_data;
	std::vector<SubMemoryPointer> bone_buffer_data;
	pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	for (int i = 0; i < Frame_num; ++i)
	{
		skin_naimation_buffer[i] = new PancySkinAnimationBuffer(animation_buffer_size, bone_buffer_size);
		check_error = skin_naimation_buffer[i]->Create();
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		SubMemoryPointer skin_vertex_resource;
		SubMemoryPointer bone_matrix_resource;
		skin_naimation_buffer[i]->GetSkinVertexResource(skin_vertex_resource);
		skin_naimation_buffer[i]->GetBoneMatrixResource(bone_matrix_resource);
		skin_animation_buffer_data.push_back(skin_vertex_resource);
		bone_buffer_data.push_back(bone_matrix_resource);
	}
	//创建骨骼动画缓冲区的描述符
	std::vector<BasicDescriptorDesc> skin_animation_descriptor_desc;
	BasicDescriptorDesc skin_animation_SRV_desc;
	skin_animation_SRV_desc.basic_descriptor_type = PancyDescriptorType::DescriptorTypeShaderResourceView;
	pancy_object_id number_vertex_num = animation_buffer_size / sizeof(PancystarEngine::mesh_animation_data);
	skin_animation_SRV_desc.shader_resource_view_desc = {};
	skin_animation_SRV_desc.shader_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	skin_animation_SRV_desc.shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;
	skin_animation_SRV_desc.shader_resource_view_desc.Buffer.StructureByteStride = sizeof(PancystarEngine::mesh_animation_data);
	skin_animation_SRV_desc.shader_resource_view_desc.Buffer.NumElements = number_vertex_num;
	skin_animation_SRV_desc.shader_resource_view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	skin_animation_SRV_desc.shader_resource_view_desc.Buffer.FirstElement = 0;
	for (int i = 0; i < Frame_num; ++i)
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
	skin_animation_UAV_desc.unordered_access_view_desc.Buffer.NumElements = vertex_num;
	skin_animation_UAV_desc.unordered_access_view_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	skin_animation_UAV_desc.unordered_access_view_desc.Buffer.FirstElement = 0;
	skin_animation_UAV_desc.unordered_access_view_desc.Buffer.CounterOffsetInBytes = 0;
	for (int i = 0; i < Frame_num; ++i)
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
	skin_bone_srv_desc.shader_resource_view_desc.Buffer.NumElements = matrix_num;
	skin_bone_srv_desc.shader_resource_view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	skin_bone_srv_desc.shader_resource_view_desc.Buffer.FirstElement = 0;
	for (int i = 0; i < Frame_num; ++i)
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
/*
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
*/
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
	data_offset.x = bone_block_pos.start_pos;
	data_offset.y = animation_block_pos.start_pos;
	data_num.x = vertex_num;
	data_num.y = matrix_num;
	check_error = PancystarEngine::RenderParamSystem::GetInstance()->SetCbufferUint4(render_param_id,"per_object", "data_offset", data_offset, 0);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancystarEngine::RenderParamSystem::GetInstance()->SetCbufferUint4(render_param_id,"per_object", "data_num", data_num, 0);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	/*
	auto check = data_descriptor_skinmesh->GetRootSignature();
	m_commandList_skin->GetCommandList()->SetComputeRootSignature(check);
	ID3D12DescriptorHeap *descriptor_heap_id = data_descriptor_skinmesh->GetDescriptoHeap();
	m_commandList_skin->GetCommandList()->SetDescriptorHeaps(1, &descriptor_heap_id);
	std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> descriptor_offset = data_descriptor_skinmesh->GetDescriptorOffset();
	for (int i = 0; i < descriptor_offset.size(); ++i)
	{
		m_commandList_skin->GetCommandList()->SetComputeRootDescriptorTable(i, descriptor_offset[i]);
	}
	*/
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