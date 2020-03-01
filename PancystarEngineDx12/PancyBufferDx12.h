#pragma once
#include"PancyResourceBasic.h"
#include"PancyThreadBasic.h"
namespace PancystarEngine 
{
	enum PancyBufferType 
	{
		Buffer_ShaderResource_static = 0,
		Buffer_ShaderResource_dynamic,
		Buffer_Constant,
		Buffer_Vertex,
		Buffer_Index,
		Buffer_UnorderedAccess_static
	};
	struct PancyCommonBufferDesc 
	{
		PancyBufferType buffer_type;
		D3D12_RESOURCE_DESC buffer_res_desc = {};
		std::string buffer_data_file;
	};
	class CommonBufferJsonReflect :public PancyJsonReflectTemplate<PancyCommonBufferDesc>
	{ 
	public:
		CommonBufferJsonReflect();
	private:
		PancystarEngine::EngineFailReason InitChildReflectClass() override;
		void InitBasicVariable() override;
	};
	CommonBufferJsonReflect::CommonBufferJsonReflect() 
	{
		
	}
	PancystarEngine::EngineFailReason CommonBufferJsonReflect::InitChildReflectClass()
	{
	}
	void CommonBufferJsonReflect::InitBasicVariable() 
	{
		Init_Json_Data_Vatriable(reflect_data.buffer_type);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Dimension);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Alignment);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Width);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Height);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.DepthOrArraySize);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.MipLevels);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Format);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.SampleDesc.Count);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.SampleDesc.Quality);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Layout);
		Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Flags);
		Init_Json_Data_Vatriable(reflect_data.buffer_data_file);
	}
	//缓冲区资源
	class PancyBasicBuffer : public PancyBasicVirtualResource
	{
		pancy_resource_size subresources_size = 0;
		UINT8* map_pointer = NULL;
		ResourceBlockGpu *buffer_data = nullptr;     //buffer数据指针
	public:
		PancyBasicBuffer(const bool &if_could_reload);
		~PancyBasicBuffer();
		inline const pancy_resource_size GetBufferSize() const
		{
			return subresources_size;
		}
		inline UINT8* GetBufferCPUPointer() const
		{
			return map_pointer;
		}
		inline ResourceBlockGpu *GetGpuResourceData() const
		{
			return buffer_data;
		}
		//检测当前的资源是否已经被载入GPU
		bool CheckIfResourceLoadFinish() override;
	private:
		void BuildJsonReflect(PancyJsonReflect **pointer_data) override;
		PancystarEngine::EngineFailReason InitResource() override;
	};

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
	
}