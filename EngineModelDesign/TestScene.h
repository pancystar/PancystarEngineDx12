#pragma once
#include"..\\PancystarEngineDx12\\PancySceneDesign.h"
#include <assimp/Importer.hpp>      // 导入器在该头文件中定义
#include <assimp/scene.h>           // 读取到的模型数据都放在scene中
#include <assimp/postprocess.h>     // 该头文件中包含后处理的标志位定义
#include <assimp/matrix4x4.h>
#include <assimp/matrix3x3.h>
#include <fbxsdk.h>
#include <DirectXMesh.h>
#ifdef _DEBUG
#pragma comment(lib,"..\\x64\\Debug\\PancystarEngineDx12.lib")
#else
#pragma comment(lib,"..\\x64\\Release\\PancystarEngineDx12.lib")
#endif
#define MaxBoneNum 100
#define NouseAssimpStruct -12138
#define VertexAnimationID uint32_t
struct per_view_pack
{
	DirectX::XMFLOAT4X4 view_matrix;
	DirectX::XMFLOAT4X4 projectmatrix;
	DirectX::XMFLOAT4X4 invview_matrix;
	DirectX::XMFLOAT4 view_position;
};
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
struct skin_tree
{
	char bone_ID[128];
	int bone_number;
	DirectX::XMFLOAT4X4 basic_matrix;
	DirectX::XMFLOAT4X4 animation_matrix;
	DirectX::XMFLOAT4X4 now_matrix;
	skin_tree *brother;
	skin_tree *son;
	skin_tree()
	{
		bone_ID[0] = '\0';
		bone_number = NouseAssimpStruct;
		brother = NULL;
		son = NULL;
		DirectX::XMStoreFloat4x4(&basic_matrix, DirectX::XMMatrixIdentity());
		DirectX::XMStoreFloat4x4(&animation_matrix, DirectX::XMMatrixIdentity());
		DirectX::XMStoreFloat4x4(&now_matrix, DirectX::XMMatrixIdentity());
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
	skin_tree *bone_point;                              //本次变换数据对应的骨骼的指针
	std::vector<vector_animation> translation_key;      //各个平移变换数据
	std::vector<vector_animation> scaling_key;          //各个放缩变换数据
	std::vector<quaternion_animation> rotation_key;     //各个旋转变换的数据
};
struct animation_set
{
	float animation_length;                             //动画的长度
	std::vector<animation_data> data_animition;         //该动画的数据
};
class PancySubModel
{
	PancystarEngine::GeometryBasic *model_mesh;
	pancy_object_id material_use;
public:
	PancySubModel();
	~PancySubModel();
	template<typename T>
	PancystarEngine::EngineFailReason Create(const T* vertex_need, const IndexType* index_need, const int32_t &vert_num, const int32_t &index_num, const pancy_object_id& material_id)
	{
		model_mesh = new PancystarEngine::GeometryCommonModel<T>(vertex_need, index_need, vert_num, index_num,false,true);
		auto check_error = model_mesh->Create();
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		material_use = material_id;
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
	inline pancy_object_id GetMaterial()
	{
		return material_use;
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
};
class PancyModelBasic : public PancystarEngine::PancyBasicVirtualResource
{
	//测试分支
protected:
	std::vector<PancySubModel*> model_resource_list;     //模型的每个子部件
	std::unordered_map<pancy_object_id, std::unordered_map<TexType, pancy_object_id>> material_list;
	std::vector<pancy_object_id> texture_list;
	//模型的LOD信息
	std::vector<std::vector<int32_t>> model_lod_divide;
	std::string model_root_path;
	//模型的动画信息
	bool if_skinmesh;
	bool if_pointmesh;
	//模型的pbr格式
	PbrMaterialType model_pbr_type;
public:
	PancyModelBasic(const std::string &desc_file_in);
	void GetRenderMesh(std::vector<PancySubModel*> &render_mesh);
	inline pancy_object_id GetSubModelNum()
	{
		return model_resource_list.size();
	};
	inline pancy_object_id GetSubModelTexture(pancy_object_id submodel_id, TexType texture_type)
	{
		if (submodel_id > model_resource_list.size())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "submodel id:" + std::to_string(submodel_id) + " bigger than the submodel num:" + std::to_string(model_resource_list.size()) + " of model: " + resource_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Find submodel from model ", error_message);
			return 0;
		}
		pancy_object_id mat_use = model_resource_list[submodel_id]->GetMaterial();
		auto material_data = material_list.find(mat_use);
		auto texture_data = material_data->second.find(texture_type)->second;
		return texture_list[texture_data];
	}
	inline size_t GetMaterialNum()
	{
		return material_list.size();
	}
	inline PancystarEngine::EngineFailReason GetMateriaTexture(const pancy_object_id &material_id, const TexType &texture_type, pancy_object_id &texture_id)
	{
		if (material_id > material_list.size())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "material id:" + std::to_string(material_id) + " bigger than the submodel num:" + std::to_string(model_resource_list.size()) + " of model: " + resource_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Find texture from model ", error_message);
			return error_message;
		}
		auto material_data = material_list.find(material_id);
		auto texture_data = material_data->second.find(texture_type);
		if (texture_data == material_data->second.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the texture id:" + std::to_string(texture_type) + " in material id:" + std::to_string(material_id) + "in model " + resource_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Find texture from model ", error_message);
			return error_message;
		}
		texture_id = texture_list[texture_data->second];
		return PancystarEngine::succeed;
	}
	inline void GetModelLod(std::vector<std::vector<int32_t>> &Lod_out)
	{
		Lod_out = model_lod_divide;
	}
	inline bool CheckIfSkinMesh()
	{
		return if_skinmesh;
	}
	inline bool CheckIfPointMesh()
	{
		return if_pointmesh;
	}
	virtual ~PancyModelBasic();
	inline PbrMaterialType GetModelPbrDesc()
	{
		return model_pbr_type;
	}
private:

