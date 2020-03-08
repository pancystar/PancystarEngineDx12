#pragma once
#include"PancyBufferDx12.h"
#include"PancyRenderParam.h"
namespace PancystarEngine 
{
	//骨骼动画缓冲区块
	struct SkinAnimationBlock
	{
		pancy_resource_size start_pos;
		pancy_resource_size block_size;
	};
	//顶点动画数据
	struct mesh_animation_data
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT3 tangent;
		//float delta_time;
		mesh_animation_data()
		{
			position = DirectX::XMFLOAT3(0, 0, 0);
			normal = DirectX::XMFLOAT3(0, 0, 0);
			tangent = DirectX::XMFLOAT3(0, 0, 0);
		}
	};
	//骨骼动画缓冲区
	class PancySkinAnimationBuffer
	{
		//缓冲区的大小
		pancy_resource_size animation_buffer_size;//存储蒙皮结果的缓冲区的大小
		pancy_resource_size bone_buffer_size;//存储骨骼矩阵的缓冲区的大小

		//当前已经被占用的指针位置
		pancy_resource_size now_used_position_animation;//当前动画结果缓冲区的使用情况指针
		pancy_resource_size now_used_position_bone;//当前骨骼矩阵缓冲区的使用情况指针

		//存储每一个骨骼动画的Compute Shader计算位置
		std::unordered_map<pancy_object_id, SkinAnimationBlock> animation_block_map;
		//存储每一个骨骼矩阵区域的起始位置
		std::unordered_map<pancy_object_id, SkinAnimationBlock> bone_block_map;
		//骨骼动画缓冲区的数据
		VirtualResourcePointer buffer_animation;//动画结果缓冲区
		VirtualResourcePointer buffer_bone;     //骨骼矩阵缓冲区

		//骨骼数据的CPU指针
		UINT8* bone_data_pointer;
	public:
		PancySkinAnimationBuffer(const pancy_resource_size &animation_buffer_size_in, const pancy_resource_size &bone_buffer_size_in);
		~PancySkinAnimationBuffer();
		PancystarEngine::EngineFailReason Create();
		//清空当前所有使用的骨骼动画数据(由于动画数据逐帧重置，不需要考虑随机寻址类型的增删查改)
		void ClearUsedBuffer();
		//从当前蒙皮结果缓冲区中请求一块数据区(蒙皮结果数据区由GPU填充数据，因而只需要开辟)
		PancystarEngine::EngineFailReason BuildAnimationBlock(
			const pancy_resource_size &vertex_num,
			pancy_object_id &block_id,
			SkinAnimationBlock &new_animation_block
		);
		//从当前骨骼矩阵缓冲区中请求一块数据区(骨骼矩阵数据区由CPU填充数据，因而需要将填充数据一并传入)
		PancystarEngine::EngineFailReason BuildBoneBlock(
			const pancy_resource_size &matrix_num,
			const DirectX::XMFLOAT4X4 *matrix_data,
			pancy_object_id &block_id,
			SkinAnimationBlock &new_bone_block
		);
		//获取矩阵存储缓冲区
		inline VirtualResourcePointer& GetBoneMatrixResource()
		{
			return buffer_bone;
		}
		//获取蒙皮结果缓冲区
		inline VirtualResourcePointer& GetSkinVertexResource()
		{
			return buffer_animation;
		}
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
		//清空当前帧的缓冲区使用信息
		void ClearUsedBuffer();
		//填充渲染commandlist
		PancystarEngine::EngineFailReason BuildCommandList(
			const VirtualResourcePointer &mesh_buffer,
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