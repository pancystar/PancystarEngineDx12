#pragma once
#include"PancyTextureDx12.h"
#include"PancyGeometryDx12.h"
#include"PancyShaderDx12.h"
namespace PancystarEngine
{
#define MaxBoneNum 100
#define NouseBoneStruct -12138
#define VertexAnimationID uint32_t
//GPU骨骼计算最大的迭代次数
#define MaxSkinDeSampleTime 8
	enum TexType
	{
		tex_diffuse = 0,
		tex_normal,
		tex_metallic_roughness,
		tex_metallic,
		tex_roughness,
		tex_specular_smoothness,
		tex_ambient,
		tex_subsurface_color,
		tex_subsurface_value,
	};
	
	struct BoundingData
	{
		DirectX::XMFLOAT3 min_box_pos;
		DirectX::XMFLOAT3 max_box_pos;
	};
	struct bone_struct 
	{
		std::string bone_name;
		int32_t bone_ID_brother;
		int32_t bone_ID_son;
		bool if_used_for_skin;
		bone_struct() 
		{
			bone_name = "";
			bone_ID_brother = 99999999;
			bone_ID_son     = 99999999;
			if_used_for_skin = false;
		}
	};
	enum PbrMaterialType
	{
		PbrType_MetallicRoughness = 0,
		PbrType_SpecularSmoothness
	};
	//变换向量
	struct vector_animation
	{
		float time;               //帧时间
		float main_key[3];        //帧数据
	};
	//变换四元数
	struct quaternion_animation
	{
		float time;               //帧时间
		float main_key[4];        //帧数据
	};
	struct animation_data
	{
		std::string bone_name;                              //本次变换数据对应的骨骼名称
		int32_t bone_ID;                                    //本次变换数据对应的骨骼编号
		std::vector<vector_animation> translation_key;      //各个平移变换数据
		std::vector<vector_animation> scaling_key;          //各个放缩变换数据
		std::vector<quaternion_animation> rotation_key;     //各个旋转变换的数据
	};
	struct animation_set
	{
		float animation_length;                             //动画的长度
		std::vector<animation_data> data_animition;         //该动画的数据
	};
	
