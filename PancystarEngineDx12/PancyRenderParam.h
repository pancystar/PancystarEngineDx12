#pragma once
#include"PancyDescriptor.h"
#include"PancyShaderDx12.h"
namespace PancystarEngine
{
	class BasicRenderParam
	{
		std::string render_param_name;
		//渲染管线
		std::string          PSO_name;
		ID3D12PipelineState  *PSO_pointer = NULL;
		ID3D12RootSignature  *rootsignature = NULL;
		//描述符堆
		ID3D12DescriptorHeap *descriptor_heap_use = NULL;
		//渲染所需的描述符数量，用于判断当前的渲染单元是否已经注册完毕
		bool if_render_param_inited = false;
		pancy_object_id globel_cbuffer_num = 99999;
		pancy_object_id private_cbuffer_num = 99999;
		pancy_object_id globel_shader_resource_num = 99999;
		pancy_object_id bind_shader_resource_num = 99999;
		pancy_object_id bindless_shader_resource_num = 99999;
		//渲染所需的描述符数据
		std::unordered_map<std::string, BindDescriptorPointer> globel_constant_buffer;       //全局常量缓冲区
		std::unordered_map<std::string, BindDescriptorPointer> private_constant_buffer;      //私有常量缓冲区
		std::unordered_map<std::string, BindDescriptorPointer> globel_shader_resource;       //全局描述符
		std::unordered_map<std::string, BindDescriptorPointer> bind_shader_resource;         //私有描述符
		std::unordered_map<std::string, BindlessDescriptorPointer> bindless_shader_resource; //解绑定描述符
		//渲染需要绑定的rootsignature slot
		std::unordered_map<std::string, pancy_object_id> globel_constant_buffer_root_signature_offset;    //全局常量缓冲区slot
		std::unordered_map<std::string, pancy_object_id> private_constant_buffer_root_signature_offset;   //私有常量缓冲区slot
		std::unordered_map<std::string, pancy_object_id> globel_shader_resource_root_signature_offset;    //全局描述符slot
		std::unordered_map<std::string, pancy_object_id> bind_shader_resource_root_signature_offset;      //私有描述符slot
		std::unordered_map<std::string, pancy_object_id> bindless_shader_resource_root_signature_offset;  //解绑定描述符slot
		//私有存储资源
		std::unordered_map<std::string, std::vector<PancyConstantBuffer*>> per_object_cbuffer;//每个描述符独享的cbuffer，需要自主管理这片存储区域
	public:
		BasicRenderParam(const std::string &render_param_name_in);
		~BasicRenderParam();
		PancystarEngine::EngineFailReason GetPsoData(ID3D12PipelineState  **pso_data);
		PancystarEngine::EngineFailReason SetCbufferMatrix(
			const std::string &cbuffer_name,
			const std::string &variable_name,
			const DirectX::XMFLOAT4X4 &data_in,
			const pancy_resource_size &offset
		);
		PancystarEngine::EngineFailReason SetCbufferFloat4(
			const std::string &cbuffer_name,
			const std::string &variable_name,
			const DirectX::XMFLOAT4 &data_in,
			const pancy_resource_size &offset
		);
		PancystarEngine::EngineFailReason SetCbufferUint4(
			const std::string &cbuffer_name,
			const std::string &variable_name,
			const DirectX::XMUINT4 &data_in,
			const pancy_resource_size &offset
		);
		PancystarEngine::EngineFailReason SetCbufferStructData(
			const std::string &cbuffer_name,
			const std::string &variable_name,
			const void* data_in,
			const pancy_resource_size &data_size,
			const pancy_resource_size &offset
		);
		PancystarEngine::EngineFailReason CommonCreate(
			const std::string &PSO_name,
			const std::unordered_map<std::string, BindDescriptorPointer> &bind_shader_resource_in,
			const std::unordered_map<std::string, BindlessDescriptorPointer> &bindless_shader_resource_in
		);
		PancystarEngine::EngineFailReason AddToCommandList(PancyRenderCommandList *m_commandList, const D3D12_COMMAND_LIST_TYPE &render_param_type);
	private:
		//绑定描述符到渲染管线
		PancystarEngine::EngineFailReason BindDescriptorToRootsignature(
			const PancyDescriptorType &bind_descriptor_type,
			const std::unordered_map<std::string, BindDescriptorPointer> &descriptor_data,
			const std::unordered_map<std::string, pancy_object_id> &root_signature_slot_data,
			const D3D12_COMMAND_LIST_TYPE &render_param_type,
			PancyRenderCommandList *m_commandList
		);
		//绑定解绑顶描述符到渲染管线
		PancystarEngine::EngineFailReason BindBindlessDescriptorToRootsignature(
			const PancyDescriptorType &bind_descriptor_type,
			const std::unordered_map<std::string, BindlessDescriptorPointer> &descriptor_data,
			const std::unordered_map<std::string, pancy_object_id> &root_signature_slot_data,
			const D3D12_COMMAND_LIST_TYPE &render_param_type,
			PancyRenderCommandList *m_commandList
		);
		//检查当前的渲染单元是否已经注册完毕
		bool CheckIfInitFinished();
	};

