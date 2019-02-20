#include"PancyGeometryDx12.h"
using namespace PancystarEngine;
GeometryBasic::GeometryBasic()
{
	all_vertex = 0;
	all_index = 0;
	all_index_adj = 0;
	if_buffer_created = false;
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
PancystarEngine::EngineFailReason GeometryBasic::CheckGeometryState(ResourceStateType &now_state)
{
	PancystarEngine::EngineFailReason check_error;
	ResourceStateType state_Vbuffer;
	ResourceStateType state_Ibuffer;
	check_error = PancyBasicBufferControl::GetInstance()->GetResourceState(geometry_vertex_buffer, state_Vbuffer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancyBasicBufferControl::GetInstance()->GetResourceState(geometry_index_buffer, state_Ibuffer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if (state_Vbuffer == ResourceStateType::resource_state_not_init || state_Ibuffer == ResourceStateType::resource_state_not_init)
	{
		now_state = ResourceStateType::resource_state_not_init;
	}
	else if (state_Vbuffer == ResourceStateType::resource_state_load_GPU_memory_finish && state_Ibuffer == ResourceStateType::resource_state_load_GPU_memory_finish)
	{
		now_state = ResourceStateType::resource_state_load_GPU_memory_finish;
	}
	else 
	{
		now_state = ResourceStateType::resource_state_load_CPU_memory_finish;
	}
	return PancystarEngine::succeed;
}
GeometryBasic::~GeometryBasic()
{
	PancystarEngine::PancyBasicBufferControl::GetInstance()->DeleteResurceReference(geometry_vertex_buffer);
	PancystarEngine::PancyBasicBufferControl::GetInstance()->DeleteResurceReference(geometry_index_buffer);
	if (if_create_adj) 
	{
		PancystarEngine::PancyBasicBufferControl::GetInstance()->DeleteResurceReference(geometry_adjindex_buffer);
	}
}