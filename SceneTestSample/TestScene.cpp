#include"TestScene.h"
using namespace PancystarEngine;
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
PancystarEngine::EngineFailReason scene_test_simple::BuildGlobelTextureSRV(const std::string &shader_resource, VirtualResourcePointer &tex_res_id)
{
	PancystarEngine::EngineFailReason check_error;
	std::vector<BasicDescriptorDesc> globel_descriptor_desc_in;
	std::vector<VirtualResourcePointer> globelmemory_data;
	BasicDescriptorDesc tex_bind_SRV_desc;
	tex_bind_SRV_desc.basic_descriptor_type = PancyDescriptorType::DescriptorTypeShaderResourceView;
	
	check_error = LoadDDSTextureResource(shader_resource, tex_res_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	auto texture_gpu_resource = dynamic_cast<PancyBasicTexture*>(tex_res_id.GetResourceData());
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	tex_bind_SRV_desc.shader_resource_view_desc = texture_gpu_resource->GetSRVDesc();
	globel_descriptor_desc_in.push_back(tex_bind_SRV_desc);
	globelmemory_data.push_back(tex_res_id);
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCommonGlobelDescriptor(shader_resource, globel_descriptor_desc_in, globelmemory_data, false);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	globel_descriptor_desc_in.clear();
	globelmemory_data.clear();
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
	check_error = LoadDDSTextureResource("data\\IrradianceMap.dds", tex_ibl_diffuse_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = BuildGlobelTextureSRV("environment_IBL_diffuse", tex_ibl_diffuse_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//读取空白纹理
	check_error = LoadDDSTextureResource("data\\white.dds", tex_empty_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//读取pbr纹理
	check_error = LoadDDSTextureResource("data\\Cubemap.json", tex_ibl_spec_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//加载需要的pbr纹理
	check_error = BuildGlobelTextureSRV("environment_IBL_spec", tex_ibl_spec_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PretreatBrdf();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = BuildGlobelTextureSRV("environment_brdf", tex_brdf_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//todo:临时使用的模型测试代码
	check_error = model_common.Create("model\\export\\multiball\\multiball.json");
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = model_skinmesh.Create("model\\export\\lion\\lion.json");
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	/*
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
	
	PancyFenceIdGPU broken_fence;
	check_error = ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY)->SetGpuBrokenFence(broken_fence);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	*/

	
	//加载一个pso
	check_error = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_test.json", PSO_test);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	
	check_error = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_pbr.json", PSO_pbr);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}

	/*
	//模型加载测试
	SubresourceControl::GetInstance()->WriteSubMemoryMessageToFile("memory_log.json");

	PancystarEngine::ResourceStateType now_id_state;
	check_error = PancystarEngine::PancyModelControl::GetInstance()->GetResourceState(model_skinmesh, now_id_state);
	SubresourceControl::GetInstance()->WriteSubMemoryMessageToFile("memory_log4.json");
	*/
	//调用一次骨骼动画单例，完成全局缓冲区的注册
	PancystarEngine::PancySkinAnimationControl::GetInstance();

	check_error = BuildSkinmeshComputeDescriptor();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = BuildSkinmeshDescriptor();
	
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::BuildSkinmeshDescriptor()
{
	PancystarEngine::EngineFailReason check_error;
	//加载骨骼动画的描述符信息
	std::vector<PancyConstantBuffer *> now_used_cbuffer;
	check_error = GetGlobelCbuffer(PSO_test, "per_frame", now_used_cbuffer);
	//获取模型的描述符数据(todo:后面要改成模型材质分离的格式)
	std::vector<VirtualResourcePointer> resource_data_per_frame;
	std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> resource_desc_per_data;
	check_error = model_skinmesh.GetShaderResourcePerObject(
		resource_data_per_frame,
		resource_desc_per_data
	);
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCommonBindlessShaderResourceView(resource_desc_per_data, resource_data_per_frame, resource_desc_per_data.size(), model_skinmesh_descriptor_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	std::unordered_map<std::string, BindDescriptorPointer> bind_shader_resource_in;
	std::unordered_map<std::string, BindlessDescriptorPointer> bindless_shader_resource_in;
	bindless_shader_resource_in.insert(std::pair<std::string, BindlessDescriptorPointer>("texture_model", model_skinmesh_descriptor_id));
	/*
	check_error = PancystarEngine::DescriptorControl::GetInstance()->BuildDescriptorGraph(
		model_skinmesh,
		PSO_pbr,
		cbuffer_name_perobj,
		cbuffer_data_perframe,
		globel_shader_resource,
		globel_shader_desc,
		skinmesh_descriptor
		);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	*/
	std::string pso_name_now;
	PancyEffectGraphic::GetInstance()->GetPSOName(PSO_pbr,pso_name_now);
	check_error = PancystarEngine::RenderParamSystem::GetInstance()->GetCommonRenderParam(
		pso_name_now,
		"test_skin_mesh_draw_pass",
		bind_shader_resource_in,
		bindless_shader_resource_in,
		render_param_id_skin_mesh_draw
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::BuildSkinmeshComputeDescriptor()
{
	PancystarEngine::EngineFailReason check_error;
	//获取作为输入的顶点缓冲区
	PancystarEngine::PancySubModel* model_resource_render;
	check_error = model_skinmesh.GetRenderMesh(0, &model_resource_render);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//根据输入顶点缓冲区创建一个蒙皮计算描述符
	BasicDescriptorDesc now_buffer_desc;
	now_buffer_desc.basic_descriptor_type = PancyDescriptorType::DescriptorTypeShaderResourceView;
	now_buffer_desc.shader_resource_view_desc = {};
	now_buffer_desc.shader_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	now_buffer_desc.shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;
	now_buffer_desc.shader_resource_view_desc.Buffer.StructureByteStride = sizeof(PancystarEngine::PointSkinCommon8);
	now_buffer_desc.shader_resource_view_desc.Buffer.NumElements = model_resource_render->GetVertexNum();
	now_buffer_desc.shader_resource_view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	VirtualResourcePointer now_buffer_data = model_resource_render->GetVertexBuffer();
	std::vector<BasicDescriptorDesc> now_descriptor_desc_in;
	std::vector<VirtualResourcePointer> memory_data;
	now_descriptor_desc_in.push_back(now_buffer_desc);
	memory_data.push_back(now_buffer_data);
	BindDescriptorPointer mesh_vertex_data_srv;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCommonDescriptor(now_descriptor_desc_in, memory_data,false, mesh_vertex_data_srv);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	std::unordered_map<std::string, BindDescriptorPointer> bind_shader_resource_in;
	std::unordered_map<std::string, BindlessDescriptorPointer> bindless_shader_resource_in;
	bind_shader_resource_in.insert(std::pair<std::string, BindDescriptorPointer>("vertex_data", mesh_vertex_data_srv));
	check_error = PancystarEngine::RenderParamSystem::GetInstance()->GetCommonRenderParam(
		"json\\pipline_state_object\\pso_skinmesh.json",
		"test_skin_mesh_compute_pass",
		bind_shader_resource_in,
		bindless_shader_resource_in,
		render_param_id_skin_mesh_compute
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	/*
	check_error = PancystarEngine::PancySkinAnimationControl::GetInstance()->BuildDescriptor(
		model_resource_render->GetVertexBuffer(),
		model_resource_render->GetVertexNum(),
		sizeof(PancystarEngine::PointSkinCommon8),
		skinmesh_compute_descriptor
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	*/
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
	PancyCommonTextureDesc new_texture_desc;
	new_texture_desc.heap_flag_in = D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES;
	new_texture_desc.heap_type = D3D12_HEAP_TYPE_DEFAULT;
	new_texture_desc.if_force_srgb = false;
	new_texture_desc.if_gen_mipmap = false;
	new_texture_desc.max_size = 0;
	new_texture_desc.texture_data_file = "";
	new_texture_desc.texture_res_desc = default_tex_RGB_desc;
	new_texture_desc.texture_type = PancyTextureType::Texture_Render_Target;
	check_error = BuildTextureResource("globel_brdf_texture", new_texture_desc, tex_brdf_id, false);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//创建渲染目标
	std::vector<BasicDescriptorDesc> now_descriptor_desc_in;
	std::vector<VirtualResourcePointer> memory_data;
	BasicDescriptorDesc rtv_brdf;
	rtv_brdf.basic_descriptor_type = PancyDescriptorType::DescriptorTypeRenderTargetView;
	rtv_brdf.render_target_view_desc = {};
	rtv_brdf.render_target_view_desc.Texture2D.MipSlice = 0;
	rtv_brdf.render_target_view_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtv_brdf.render_target_view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtv_brdf.render_target_view_desc.Texture2D.MipSlice = 0;
	rtv_brdf.render_target_view_desc.Texture2D.PlaneSlice = 0;
	now_descriptor_desc_in.push_back(rtv_brdf);
	memory_data.push_back(tex_brdf_id);
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCommonDescriptor(now_descriptor_desc_in, memory_data,false, brdf_rtv_id);
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
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	m_commandList->GetCommandList()->SetGraphicsRootSignature(rootsignature_data);

	auto brde_tex_res = GetTextureResourceData(tex_brdf_id,check_error);
	check_error = brde_tex_res->ResourceBarrier(m_commandList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//设置渲染目标
	std::vector<pancy_object_id> render_target_list;
	render_target_list.push_back(brdf_rtv_id.descriptor_id);
	check_error = PancyDescriptorHeapControl::GetInstance()->BindCommonRenderTargetUncontiguous(render_target_list, 0, m_commandList,true,false);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//todo：rendertarget的清空工作
	//const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	//m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	//渲染到纹理
	m_commandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &test_model->GetVertexBufferView());
	m_commandList->GetCommandList()->IASetIndexBuffer(&test_model->GetIndexBufferView());
	m_commandList->GetCommandList()->DrawIndexedInstanced(test_model->GetIndexNum(), 1, 0, 0, 0);

	check_error = brde_tex_res->ResourceBarrier(m_commandList, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
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
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::ShowSkinModel()
{
	PancystarEngine::EngineFailReason check_error;
	static float time = 0.0f;
	time += 0.01;
	//测试计算着色器进行骨骼蒙皮
	PancystarEngine::SkinAnimationBlock animation_block_pos;
	PancystarEngine::PancySkinAnimationControl::GetInstance()->ClearUsedBuffer();
	std::vector<DirectX::XMFLOAT4X4> matrix_out_bone;
	check_error = model_skinmesh.GetBoneByAnimation(4, time, matrix_out_bone);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	PancystarEngine::PancySubModel* model_resource_render;
	check_error = model_skinmesh.GetRenderMesh(0, &model_resource_render);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	
	PancyRenderCommandList *m_commandList_skin;
	PancyThreadIdGPU commdlist_id_skin;
	ID3D12PipelineState *pso_data;
	check_error = PancystarEngine::RenderParamSystem::GetInstance()->GetPsoData(render_param_id_skin_mesh_compute, &pso_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(pso_data, &m_commandList_skin, commdlist_id_skin);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancystarEngine::PancySkinAnimationControl::GetInstance()->BuildCommandList(
		model_resource_render->GetVertexBuffer(),
		model_resource_render->GetVertexNum(),
		render_param_id_skin_mesh_compute,
		matrix_out_bone.size(),
		&matrix_out_bone[0],
		animation_block_pos,
		m_commandList_skin
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(1, &commdlist_id_skin);
	
	//渲染骨骼动画
	/*
	PancystarEngine::DescriptorObject *data_descriptor_test;
	check_error = PancystarEngine::DescriptorControl::GetInstance()->GetDescriptor(
		skinmesh_descriptor,
		&data_descriptor_test
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}*/
	//为测试渲染描述符填充专用的cbuffer
	instance_value new_data;
	new_data.animation_index = DirectX::XMUINT4(animation_block_pos.start_pos/sizeof(PancystarEngine::mesh_animation_data), 0, 0, 0);
	DirectX::XMStoreFloat4x4(&new_data.world_mat, DirectX::XMMatrixScaling(0.1,0.1,0.1));
	check_error = PancystarEngine::RenderParamSystem::GetInstance()->SetCbufferStructData(render_param_id_skin_mesh_draw, "per_instance", "_Instances", &new_data, sizeof(new_data), 0);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//check_error = data_descriptor_test->SetCbufferStructData("per_instance", "_Instances", &new_data, sizeof(new_data), 0);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if (time > 1.0f) 
	{
		time -= 1.0f;
	}
	//绑定并制造commandlist
	PancyRenderCommandList *m_commandList;
	PancyThreadIdGPU commdlist_id_use;
	ID3D12PipelineState *pso_data_draw;
	check_error = PancystarEngine::RenderParamSystem::GetInstance()->GetPsoData(render_param_id_skin_mesh_draw, &pso_data_draw);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(pso_data_draw, &m_commandList, commdlist_id_use);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect);

	check_error = PancystarEngine::RenderParamSystem::GetInstance()->AddRenderParamToCommandList(render_param_id_skin_mesh_draw, m_commandList, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	/*
	m_commandList->GetCommandList()->SetGraphicsRootSignature(data_descriptor_test->GetRootSignature());
	ID3D12DescriptorHeap *descriptor_heap_id = data_descriptor_test->GetDescriptoHeap();
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &descriptor_heap_id);
	std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> descriptor_offset = data_descriptor_test->GetDescriptorOffset();
	for (int i = 0; i < descriptor_offset.size(); ++i)
	{
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(i, descriptor_offset[i]);
	}
	*/
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ComPtr<ID3D12Resource> screen_rendertarget = PancyDx12DeviceBasic::GetInstance()->GetBackBuffer(rtvHandle);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	//修改资源格式为dsv
	int32_t now_render_num = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	auto default_depth_tex_res = GetTextureResourceData(Default_depthstencil_buffer[now_render_num], check_error);
	check_error = default_depth_tex_res->ResourceBarrier(m_commandList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//获取深度缓冲区
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	BindDescriptorPointer dsv_descriptor_id;
	check_error = PancyDescriptorHeapControl::GetInstance()->GetCommonGlobelDescriptorID(PancyDescriptorType::DescriptorTypeDepthStencilView,"DefaultDepthBufferSRV", dsv_descriptor_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetCommonDepthStencilBufferOffset(dsv_descriptor_id.descriptor_id, dsvHandle);
	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	//设置渲染单元
	m_commandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &model_resource_render->GetVertexBufferView());
	m_commandList->GetCommandList()->IASetIndexBuffer(&model_resource_render->GetIndexBufferView());
	m_commandList->GetCommandList()->DrawIndexedInstanced(model_resource_render->GetIndexNum(), 1, 0, 0, 0);

	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	default_depth_tex_res->ResourceBarrier(m_commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON);
	m_commandList->UnlockPrepare();
	//提交渲染命令
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(1, &commdlist_id_use);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ShowModel();
void scene_test_simple::Display()
{
	if (model_skinmesh.CheckIfLoadSucceed())
	{
		renderlist_ID.clear();
		if (if_have_previous_frame)
		{
			auto check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->FreeAlloctor();
		}
		ClearScreen();
		auto check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(renderlist_ID.size(), &renderlist_ID[0]);
		last_broken_fence_id = broken_fence_id;

		//ShowFloor();
		ShowSkinModel();

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
	auto default_depth_tex_res = GetTextureResourceData(Default_depthstencil_buffer[now_render_num], check_error);
	check_error = default_depth_tex_res->ResourceBarrier(m_commandList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	//获取深度缓冲区
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	BindDescriptorPointer dsv_descriptor_id;
	check_error = PancyDescriptorHeapControl::GetInstance()->GetCommonGlobelDescriptorID(PancyDescriptorType::DescriptorTypeDepthStencilView, "DefaultDepthBufferSRV", dsv_descriptor_id);
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetCommonDepthStencilBufferOffset(dsv_descriptor_id.descriptor_id, dsvHandle);

	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	check_error = default_depth_tex_res->ResourceBarrier(m_commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON);
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
	PancyConstantBuffer *PSO_test_cbuffer, *PSO_pbr_cbuffer;
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