	PancystarEngine::EngineFailReason InitResource(const std::string &resource_desc_file);
	virtual PancystarEngine::EngineFailReason LoadModel(
		const std::string &resource_desc_file,
		std::vector<PancySubModel*> &model_resource,
		std::unordered_map<pancy_object_id, std::unordered_map<TexType, pancy_object_id>> &material_list,
		std::vector<pancy_object_id> &texture_use
	) = 0;
	void GetRootPath(const std::string &desc_file_in);
};
class PancyModelJson : public PancyModelBasic
{
public:
	PancyModelJson(const std::string &desc_file_in);

};

//FBX文件解析
struct mesh_animation_data
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT3 tangent;
	float delta_time;
	mesh_animation_data()
	{
		position = DirectX::XMFLOAT3(0, 0, 0);
		normal = DirectX::XMFLOAT3(0, 0, 0);
		tangent = DirectX::XMFLOAT3(0, 0, 0);
	}
};
class mesh_animation_FBX
{
#ifdef IOS_REF
#undef  IOS_REF
#define IOS_REF (*(pManager->GetIOSettings()))
#endif
	//FBX属性
	bool if_fbx_file;
	FbxString *lFilePath;
	FbxManager* lSdkManager = NULL;
	FbxScene* lScene = NULL;
	std::vector<FbxMesh*> lMesh_list;
	//动画属性
	std::vector <std::vector<std::vector<mesh_animation_data>>> anim_data_list;
	FbxTime anim_start;
	FbxTime anim_end;
	FbxTime anim_frame;
	int frame_per_second;
	int frame_num;
	int32_t vertex_pack_num;
public:
	mesh_animation_FBX(std::string file_name_in);
	PancystarEngine::EngineFailReason create(
		const std::vector<int32_t> &vertex_buffer_num_list,
		const std::vector<int32_t> &index_buffer_num_list,
		const std::vector<vector<IndexType>> &index_buffer_data_list,
		const std::vector<std::vector<DirectX::XMFLOAT2>> &UV_buffer_data_list
	);
	int get_frame_num() { return frame_num; };
	int get_FPS() { return frame_per_second; };
	inline int32_t GetMeshAnimNumber() 
	{
		int anim_point_num = 0;
		for (int i = 0; i < anim_data_list.size(); ++i) 
		{
			for (int j = 0; j < anim_data_list[i].size(); ++j) 
			{
				anim_point_num += anim_data_list[i][j].size();
			}
		}
		return anim_point_num;
	}
	//获取顶点动画的数据(压缩后的数据，包括一个压缩过的关键帧记录数据以及记录关键帧位置的指针数据，压缩质量越高，压缩率越低)
	void GetMeshAnimData(
		std::vector<std::vector<mesh_animation_data>> &now_check_point_all,
		std::vector<std::vector<VertexAnimationID>> &now_ID_save_all,
		float compress_quality = 0.999f
	);
	inline int32_t GetMeshSizePerFrame()
	{
		return vertex_pack_num;
	}
	//std::vector<mesh_animation_per_frame> get_anim_list() { return anim_data_list; };
	void release();
private:
	PancystarEngine::EngineFailReason build_buffer();
	void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);
	void DestroySdkObjects(FbxManager* pManager, bool pExitStatus);
	bool SaveScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename, int pFileFormat, bool pEmbedMedia);
	bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);
	void PreparePointCacheData(FbxScene* pScene, FbxTime &pCache_Start, FbxTime &pCache_Stop);
	PancystarEngine::EngineFailReason ReadVertexCacheData(FbxMesh* pMesh, FbxTime& pTime, FbxVector4* pVertexArray);
	void UpdateVertexPosition(
		const int32_t &animation_id,
		FbxMesh * pMesh, 
		const FbxVector4 * pVertices, 
		const int32_t &vertex_num_assimp,
		const std::vector<IndexType> &index_assimp,
		const std::vector<DirectX::XMFLOAT2> &uv_assimp
	);
	void find_tree_mesh(FbxNode *pNode);
	//void compute_normal();
	DirectX::XMFLOAT3 get_normal_vert(FbxMesh * pMesh, int vertex_count);
};