	struct PancyRenderParamID
	{
		//渲染状态ID号
		pancy_object_id PSO_id;
		pancy_object_id render_param_id;
	};
	class RenderParamSystem
	{
		//存储每一个pso为不同的输入参数所分配的描述符表单
		std::unordered_map<pancy_object_id, pancy_object_id> render_param_id_self_add;
		std::unordered_map<pancy_object_id, std::queue<pancy_object_id>> render_param_id_reuse_table;
		std::unordered_map<pancy_object_id, std::unordered_map<std::string, pancy_object_id>> render_param_name_table;
		std::unordered_map<pancy_object_id, std::unordered_map<pancy_object_id, BasicRenderParam*>> render_param_table;
	private:
		RenderParamSystem();
	public:
		static RenderParamSystem* GetInstance()
		{
			static RenderParamSystem* this_instance;
			if (this_instance == NULL)
			{
				this_instance = new RenderParamSystem();
			}
			return this_instance;
		}
		~RenderParamSystem();
		PancystarEngine::EngineFailReason GetCommonRenderParam(
			const std::string &PSO_name,
			const std::string &render_param_name,
			const std::unordered_map<std::string, BindDescriptorPointer> &bind_shader_resource_in,
			const std::unordered_map<std::string, BindlessDescriptorPointer> &bindless_shader_resource_in,
			PancyRenderParamID &render_param_id
		);
		PancystarEngine::EngineFailReason AddRenderParamToCommandList(
			const PancyRenderParamID &renderparam_id,
			PancyRenderCommandList *m_commandList,
			const D3D12_COMMAND_LIST_TYPE &render_param_type
		);
		PancystarEngine::EngineFailReason GetPsoData(
			const PancyRenderParamID &renderparam_id,
			ID3D12PipelineState  **pso_data
		);
		PancystarEngine::EngineFailReason SetCbufferMatrix(
			const PancyRenderParamID &renderparam_id,
			const std::string &cbuffer_name,
			const std::string &variable_name,
			const DirectX::XMFLOAT4X4 &data_in,
			const pancy_resource_size &offset
		);
		PancystarEngine::EngineFailReason SetCbufferFloat4(
			const PancyRenderParamID &renderparam_id,
			const std::string &cbuffer_name,
			const std::string &variable_name,
			const DirectX::XMFLOAT4 &data_in,
			const pancy_resource_size &offset
		);
		PancystarEngine::EngineFailReason SetCbufferUint4(
			const PancyRenderParamID &renderparam_id,
			const std::string &cbuffer_name,
			const std::string &variable_name,
			const DirectX::XMUINT4 &data_in,
			const pancy_resource_size &offset
		);
		PancystarEngine::EngineFailReason SetCbufferStructData(
			const PancyRenderParamID &renderparam_id,
			const std::string &cbuffer_name,
			const std::string &variable_name,
			const void* data_in,
			const pancy_resource_size &data_size,
			const pancy_resource_size &offset
		);
		//todo:使用引用计数删除不需要的渲染单元
		PancystarEngine::EngineFailReason DeleteCommonRenderParam(PancyRenderParamID &render_param_id);
	private:
		PancystarEngine::EngineFailReason GetResource(const PancyRenderParamID &renderparam_id, BasicRenderParam** data_pointer);
	};
	//全局骨骼动画控制器
	class PancySkinAnimationControl
	{
		pancy_resource_size animation_buffer_size;                    //存储动画结果的缓冲区大小
		pancy_resource_size bone_buffer_size;                         //存储骨骼矩阵的缓冲区大小
		pancy_object_id PSO_skinmesh;                                 //骨骼动画的渲染状态表
		std::vector<PancySkinAnimationBuffer*> skin_naimation_buffer; //骨骼动画的缓冲区信息
		PancySkinAnimationControl(
			const pancy_resource_size &animation_buffer_size_in,
			const pancy_resource_size &bone_buffer_size_in
		);
		PancystarEngine::EngineFailReason Create();
	public:
		static PancySkinAnimationControl *this_instance;
		static PancystarEngine::EngineFailReason SingleCreate(
			const pancy_resource_size &animation_buffer_size_in,
			const pancy_resource_size &bone_buffer_size_in
		)
		{
			if (this_instance != NULL)
			{

				PancystarEngine::EngineFailReason error_message(E_FAIL, "the d3d input instance have been created before");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Create directx input object", error_message);
				return error_message;
			}
			else
			{
				this_instance = new PancySkinAnimationControl(animation_buffer_size_in, bone_buffer_size_in);
				PancystarEngine::EngineFailReason check_error = this_instance->Create();
				return check_error;
			}
		}
		static PancySkinAnimationControl * GetInstance()
		{
			return this_instance;
		}
		/*
		PancystarEngine::EngineFailReason BuildDescriptor(
			const pancy_object_id &mesh_buffer,
			const UINT &vertex_num,
			const UINT &per_vertex_size,
			pancy_object_id &descriptor_id
		);

		//获取蒙皮结果缓冲区
		PancystarEngine::EngineFailReason GetSkinAnimationBuffer(std::vector<SubMemoryPointer> &skin_animation_data, pancy_resource_size &animation_buffer_size_in);
		*/
		//清空当前帧的缓冲区使用信息
		void ClearUsedBuffer();
		//填充渲染commandlist
		PancystarEngine::EngineFailReason BuildCommandList(
			const pancy_object_id &mesh_buffer,
			const pancy_object_id &vertex_num,
			const PancyRenderParamID &render_param_id,
			const pancy_resource_size &matrix_num,
			const DirectX::XMFLOAT4X4 *matrix_data,
			SkinAnimationBlock &new_animation_block,
			PancyRenderCommandList *m_commandList_skin
		);
		~PancySkinAnimationControl();
	private:
		//从当前蒙皮结果缓冲区中请求一块数据区(蒙皮结果数据区由GPU填充数据，因而只需要开辟)
		PancystarEngine::EngineFailReason BuildAnimationBlock(
			const pancy_resource_size &vertex_num,
			pancy_object_id &block_id,
			SkinAnimationBlock &animation_block_pos
		);
		//从当前骨骼矩阵缓冲区中请求一块数据区(骨骼矩阵数据区由CPU填充数据，因而需要将填充数据一并传入)
		PancystarEngine::EngineFailReason BuildBoneBlock(
			const pancy_resource_size &matrix_num,
			const DirectX::XMFLOAT4X4 *matrix_data,
			pancy_object_id &block_id,
			SkinAnimationBlock &new_bone_block
		);
	};
}