#include"TestScene.h"
struct instance_value
{
	DirectX::XMFLOAT4X4 world_mat;
	DirectX::XMUINT4 animation_index;
};
PancystarEngine::EngineFailReason scene_test_simple::ScreenChange()
{
	view_port.TopLeftX = 0;
	view_port.TopLeftY = 0;
	view_port.Width = static_cast<FLOAT>(Scene_width);
	view_port.Height = static_cast<FLOAT>(Scene_height);
	view_port.MaxDepth = 1.0f;
	view_port.MinDepth = 0.0f;
	view_rect.left = 0;
	view_rect.top = 0;
	view_rect.right = Scene_width;
	view_rect.bottom = Scene_height;
	return PancystarEngine::succeed;
}
void scene_test_simple::updateinput(float delta_time)
{
	float move_speed = 0.15f;
	auto user_input = PancyInput::GetInstance();
	auto scene_camera = PancyCamera::GetInstance();
	user_input->GetInput();
	if (user_input->CheckKeyboard(DIK_A))
	{
		scene_camera->WalkRight(-move_speed);
	}
	if (user_input->CheckKeyboard(DIK_W))
	{
		scene_camera->WalkFront(move_speed);
	}
	if (user_input->CheckKeyboard(DIK_R))
	{
		scene_camera->WalkUp(move_speed);
	}
	if (user_input->CheckKeyboard(DIK_D))
	{
		scene_camera->WalkRight(move_speed);
	}
	if (user_input->CheckKeyboard(DIK_S))
	{
		scene_camera->WalkFront(-move_speed);
	}
	if (user_input->CheckKeyboard(DIK_F))
	{
		scene_camera->WalkUp(-move_speed);
	}
	if (user_input->CheckKeyboard(DIK_Q))
	{
		scene_camera->RotationLook(0.001f);
	}
	if (user_input->CheckKeyboard(DIK_E))
	{
		scene_camera->RotationLook(-0.001f);
	}
	if (user_input->CheckMouseDown(1))
	{
		scene_camera->RotationUp(user_input->MouseMove_X() * 0.001f);
		scene_camera->RotationRight(user_input->MouseMove_Y() * 0.001f);
	}
}
PancystarEngine::EngineFailReason scene_test_simple::PretreatPbrDescriptor()
{
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::Init()
{
	PancystarEngine::EngineFailReason check_error;
	//创建全屏三角形
	PancystarEngine::Point2D point[4];
	point[0].position = DirectX::XMFLOAT4(-1.0f, -1.0f, 0.0f, 1.0f);
	point[1].position = DirectX::XMFLOAT4(-1.0f, +1.0f, 0.0f, 1.0f);
	point[2].position = DirectX::XMFLOAT4(+1.0f, +1.0f, 0.0f, 1.0f);
	point[3].position = DirectX::XMFLOAT4(+1.0f, -1.0f, 0.0f, 1.0f);
	point[0].tex_color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
	point[1].tex_color = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	point[2].tex_color = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
	point[3].tex_color = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 0.0f);
	UINT index[] = { 0,1,2 ,0,2,3 };
	test_model = new PancystarEngine::GeometryCommonModel<PancystarEngine::Point2D>(point, index, 4, 6);
	check_error = test_model->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}


	
	
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\IrradianceMap.json", tex_ibl_diffuse_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//读取空白纹理
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\white.json", tex_empty_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	
	//读取pbr纹理
	//加载需要的pbr纹理
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\Cubemap.json", tex_ibl_spec_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	
	check_error = PretreatBrdf();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	
	//创测试模型

	check_error = PancystarEngine::PancyModelControl::GetInstance()->LoadResource("model\\export\\multiball\\multiball.json", model_common);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	/*
	check_error = PancystarEngine::PancyModelControl::GetInstance()->LoadResource("model\\export\\lion\\lion.json", model_skinmesh);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancystarEngine::PancyModelControl::GetInstance()->LoadResource("model\\export\\treetest\\tree.json", model_pointmesh);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	*/
	PancyFenceIdGPU broken_fence;
	check_error = ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY)->SetGpuBrokenFence(broken_fence);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}



	//加载一个pso
	check_error = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_test.json", PSO_test);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//模型加载测试
	check_error = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_pbr.json", PSO_pbr);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	SubresourceControl::GetInstance()->WriteSubMemoryMessageToFile("memory_log.json");

	PancystarEngine::ResourceStateType now_id_state;
	check_error = PancystarEngine::PancyModelControl::GetInstance()->GetResourceState(model_common, now_id_state);
	//check_error = PancystarEngine::PancyModelControl::GetInstance()->GetResourceState(model_skinmesh, now_id_state);
	//check_error = PancystarEngine::PancyModelControl::GetInstance()->GetResourceState(model_pointmesh, now_id_state);
	/*
	if (check_error.CheckIfSucceed() && now_id_state == PancystarEngine::ResourceStateType::resource_state_load_GPU_memory_finish)
	{
		SubresourceControl::GetInstance()->WriteSubMemoryMessageToFile("memory_log2.json");
	}
	PancystarEngine::PancyModelControl::GetInstance()->DeleteResurceReference(model_common);
	PancystarEngine::PancyModelControl::GetInstance()->DeleteResurceReference(model_skinmesh);
	PancystarEngine::PancyModelControl::GetInstance()->DeleteResurceReference(model_pointmesh);
	SubresourceControl::GetInstance()->WriteSubMemoryMessageToFile("memory_log3.json");
	//重复创建测试
	check_error = PancystarEngine::PancyModelControl::GetInstance()->LoadResource("model\\export\\multiball\\multiball.json", model_common);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancystarEngine::PancyModelControl::GetInstance()->LoadResource("model\\export\\lion\\lion.json", model_skinmesh);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancystarEngine::PancyModelControl::GetInstance()->LoadResource("model\\export\\treetest\\tree.json", model_pointmesh);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}

	PancyFenceIdGPU broken_fence2;
	check_error = ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY)->SetGpuBrokenFence(broken_fence2);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}*/
	SubresourceControl::GetInstance()->WriteSubMemoryMessageToFile("memory_log4.json");

	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::PretreatBrdf()
{
	CD3DX12_VIEWPORT view_port_brdf;
	CD3DX12_RECT view_rect_brdf;
	view_port_brdf.TopLeftX = 0;
	view_port_brdf.TopLeftY = 0;
	view_port_brdf.Width = 1024.0;
	view_port_brdf.Height = 1024.0;
	view_port_brdf.MaxDepth = 1.0f;
	view_port_brdf.MinDepth = 0.0f;
	view_rect_brdf.left = 0;
	view_rect_brdf.top = 0;
	view_rect_brdf.right = 1024;
	view_rect_brdf.bottom = 1024;
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id PSO_brdfgen;
	check_error = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_brdfgen.json", PSO_brdfgen);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//加载brdf预处理纹理
	//todo:commandalloctor间隔帧需要两个线程池
	//todo：依靠resourcedesc来计算heap及分块的大小
	//done:2019.2.1
	//创建新的屏幕空间纹理格式
	D3D12_RESOURCE_DESC default_tex_RGB_desc;
	default_tex_RGB_desc.Alignment = 0;
	default_tex_RGB_desc.DepthOrArraySize = 1;
	default_tex_RGB_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	default_tex_RGB_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	default_tex_RGB_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	default_tex_RGB_desc.Height = 1024;
	default_tex_RGB_desc.Width = 1024;
	default_tex_RGB_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	default_tex_RGB_desc.MipLevels = 1;
	default_tex_RGB_desc.SampleDesc.Count = 1;
	default_tex_RGB_desc.SampleDesc.Quality = 0;
	std::vector<D3D12_HEAP_FLAGS> heap_flags;
	heap_flags.clear();
	heap_flags.push_back(D3D12_HEAP_FLAG_DENY_BUFFERS);
	heap_flags.push_back(D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES);
	std::string subres_name;
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->BuildTextureTypeJson(default_tex_RGB_desc, 1, D3D12_HEAP_TYPE_DEFAULT, heap_flags, D3D12_RESOURCE_STATE_COMMON, subres_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	std::string RGB8_file_data = "json\\texture\\1024_1024_R16B16G16A16FLOAT.json";
	if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(RGB8_file_data))
	{
		Json::Value json_data_out;
		//填充资源格式
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfFromFile", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFSRV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfRTV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFUAV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFDSV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "SubResourceFile", subres_name);
		//写入文件并标记为已创建
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, RGB8_file_data);
		//将文件标记为已经创建
		PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(RGB8_file_data);
	}



	SubMemoryPointer texture_brdf_need;
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("json\\texture\\1024_1024_R16B16G16A16FLOAT.json", tex_brdf_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//创建渲染目标
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_brdf_id, texture_brdf_need);
	D3D12_RENDER_TARGET_VIEW_DESC RTV_desc;
	D3D12_RESOURCE_DESC res_desc;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexDesc(tex_brdf_id, res_desc);
	RTV_desc.Texture2D.MipSlice = 0;
	RTV_desc.Format = res_desc.Format;
	RTV_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	RTV_desc.Texture2D.MipSlice = 0;
	RTV_desc.Texture2D.PlaneSlice = 0;

	std::string dsv_descriptor_name = "json\\descriptor_heap\\RTV_1_descriptor_heap.json";
	ResourceViewPointer RTV_pointer;
	pancy_object_id rsv_pack_size;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(dsv_descriptor_name, RTV_pointer.resource_view_pack_id, rsv_pack_size);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	RTV_pointer.resource_view_offset_id = 0;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildRTV(RTV_pointer, texture_brdf_need, RTV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//设置预渲染参数
	PancyRenderCommandList *m_commandList;
	ID3D12PipelineState *PSO_res_brdfgen;
	auto pso_data = PancyEffectGraphic::GetInstance()->GetPSOResource(PSO_brdfgen, &PSO_res_brdfgen);
	PancyThreadIdGPU commdlist_id_use;
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(PSO_res_brdfgen, &m_commandList, commdlist_id_use);
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port_brdf);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect_brdf);

	ID3D12RootSignature *rootsignature_data;
	check_error = PancyEffectGraphic::GetInstance()->GetRootSignatureResource(PSO_brdfgen, &rootsignature_data);
	m_commandList->GetCommandList()->SetGraphicsRootSignature(rootsignature_data);
	//设置渲染目标
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	SubresourceControl::GetInstance()->ResourceBarrier(m_commandList, texture_brdf_need, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(RTV_pointer, rtvHandle);

	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, NULL);
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	//渲染到纹理
	m_commandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &test_model->GetVertexBufferView());
	m_commandList->GetCommandList()->IASetIndexBuffer(&test_model->GetIndexBufferView());
	m_commandList->GetCommandList()->DrawIndexedInstanced(test_model->GetIndexNum(), 1, 0, 0, 0);

	SubresourceControl::GetInstance()->ResourceBarrier(m_commandList, texture_brdf_need, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->UnlockPrepare();
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(1, &commdlist_id_use);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::ShowFloor()
{
	PancystarEngine::EngineFailReason check_error;
	//清空模型上一帧使用的描述符表
	//todo:自动化清空操作
	PancystarEngine::PancyModelControl::GetInstance()->ResetModelRenderDescriptor(model_common);
	//获取一个测试渲染描述符
	std::vector<std::string> cbuffer_name_perobj;
	cbuffer_name_perobj.push_back("per_instance");
	std::vector<PancystarEngine::PancyConstantBuffer *> cbuffer_data_perframe;
	PancystarEngine::PancyConstantBuffer *now_used_cbuffer;
	check_error = GetGlobelCbuffer(PSO_test, "per_frame", &now_used_cbuffer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	cbuffer_data_perframe.push_back(now_used_cbuffer);
	std::vector<SubMemoryPointer> globel_shader_resource;
	std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> globel_shader_desc;
	//绑定全局纹理信息
	SubMemoryPointer tex_bind_submemory;
	D3D12_SHADER_RESOURCE_VIEW_DESC tex_bind_SRV_desc;
	D3D12_RESOURCE_DESC tex_bind_desc;
	
	//镜面反射IBL
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_ibl_spec_id, tex_bind_submemory);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	globel_shader_resource.push_back(tex_bind_submemory);
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_ibl_spec_id, tex_bind_SRV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	globel_shader_desc.push_back(tex_bind_SRV_desc);
	//漫反射IBL
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_ibl_diffuse_id, tex_bind_submemory);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	globel_shader_resource.push_back(tex_bind_submemory);
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_ibl_diffuse_id, tex_bind_SRV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	globel_shader_desc.push_back(tex_bind_SRV_desc);
	//预处理的全局BRDF
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_brdf_id, tex_bind_submemory);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	globel_shader_resource.push_back(tex_bind_submemory);
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_brdf_id, tex_bind_SRV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	globel_shader_desc.push_back(tex_bind_SRV_desc);
	//空纹理，由于不需要动画数据缓冲
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_empty_id, tex_bind_submemory);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	globel_shader_resource.push_back(tex_bind_submemory);
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_empty_id, tex_bind_SRV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	globel_shader_desc.push_back(tex_bind_SRV_desc);
	PancystarEngine::DescriptorObject *data_descriptor_test;
	check_error = PancystarEngine::PancyModelControl::GetInstance()->GetRenderDescriptor(
		model_common,
		PSO_pbr,
		cbuffer_name_perobj,
		cbuffer_data_perframe,
		globel_shader_resource,
		globel_shader_desc,
		&data_descriptor_test
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//为测试渲染描述符填充专用的cbuffer
	instance_value new_data;
	new_data.animation_index = DirectX::XMUINT4(0, 0, 0, 0);
	DirectX::XMStoreFloat4x4(&new_data.world_mat, DirectX::XMMatrixIdentity());
	check_error = data_descriptor_test->SetCbufferStructData("per_instance", "_Instances", &new_data, sizeof(new_data), 0);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	/*
	DirectX::XMFLOAT4X4 mat_scal;
	DirectX::XMStoreFloat4x4(&mat_scal,DirectX::XMMatrixScaling(1, 1, 1));
	check_error = data_descriptor_test->SetCbufferMatrix("per_object", "world_matrix", mat_scal, 0);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	DirectX::XMFLOAT4X4 uv_scal;
	DirectX::XMStoreFloat4x4(&uv_scal, DirectX::XMMatrixScaling(1, 1, 1));
	check_error = data_descriptor_test->SetCbufferMatrix("per_object", "UV_matrix", uv_scal, 0);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	*/


	//绑定并制造commandlist
	PancyRenderCommandList *m_commandList;
	PancyThreadIdGPU commdlist_id_use;
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(data_descriptor_test->GetPSO(), &m_commandList, commdlist_id_use);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect);
	m_commandList->GetCommandList()->SetGraphicsRootSignature(data_descriptor_test->GetRootSignature());
	ID3D12DescriptorHeap *descriptor_heap_id = data_descriptor_test->GetDescriptoHeap();
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &descriptor_heap_id);
	std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> descriptor_offset = data_descriptor_test->GetDescriptorOffset();
	for (int i = 0; i < descriptor_offset.size(); ++i)
	{
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(i, descriptor_offset[i]);
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ComPtr<ID3D12Resource> screen_rendertarget = PancyDx12DeviceBasic::GetInstance()->GetBackBuffer(rtvHandle);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	//修改资源格式为dsv
	int32_t now_render_num = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	SubMemoryPointer sub_res_dsv;
	int64_t per_memory_size;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(Default_depthstencil_buffer[now_render_num], sub_res_dsv);
	SubresourceControl::GetInstance()->ResourceBarrier(m_commandList, sub_res_dsv, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	//获取深度缓冲区
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(Default_depthstencil_view[now_render_num], dsvHandle);
	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	//设置渲染单元
	m_commandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	PancystarEngine::PancySubModel* model_resource_render;
	check_error = PancystarEngine::PancyModelControl::GetInstance()->GetRenderMesh(model_common, 0, &model_resource_render);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &model_resource_render->GetVertexBufferView());
	m_commandList->GetCommandList()->IASetIndexBuffer(&model_resource_render->GetIndexBufferView());
	m_commandList->GetCommandList()->DrawIndexedInstanced(model_resource_render->GetIndexNum(), 1, 0, 0, 0);

	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	SubresourceControl::GetInstance()->ResourceBarrier(m_commandList, sub_res_dsv, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->UnlockPrepare();
	//提交渲染命令
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(1, &commdlist_id_use);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ShowModel();
void scene_test_simple::Display()
{
	PancystarEngine::ResourceStateType now_id_state;
	auto check_error = PancystarEngine::PancyModelControl::GetInstance()->GetResourceState(model_common, now_id_state);
	if (check_error.CheckIfSucceed() && now_id_state == PancystarEngine::ResourceStateType::resource_state_load_GPU_memory_finish)
	{

		HRESULT hr;
		renderlist_ID.clear();
		if (if_have_previous_frame)
		{
			auto check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->FreeAlloctor();
		}
		ClearScreen();
		check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(renderlist_ID.size(), &renderlist_ID[0]);
		last_broken_fence_id = broken_fence_id;

		ShowFloor();


		ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SetGpuBrokenFence(broken_fence_id);
		check_error = PancyDx12DeviceBasic::GetInstance()->SwapChainPresent(1, 0);
		if (if_have_previous_frame)
		{
			WaitForPreviousFrame();
		}
		if_have_previous_frame = true;
	}

}
void scene_test_simple::DisplayEnvironment(DirectX::XMFLOAT4X4 view_matrix, DirectX::XMFLOAT4X4 proj_matrix)
{
}
void scene_test_simple::ClearScreen()
{
	PancyRenderCommandList *m_commandList;
	PancyThreadIdGPU commdlist_id_use;
	auto check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(NULL, &m_commandList, commdlist_id_use);
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ComPtr<ID3D12Resource> screen_rendertarget = PancyDx12DeviceBasic::GetInstance()->GetBackBuffer(rtvHandle);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	//修改资源格式为dsv
	int32_t now_render_num = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	SubMemoryPointer sub_res_dsv;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(Default_depthstencil_buffer[now_render_num], sub_res_dsv);
	SubresourceControl::GetInstance()->ResourceBarrier(m_commandList, sub_res_dsv, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	//获取深度缓冲区
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(Default_depthstencil_view[now_render_num], dsvHandle);

	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	SubresourceControl::GetInstance()->ResourceBarrier(m_commandList, sub_res_dsv, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->UnlockPrepare();
	renderlist_ID.push_back(commdlist_id_use);
}
void scene_test_simple::WaitForPreviousFrame()
{
	auto  check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->WaitGpuBrokenFence(last_broken_fence_id);
}
void scene_test_simple::Update(float delta_time)
{
	updateinput(delta_time);
	PancystarEngine::EngineFailReason check_error;
	PancystarEngine::PancyConstantBuffer *PSO_test_cbuffer, *PSO_pbr_cbuffer;
	check_error = GetGlobelCbuffer(PSO_test, "per_frame", &PSO_test_cbuffer);
	check_error = GetGlobelCbuffer(PSO_pbr, "per_frame", &PSO_pbr_cbuffer);
	if (check_error.CheckIfSucceed())
	{
		//更新每帧更变的变量
		DirectX::XMMATRIX proj_mat = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 1000.0f);
		DirectX::XMFLOAT4X4 view_matrix, proj_matrix, view_proj_matrix, invview_matrix;
		DirectX::XMFLOAT4 view_pos;
		DirectX::XMStoreFloat4x4(&proj_matrix, proj_mat);
		PancyCamera::GetInstance()->CountViewMatrix(&view_matrix);
		PancyCamera::GetInstance()->CountInvviewMatrix(&invview_matrix);
		PancyCamera::GetInstance()->GetViewPosition(&view_pos);
		DirectX::XMStoreFloat4x4(&view_proj_matrix, DirectX::XMLoadFloat4x4(&view_matrix)*proj_mat);
		PSO_test_cbuffer->SetMatrix("view_matrix", view_matrix, 0);
		PSO_test_cbuffer->SetMatrix("projectmatrix", proj_matrix, 0);
		PSO_test_cbuffer->SetMatrix("view_projectmatrix", view_proj_matrix, 0);
		PSO_test_cbuffer->SetMatrix("invview_matrix", invview_matrix, 0);
		PSO_test_cbuffer->SetFloat4("view_position", view_pos, 0);

		PSO_pbr_cbuffer->SetMatrix("view_matrix", view_matrix, 0);
		PSO_pbr_cbuffer->SetMatrix("projectmatrix", proj_matrix, 0);
		PSO_pbr_cbuffer->SetMatrix("view_projectmatrix", view_proj_matrix, 0);
		PSO_pbr_cbuffer->SetMatrix("invview_matrix", invview_matrix, 0);
		PSO_pbr_cbuffer->SetFloat4("view_position", view_pos, 0);
	}
}
scene_test_simple::~scene_test_simple()
{
	WaitForPreviousFrame();
	delete test_model;
}