//Assimp文件解析
class PancyModelAssimp : public PancyModelBasic
{
	//临时渲染变量(模型处理工具由于只处理一个模型，不做view化处理和renderobj封装，正式使用会有view化处理)
	std::string pso_use;                  //pso
	std::vector<SubMemoryPointer> cbuffer;//常量缓冲区
	std::vector<ResourceViewPointer> table_offset;//每个shader外部变量的位置
	//模型加载变量
	Assimp::Importer importer;
	
	std::unordered_map<pancy_object_id, std::string> material_name_list;
	//模型的包围以及形变信息
	BoundingData model_size;
	DirectX::XMFLOAT4X4 model_translation;
	PancystarEngine::GeometryBasic *model_boundbox;
	
	//骨骼动画信息
	skin_tree *root_skin;
	int bone_num;
	int root_bone_num = 0;
	DirectX::XMFLOAT4X4 bone_matrix_array[MaxBoneNum];
	DirectX::XMFLOAT4X4 offset_matrix_array[MaxBoneNum];
	DirectX::XMFLOAT4X4 final_matrix_array[MaxBoneNum];
	int tree_node_num[MaxBoneNum][MaxBoneNum];
	std::unordered_map<std::string, animation_set> skin_animation_map;
	animation_set now_animation_use;//当前正在使用的动画
	float now_animation_play_station;//当前正在播放的动画
	DirectX::XMFLOAT4X4 bind_pose_matrix;//控制模型位置的根骨骼偏移矩阵
	skin_tree *model_move_skin;//当前控制模型位置的根骨骼
	bool if_animation_choose;
	//顶点动画信息
	mesh_animation_FBX *FBXanim_import;
	SubMemoryPointer vertex_anim_buffer;//用于存储压缩后的顶点动画数据
	SubMemoryPointer vertex_anim_ID_buffer;//用于存储负责解压顶点数据的ID数据
	int32_t mesh_animation_buffer_size;
	int32_t mesh_animation_ID_buffer_size;
	PancyFenceIdGPU upload_fence_value;
	PancystarEngine::EngineFailReason BuildDefaultBuffer(
		ID3D12GraphicsCommandList* cmdList,
		int64_t memory_alignment_size,
		int64_t memory_block_alignment_size,
		SubMemoryPointer &default_buffer,
		SubMemoryPointer &upload_buffer,
		const void* initData,
		const UINT BufferSize,
		D3D12_RESOURCE_STATES buffer_type
	);
	//文件存储指针
	ofstream out_stream;
public:
	PancyModelAssimp(const std::string &desc_file_in, const std::string &pso_in);
	~PancyModelAssimp();
	PancystarEngine::EngineFailReason SaveModelToFile(ID3D11Device* device_pancy,const std::string &out_file_in);
	inline PancyPiplineStateObjectGraph* GetPso()
	{
		return PancyEffectGraphic::GetInstance()->GetPSO(pso_use);
	}
	inline std::vector<ResourceViewPointer> GetDescriptorHeap()
	{
		return table_offset;
	}
	void update(DirectX::XMFLOAT4X4 world_matrix, DirectX::XMFLOAT4X4 uv_matrix, float delta_time);
	inline std::string GetMaterialName(pancy_object_id mat_id)
	{
		auto mat_data = material_name_list.find(mat_id);
		if (mat_data != material_name_list.end())
		{
			return mat_data->second;
		}
		return "";
	}
	inline PancystarEngine::GeometryBasic *GetBoundBox()
	{
		return model_boundbox;
	}
	inline void SetAnimation(std::string animation_name) 
	{
		auto animation_find = skin_animation_map.find(animation_name);
		if (animation_find != skin_animation_map.end()) 
		{
			now_animation_use = animation_find->second;
			if_animation_choose = true;
			now_animation_play_station = 0.0f;
		}
	}
	inline void SetAnimationTime(float time) 
	{
		if (time > 0.0f && time < 1.0f) 
		{
			now_animation_play_station = time;
		}
	}
	void GetAnimationNameList(std::vector<std::string> &animation_name);
	void GetModelBoneData(DirectX::XMFLOAT4X4 *bone_matrix);
	inline DirectX::XMFLOAT4X4 GetModelAnimationOffset() 
	{
		if (model_move_skin != NULL) 
		{
			DirectX::XMFLOAT4X4 bone_mat;
			DirectX::XMStoreFloat4x4(&bone_mat, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&bind_pose_matrix) * DirectX::XMLoadFloat4x4(&model_move_skin->now_matrix)));
			return bone_mat;
		}
		else
		{
			DirectX::XMFLOAT4X4 identity_mat;
			DirectX::XMStoreFloat4x4(&identity_mat,DirectX::XMMatrixIdentity());
			return identity_mat;
		}
	}
	inline SubMemoryPointer GetPointAnimationBuffer(int32_t &buffer_size, int32_t &stride_size)
	{
		buffer_size = mesh_animation_buffer_size;
		stride_size = sizeof(mesh_animation_data);
		return vertex_anim_buffer;
	}
	inline SubMemoryPointer GetPointAnimationIDBuffer(int32_t &buffer_size, int32_t &stride_size)
	{
		buffer_size = mesh_animation_ID_buffer_size;
		stride_size = sizeof(VertexAnimationID);
		return vertex_anim_ID_buffer;
	}
	inline void GetPointAnimationFrame(uint32_t &now_frame,uint32_t &all_frame_num)
	{
		all_frame_num = FBXanim_import->get_frame_num();
		//perframe_size = FBXanim_import->GetMeshSizePerFrame();
		now_frame = now_animation_play_station * all_frame_num;
	}