	enum PancyRenderMeshVertexType
	{
		PancyMeshVertexCommon = 0,
		PancyMeshVertexSkin,
		PancyMeshVertexPointCatch
	};
	struct AnimationNodeData 
	{
		DirectX::XMFLOAT4 translation_key;
		DirectX::XMFLOAT4 rotation_key;
		DirectX::XMFLOAT4 scaling_key;
	};
	class PancyRenderMesh
	{
		//顶点的格式数据
		PancyRenderMeshVertexType RenderType;
		//模型网格数据
		PancystarEngine::GeometryBasic *model_mesh;
		//todo:包围盒信息
		BoundingData mesh_bound;
	public:
		PancyRenderMesh();
		~PancyRenderMesh();
		template<typename T>
		PancystarEngine::EngineFailReason Create(const T* vertex_need, const IndexType* index_need, const int32_t &vert_num, const int32_t &index_num)
		{
			model_mesh = new PancystarEngine::GeometryCommonModel<T>(vertex_need, index_need, vert_num, index_num, false, true);
			auto check_error = model_mesh->Create();
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			return PancystarEngine::succeed;
		}
		template<typename T>
		inline PancystarEngine::EngineFailReason GetSubModelData(
			std::vector<T> &vertex_data_in,
			std::vector<IndexType> &index_data_in
		)
		{
			PancystarEngine::GeometryCommonModel<T> *model_real = dynamic_cast<PancystarEngine::GeometryCommonModel<T> *>(model_mesh);
			return model_real->GetModelData(vertex_data_in, index_data_in);
		}
		inline D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView()
		{
			return model_mesh->GetVertexBufferView();
		};
		inline D3D12_INDEX_BUFFER_VIEW GetIndexBufferView()
		{
			return model_mesh->GetIndexBufferView();
		};
		inline int32_t GetVertexNum()
		{
			return model_mesh->GetVetexNum();
		}
		inline int32_t GetIndexNum()
		{
			return model_mesh->GetIndexNum();
		}
		inline bool CheckIfLoadSucceed()
		{
			return model_mesh->CheckIfCreateSucceed();
		}
		inline VirtualResourcePointer& GetVertexBuffer()
		{
			return model_mesh->GetVertexBufferResource();
		};
	};
	//基础模型
	class PancyBasicModel
	{
		//模型的数据信息
		std::vector<PancyRenderMesh*> model_resource_list;     //模型的每个子部件
		//模型的动画信息
		bool if_skinmesh;
		bool if_pointmesh;
		//模型的包围以及形变信息
		BoundingData model_size;
		DirectX::XMFLOAT4X4 model_translation;
		PancystarEngine::GeometryBasic *model_boundbox;
		//骨骼动画信息
		int32_t root_id;
		std::unordered_map<std::string,pancy_object_id> bone_name_index;//根据骨骼名称索引骨骼ID
		std::unordered_map<pancy_object_id, bone_struct> bone_tree_data;//骨骼数据
		std::unordered_map<pancy_object_id, pancy_object_id> bone_parent_data;
		int bone_num;//用于蒙皮的骨骼数量
		int bone_object_num;//所有的骨骼数量(包括不用于蒙皮的部分)
		DirectX::XMFLOAT4X4 offset_matrix_array[MaxBoneNum];
		std::unordered_map<std::string, pancy_resource_id> skin_animation_name;
		std::unordered_map<pancy_resource_id, animation_set> skin_animation_map;
		float now_animation_play_station;//当前正在播放的动画
		bool if_animation_choose;
		//文件读取器
		ifstream instream;
		//动画数据buffer
		VirtualResourcePointer model_animation_buffer;
		VirtualResourcePointer model_bonetree_buffer;
		VirtualResourcePointer model_boneoffset_buffer;
	public:
		PancyBasicModel();
		PancystarEngine::EngineFailReason Create(const std::string &resource_name);
		//获取渲染网格
		inline pancy_object_id GetSubModelNum()
		{
			return static_cast<pancy_object_id>(model_resource_list.size());
		};
		inline PancystarEngine::EngineFailReason GetRenderMesh(const pancy_object_id &submesh_id, PancyRenderMesh **render_mesh)
		{
			if (submesh_id >= model_resource_list.size()) 
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "submesh id:" + std::to_string(submesh_id) + " bigger than the submodel num:" + std::to_string(model_resource_list.size()) + " of model: ");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Find submesh from model ", error_message);
				return error_message;
			}
			*render_mesh = model_resource_list[submesh_id];
			return PancystarEngine::succeed;
		}
		//检验动画信息
		inline bool CheckIfSkinMesh()
		{
			return if_skinmesh;
		}
		inline bool CheckIfPointMesh()
		{
			return if_pointmesh;
		}
		//获取包围盒信息
		inline PancystarEngine::GeometryBasic *GetBoundBox()
		{
			return model_boundbox;
		}
		//获取指定动画的指定时间的骨骼数据以及动画的世界偏移矩阵数据
		PancystarEngine::EngineFailReason GetBoneByAnimation(
			const pancy_resource_id &animation_ID,
			const float &animation_time, 
			std::vector<DirectX::XMFLOAT4X4> &matrix_out
		);
		virtual ~PancyBasicModel();
		bool CheckIfLoadSucceed();
	private:
		PancystarEngine::EngineFailReason InitResource(const Json::Value &root_value, const std::string &resource_name);
		//读取骨骼树
		PancystarEngine::EngineFailReason LoadSkinTree(const string &filename);
		PancystarEngine::EngineFailReason ReadBoneTree(int32_t &now_build_id);
		//更新骨骼的动画数据
		PancystarEngine::EngineFailReason UpdateAnimData(
			const pancy_resource_id &animation_ID,
			const float &time_in,
			std::vector<DirectX::XMFLOAT4X4> &matrix_out
		);
		//帧间插值(四元数)
		void Interpolate(quaternion_animation& pOut, const quaternion_animation &pStart, const quaternion_animation &pEnd, const float &pFactor);
		//帧间插值(向量)
		void Interpolate(vector_animation& pOut, const vector_animation &pStart, const vector_animation &pEnd, const float &pFactor);
		//根据指定的时间，获取其前后两个关键帧数据(四元数)
		void FindAnimStEd(const float &input_time, int &st, int &ed, const std::vector<quaternion_animation> &input);
		//根据指定的时间，获取其前后两个关键帧数据(向量)
		void FindAnimStEd(const float &input_time, int &st, int &ed, const std::vector<vector_animation> &input);
		//根据四元数获取变换矩阵
		void GetQuatMatrix(DirectX::XMFLOAT4X4 &resMatrix, const quaternion_animation& pOut);
		PancystarEngine::EngineFailReason UpdateRoot(
			int32_t root_id,
			const DirectX::XMFLOAT4X4 &matrix_parent,
			const std::vector<DirectX::XMFLOAT4X4> &matrix_animation,
			std::vector<DirectX::XMFLOAT4X4> &matrix_combine_save,
			std::vector<DirectX::XMFLOAT4X4> &matrix_out
		);
		//读取网格数据
		template<typename T>
		PancystarEngine::EngineFailReason LoadMeshData(const std::string &file_name_vertex, const std::string &file_name_index)
		{
			PancystarEngine::EngineFailReason check_error;
			
			int32_t vertex_num;
			int32_t index_num;
			
			
			instream.open(file_name_vertex, ios::binary);
			instream.read(reinterpret_cast<char*>(&vertex_num), sizeof(vertex_num));
			T *vertex_data = new T[vertex_num];
			int32_t vertex_size = vertex_num * sizeof(vertex_data[0]);
			instream.read(reinterpret_cast<char*>(vertex_data), vertex_size);
			instream.close();

			instream.open(file_name_index, ios::binary);
			instream.read(reinterpret_cast<char*>(&index_num), sizeof(index_num));
			IndexType *index_data = new IndexType[index_num];
			int32_t index_size = index_num * sizeof(index_data[0]);
			
			instream.read(reinterpret_cast<char*>(index_data), index_size);
			instream.close();
			PancyRenderMesh *new_submodel = new PancyRenderMesh();
			check_error = new_submodel->Create(vertex_data, index_data, vertex_num, index_num);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			model_resource_list.push_back(new_submodel);
			delete[] vertex_data;
			delete[] index_data;
			return PancystarEngine::succeed;
		}
		//根据层级获取父节点
		pancy_object_id FindParentByLayer(
			const pancy_object_id& bone_id,
			const pancy_object_id& layer,
			const std::vector<uint32_t> &bone_node_parent_data
		);
	};
}