#pragma once
#include"..\\PancystarEngineDx12\\PancySceneDesign.h"
#include <assimp/Importer.hpp>      // 导入器在该头文件中定义
#include <assimp/scene.h>           // 读取到的模型数据都放在scene中
#include <assimp/postprocess.h>     // 该头文件中包含后处理的标志位定义
#include <assimp/matrix4x4.h>
#include <assimp/matrix3x3.h>
#ifdef _DEBUG
#pragma comment(lib,"..\\x64\\Debug\\PancystarEngineDx12.lib")
#else
#pragma comment(lib,"..\\x64\\Release\\PancystarEngineDx12.lib")
#endif

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
	tex_specular,
	tex_ambient
};
struct BoundingData
{
	DirectX::XMFLOAT3 min_box_pos;
	DirectX::XMFLOAT3 max_box_pos;
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
		model_mesh = new PancystarEngine::GeometryCommonModel<T>(vertex_need, index_need, vert_num, index_num);
		auto check_error = model_mesh->Create();
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		material_use = material_id;
		return PancystarEngine::succeed;
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
	std::vector<PancySubModel*> model_resource_list;     //模型的每个子部件
	std::unordered_map<pancy_object_id, std::unordered_map<TexType, pancy_object_id>> material_list;
	std::vector<pancy_object_id> texture_list;
protected:
	std::string model_root_path;
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
			PancystarEngine::EngineFailReason error_message(E_FAIL,"submodel id:"+std::to_string(submodel_id)+" bigger than the submodel num:"+std::to_string(model_resource_list.size())+" of model: " +resource_name);
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
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the texture id:"+std::to_string(texture_type)+" in material id:" + std::to_string(material_id) +"in model "+resource_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Find texture from model ", error_message);
			return error_message;
		}
		texture_id = texture_list[texture_data->second];
		return PancystarEngine::succeed;
	}
	virtual ~PancyModelBasic();
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
class PancyModelAssimp : public PancyModelBasic
{
	//临时渲染变量(模型处理工具由于只处理一个模型，不做view化处理和renderobj封装，正式使用会有view化处理)
	std::string pso_use;                  //pso
	std::vector<SubMemoryPointer> cbuffer;//常量缓冲区
	std::vector<ResourceViewPointer> table_offset;//每个shader外部变量的位置
	//模型加载变量
	Assimp::Importer importer;
	const aiScene *model_need;//assimp模型备份
	std::unordered_map<pancy_object_id, std::string> material_name_list;
	//模型的包围以及形变信息
	BoundingData model_size;
	DirectX::XMFLOAT4X4 model_translation;
	PancystarEngine::GeometryBasic *model_boundbox;
public:
	PancyModelAssimp(const std::string &desc_file_in, const std::string &pso_in);
	~PancyModelAssimp();
	inline PancyPiplineStateObjectGraph* GetPso() 
	{
		return PancyEffectGraphic::GetInstance()->GetPSO(pso_use);
	}
	inline std::vector<ResourceViewPointer> GetDescriptorHeap() 
	{
		return table_offset;
	}
	void update(DirectX::XMFLOAT4X4 world_matrix, DirectX::XMFLOAT4X4 uv_matrix,float delta_time);
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
private:
	PancystarEngine::EngineFailReason LoadModel(
		const std::string &resource_desc_file,
		std::vector<PancySubModel*> &model_resource,
		std::unordered_map<pancy_object_id, std::unordered_map<TexType, pancy_object_id>> &material_list,
		std::vector<pancy_object_id> &texture_use
	);
	PancystarEngine::EngineFailReason BuildTextureRes(std::string tex_name,const int &if_force_srgb, pancy_object_id &id_tex);
	PancystarEngine::EngineFailReason SaveModel(
		const std::string &resource_desc_file,
		std::vector<PancySubModel*> &model_resource,
		std::unordered_map<pancy_object_id, std::unordered_map<TexType, pancy_object_id>> &material_list,
		std::vector<pancy_object_id> &texture_use
	);
};
class scene_test_simple : public SceneRoot
{
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
	ResourceViewPointer table_offset_model[4];
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
	//待处理模型的变换信息
	float scale_size;
	DirectX::XMFLOAT3 translation_pos;
	DirectX::XMFLOAT3 rotation_angle;
	//pbr纹理
	pancy_object_id pic_empty_white_id;//空白纹理，标记为未加载
	pancy_object_id tex_brdf_id;
	std::vector<pancy_object_id> tex_metallic_id;
	std::vector<pancy_object_id> tex_roughness_id;
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
		translation_pos = DirectX::XMFLOAT3(0,0,0);
		rotation_angle = DirectX::XMFLOAT3(0, 0, 0);
		if_show_boundbox = false;
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
	PancystarEngine::EngineFailReason LoadDealModel(std::string file_name);
	inline void ResetDealModelScal(float scal_num) 
	{
		scale_size = scal_num;
	}
	inline void ResetDealModelTranslation(float x_value, float y_value, float z_value)
	{
		translation_pos.x = x_value;
		translation_pos.y = y_value;
		translation_pos.z = z_value;
	}
	inline void ResetDealModelRotaiton(float x_value, float y_value, float z_value) 
	{
		rotation_angle.x = x_value;
		rotation_angle.y = y_value;
		rotation_angle.z = z_value;
	}
	inline void ResetDealModelBoundboxShow(bool if_show)
	{
		if_show_boundbox = if_show;
	}
private:
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