private:
	PancystarEngine::EngineFailReason LoadModel(
		const std::string &resource_desc_file,
		std::vector<PancySubModel*> &model_resource,
		std::unordered_map<pancy_object_id,
		std::unordered_map<TexType, pancy_object_id>> &material_list,
		std::vector<pancy_object_id> &texture_use
	);
	PancystarEngine::EngineFailReason LoadAnimation(const std::string &resource_desc_file,const std::string &animation_name);
	template<typename T>
	PancystarEngine::EngineFailReason BuildModelData(
		T *point_need,
		const aiMesh* paiMesh,
		int32_t mat_start_id
	)
	{
		//创建顶点缓冲区
		for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
		{
			//从assimp中读取的数据
			point_need[j].position.x = paiMesh->mVertices[j].x;
			point_need[j].position.y = paiMesh->mVertices[j].y;
			point_need[j].position.z = paiMesh->mVertices[j].z;
			//更新AABB包围盒
			if (point_need[j].position.x > model_size.max_box_pos.x)
			{
				model_size.max_box_pos.x = point_need[j].position.x;
			}
			if (point_need[j].position.x < model_size.min_box_pos.x)
			{
				model_size.min_box_pos.x = point_need[j].position.x;
			}
			if (point_need[j].position.y > model_size.max_box_pos.y)
			{
				model_size.max_box_pos.y = point_need[j].position.y;
			}
			if (point_need[j].position.y < model_size.min_box_pos.y)
			{
				model_size.min_box_pos.y = point_need[j].position.y;
			}
			if (point_need[j].position.z > model_size.max_box_pos.z)
			{
				model_size.max_box_pos.z = point_need[j].position.z;
			}
			if (point_need[j].position.z < model_size.min_box_pos.z)
			{
				model_size.min_box_pos.z = point_need[j].position.z;
			}
			point_need[j].normal.x = paiMesh->mNormals[j].x;
			point_need[j].normal.y = paiMesh->mNormals[j].y;
			point_need[j].normal.z = paiMesh->mNormals[j].z;

			if (paiMesh->HasTextureCoords(0))
			{
				point_need[j].tex_uv.x = paiMesh->mTextureCoords[0][j].x;
				point_need[j].tex_uv.y = paiMesh->mTextureCoords[0][j].y;
				point_need[j].tex_uv.y = 1 - point_need[j].tex_uv.y;
			}
			else
			{
				point_need[j].tex_uv.x = 0.0f;
				point_need[j].tex_uv.y = 0.0f;
			}
			if (paiMesh->mTangents != NULL)
			{
				point_need[j].tangent.x = paiMesh->mTangents[j].x;
				point_need[j].tangent.y = paiMesh->mTangents[j].y;
				point_need[j].tangent.z = paiMesh->mTangents[j].z;
			}
			else
			{
				point_need[j].tangent.x = 0.0f;
				point_need[j].tangent.y = 0.0f;
				point_need[j].tangent.z = 0.0f;
			}
			//生成纹理使用数据
			//使用漫反射纹理作为第一个纹理的偏移量,uvid的y通量记录纹理数量
			point_need[j].tex_id.x = mat_start_id;
			
		}
		
		return PancystarEngine::succeed;
	};
	PancystarEngine::EngineFailReason BuildTextureRes(std::string tex_name, const int &if_force_srgb, pancy_object_id &id_tex);
	PancystarEngine::EngineFailReason SaveModel(
		const std::string &resource_desc_file,
		std::vector<PancySubModel*> &model_resource,
		std::unordered_map<pancy_object_id, std::unordered_map<TexType, pancy_object_id>> &material_list,
		std::vector<pancy_object_id> &texture_use
	);
	inline bool CheckIFJson(const std::string &file_name)
	{
		if (file_name.size() >= 4)
		{
			string check_file = file_name.substr(file_name.size() - 4, 4);
			if (check_file == "json")
			{
				return true;
			}
		}
		return false;
	}
	pancy_object_id insert_new_texture(std::vector<pancy_object_id> &texture_use, const pancy_object_id &tex_id);
	//骨骼动画计算
	skin_tree* find_tree(skin_tree* p, char name[]);
	skin_tree* find_tree(skin_tree* p, int num);
	void FindRootBone(skin_tree *now_bone);
	bool check_ifsame(char a[], char b[]);
	PancystarEngine::EngineFailReason build_skintree(aiNode *now_node, skin_tree *now_root);
	void set_matrix(DirectX::XMFLOAT4X4 &out, aiMatrix4x4 *in);
	void check_son_num(skin_tree *input,int &count);
	void update_root(skin_tree *root, DirectX::XMFLOAT4X4 matrix_parent);
	void update_mesh_offset(const aiScene *model_need);
	PancystarEngine::EngineFailReason build_animation_list(const aiScene *model_need, const std::string animation_name_in = "");
	void update_anim_data();
	void find_anim_sted(const float &input_time,int &st, int &ed, const std::vector<quaternion_animation> &input);
	void find_anim_sted(const float &input_time, int &st, int &ed, const std::vector<vector_animation> &input);
	void Interpolate(quaternion_animation& pOut, const quaternion_animation &pStart, const quaternion_animation &pEnd, const float &pFactor);
	void Interpolate(vector_animation& pOut, const vector_animation &pStart, const vector_animation &pEnd, const float &pFactor);
	void Get_quatMatrix(DirectX::XMFLOAT4X4 &resMatrix, const quaternion_animation& pOut);
	void GetRootSkinOffsetMatrix(aiNode *root, aiMatrix4x4 matrix_parent);
	void SaveBoneTree(skin_tree *bone_data);
};
class scene_test_simple : public SceneRoot
{
	//dx11接口(用于进行bc6/7纹理的gpu压缩，等微软修正了dx12下的bc7压缩后再消掉)
	ID3D11Device* device_pancy;
	ID3D11DeviceContext *contex_pancy;
	//管线状态
	ComPtr<ID3D12PipelineState> m_pipelineState;
	std::vector<PancyThreadIdGPU> renderlist_ID;
	//模型测试
	PancystarEngine::GeometryBasic *test_model;
	//视口
	CD3DX12_VIEWPORT view_port;
	CD3DX12_RECT view_rect;
	//帧等待fence号码
	PancyFenceIdGPU broken_fence_id;
	//资源绑定(天空盒)
	ResourceViewPointer table_offset[3];
	SubMemoryPointer cbuffer[2];
	//资源绑定(待处理模型)
	ResourceViewPointer table_offset_model[6];
	/*
	cbuffer_per_object
	cbuffer_per_view
	diffuse
	normal
	metallic/specular
	roughness/smoothness
	*/
	SubMemoryPointer cbuffer_model[2];
	//模型资源
	PancyModelBasic *model_sky;
	PancyModelBasic *model_cube;
	//待处理的模型资源
	bool if_load_model;
	PancyModelBasic *model_deal;
	bool if_show_boundbox;
	bool if_only_show_part;
	std::vector<int32_t> now_show_part;
	bool if_show_normal;
	bool if_show_normal_point;
	//待处理模型的变换信息
	float scale_size;
	DirectX::XMFLOAT3 translation_pos;
	DirectX::XMFLOAT3 rotation_angle;
	//pbr纹理
	pancy_object_id tex_brdf_id;
	pancy_object_id tex_ibl_spec_id;
	pancy_object_id tex_ibl_diffuse_id;
	//屏幕空间回读纹理
	bool if_readback_build;
	int32_t texture_size;
	pancy_object_id tex_uint_save[2];
	pancy_object_id read_back_buffer[2];
	pancy_object_id depth_stencil_mask[2];
	ResourceViewPointer rtv_mask[2];
	ResourceViewPointer dsv_mask[2];
	D3D12_TEXTURE_COPY_LOCATION dst_loc;
	D3D12_TEXTURE_COPY_LOCATION src_loc;
	int32_t x_point;
	int32_t y_point;
	bool if_pointed;
	uint8_t now_point_answer;
	//UI控制信息
	bool if_focus;
	
public:
	scene_test_simple()
	{
		device_pancy = NULL;
		contex_pancy = NULL;
		renderlist_ID.clear();
		model_deal = NULL;
		if_readback_build = false;
		if_pointed = false;
		if_load_model = false;
		if_focus = false;
		scale_size = 1.0f;
		translation_pos = DirectX::XMFLOAT3(0, 0, 0);
		rotation_angle = DirectX::XMFLOAT3(0, 0, 0);
		if_show_boundbox = false;
		if_only_show_part = false;
		if_show_normal = false;
		if_show_normal_point = false;
		now_show_part.push_back(0);
		PancyJsonTool::GetInstance()->SetGlobelVraiable("PbrType_MetallicRoughness", static_cast<int32_t>(PbrType_MetallicRoughness), typeid(PbrType_MetallicRoughness).name());
		PancyJsonTool::GetInstance()->SetGlobelVraiable("PbrType_SpecularSmoothness", static_cast<int32_t>(PbrType_SpecularSmoothness), typeid(PbrType_SpecularSmoothness).name());
	}
	~scene_test_simple();
	inline void PointWindow(int32_t x_pos, int32_t y_pos)
	{
		if_pointed = true;
		x_point = x_pos;
		y_point = y_pos;
	}
	void Display();
	void DisplayNopost() {};
	void DisplayEnvironment(DirectX::XMFLOAT4X4 view_matrix, DirectX::XMFLOAT4X4 proj_matrix);
	void Update(float delta_time);
	inline void SetFocus(const bool &if_focus_in)
	{
		if_focus = if_focus_in;
	}
	PancystarEngine::EngineFailReason LoadDealModel(
		std::string file_name,
		int32_t &model_part_num,
		std::vector<std::vector<int32_t>> &Lod_out,
		bool &if_skin_mesh,
		std::vector<std::string> &animation_list
	);
	PancystarEngine::EngineFailReason SaveDealModel(const std::string &file_name) 
	{
		if (if_load_model)
		{
			PancyModelAssimp *assimp_pointer = dynamic_cast<PancyModelAssimp*>(model_deal);
			PancystarEngine::EngineFailReason check_error = assimp_pointer->SaveModelToFile(device_pancy,file_name);
			if (!check_error.CheckIfSucceed()) 
			{
				return check_error;
			}
		}
		else 
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "model haven't load");
			return error_message;
		}
		return PancystarEngine::succeed;
	}
	inline void ResetDealModelScal(const float &scal_num)
	{
		scale_size = scal_num;
	}
	inline void ResetDealModelTranslation(const float &x_value, const float &y_value, const float &z_value)
	{
		translation_pos.x = x_value;
		translation_pos.y = y_value;
		translation_pos.z = z_value;
	}
	inline void ResetDealModelRotaiton(const float &x_value, const float &y_value, const float &z_value)
	{
		rotation_angle.x = x_value;
		rotation_angle.y = y_value;
		rotation_angle.z = z_value;
	}
	inline void ResetDealModelBoundboxShow(const bool &if_show)
	{
		if_show_boundbox = if_show;
	}
	inline void ResetDealModelIfPartShow(const bool &if_part_show)
	{
		if_only_show_part = if_part_show;
	}
	inline void ResetDealModelNowShowPart(const std::vector<int32_t> &part_id)
	{
		now_show_part = part_id;
	}
	inline void ResetDealModelAnimation(const std::string &animation_name)
	{
		if (if_load_model)
		{
			PancyModelAssimp *assimp_pointer = dynamic_cast<PancyModelAssimp*>(model_deal);
			if (assimp_pointer->CheckIfSkinMesh())
			{
				assimp_pointer->SetAnimation(animation_name);
			}
		}
	}
	inline void ResetDealModelAnimationTime(const float &time)
	{
		if (if_load_model) 
		{
			PancyModelAssimp *assimp_pointer = dynamic_cast<PancyModelAssimp*>(model_deal);
			if (assimp_pointer->CheckIfSkinMesh() || assimp_pointer->CheckIfPointMesh())
			{
				assimp_pointer->SetAnimationTime(time);
			}
		}
	}
	inline void ResetDealModelShowNormal(const bool &if_show_normal_in, const bool &if_show_normal_point_in)
	{
		if (if_load_model)
		{
			PancyModelAssimp *assimp_pointer = dynamic_cast<PancyModelAssimp*>(model_deal);
			if (assimp_pointer->CheckIfPointMesh())
			{
				if_show_normal = if_show_normal_in;
				if_show_normal_point = if_show_normal_point_in;
			}
		}
	}

private:
	inline void GetDealModelLodPart(std::vector<std::vector<int32_t>> &Lod_out)
	{
		model_deal->GetModelLod(Lod_out);
	}
	PancystarEngine::EngineFailReason Init();
	PancystarEngine::EngineFailReason ScreenChange();
	void PopulateCommandList(PancyModelBasic *now_res);
	void PopulateCommandListSky();
	void PopulateCommandListModelDeal();
	void PopulateCommandListReadBack();
	void PopulateCommandListModelDealBound();
	PancystarEngine::EngineFailReason PretreatBrdf();
	PancystarEngine::EngineFailReason PretreatPbrDescriptor();
	PancystarEngine::EngineFailReason UpdatePbrDescriptor();
	void ClearScreen();
	void WaitForPreviousFrame();
	void updateinput(float delta_time);
	void ReadBackData(int x_mouse, int y_mouse);
	inline int ComputeIntersectionArea(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2)
	{
		return max(0, min(ax2, bx2) - max(ax1, bx1)) * max(0, min(ay2, by2) - max(ay1, by1));
	}
};