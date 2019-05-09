#pragma once
#define AnimationSize
#include"PancyModelBasic.h"
#define threadBlockSize 128
namespace PancystarEngine 
{
	//描述符对象
	class DescriptorObject
	{
		//用于提交渲染的直接信息
		ID3D12PipelineState  *PSO_pointer;
		ID3D12RootSignature  *rootsignature;
		ID3D12DescriptorHeap *descriptor_heap_use;
		std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> descriptor_offset;
		//资源管理信息
		std::string PSO_name_descriptor;
		ResourceViewPack descriptor_block_id;
		pancy_object_id resource_view_num;
		std::unordered_map<std::string, PancystarEngine::PancyConstantBuffer*> per_object_cbuffer;//每个描述符独享的cbuffer，需要自主管理这片存储区域
		std::unordered_map<std::string, pancy_object_id> per_object_cbuffer_index;
	public:
		DescriptorObject();
		~DescriptorObject();
		ID3D12PipelineState  *GetPSO()
		{
			return PSO_pointer;
		}
		ID3D12RootSignature *GetRootSignature()
		{
			return rootsignature;
		}
		ID3D12DescriptorHeap *GetDescriptoHeap()
		{
			return descriptor_heap_use;
		}
		std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> GetDescriptorOffset()
		{
			return descriptor_offset;
		}
		PancystarEngine::EngineFailReason Create(
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
		);
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
	};
	//描述符管理器
	class DescriptorControl 
	{
		pancy_object_id now_object_id_top;
		std::queue<pancy_object_id> empty_object_id;
		std::vector<std::unordered_map<pancy_object_id, DescriptorObject *>> descriptor_data_map;
		DescriptorControl();
	public:
		~DescriptorControl();
		static DescriptorControl* GetInstance()
		{
			static DescriptorControl* this_instance;
			if (this_instance == NULL)
			{
				this_instance = new DescriptorControl();
			}
			return this_instance;
		}
		//创建计算着色描述符(不需要输入模型数据)
		PancystarEngine::EngineFailReason BuildDescriptorCompute(
			const pancy_object_id &PSO_id,
			const std::vector<std::string> &cbuffer_name_per_object_in,
			//可能会产生交换缓冲区的资源
			const std::vector<std::vector<PancystarEngine::PancyConstantBuffer *>> &cbuffer_per_frame_in,
			const std::vector<std::vector<SubMemoryPointer>> &SRV_per_frame_in,
			const std::vector<std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>> &SRV_desc_per_frame_in,
			const std::vector<std::vector<SubMemoryPointer>> &UAV_per_frame_in,
			const std::vector<std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC>> &UAV_desc_per_frame_in,
			pancy_object_id &descriptor_ID
		);
		//创建3D着色描述符(需要输入模型数据)
		PancystarEngine::EngineFailReason BuildDescriptorGraph(
			const pancy_object_id &model_id,
			const pancy_object_id &PSO_id,
			const std::vector<std::string> &cbuffer_name_per_object_in,
			//可能会产生交换缓冲区的资源
			const std::vector<std::vector<PancystarEngine::PancyConstantBuffer *>> &cbuffer_per_frame_in,
			const std::vector<std::vector<SubMemoryPointer>> &resource_data_per_frame_in,
			const std::vector<std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>> &resource_desc_per_frame_in,
			pancy_object_id &descriptor_ID
		);
		//获取描述符
		PancystarEngine::EngineFailReason GetDescriptor(const pancy_object_id &descriptor_id, DescriptorObject **descriptor_data);
		//删除描述符
		PancystarEngine::EngineFailReason DeleteDescriptor(const pancy_object_id &descriptor_id);
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
		
		PancystarEngine::EngineFailReason BuildDescriptor(
			const pancy_object_id &mesh_buffer,
			const UINT &vertex_num,
			const UINT &per_vertex_size,
			pancy_object_id &descriptor_id
		);
		//获取蒙皮结果缓冲区
		PancystarEngine::EngineFailReason GetSkinAnimationBuffer(std::vector<SubMemoryPointer> &skin_animation_data, pancy_resource_size &animation_buffer_size_in);
		//清空当前帧的缓冲区使用信息
		void ClearUsedBuffer();
		//填充渲染commandlist
		PancystarEngine::EngineFailReason BuildCommandList(
			const pancy_object_id &mesh_buffer,
			const pancy_object_id &vertex_num,
			const pancy_object_id &descriptor_id,
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
