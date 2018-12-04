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
enum TexType
{
	tex_diffuse = 0,
	tex_normal,
	tex_metallic_roughness,
	tex_specular,
	tex_ambient
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
	//临时渲染变量(模型处理工具中不做view化处理，正式使用会有view化处理)
	std::string pso_use;                  //pso
	std::vector<SubMemoryPointer> cbuffer;//常量缓冲区
	std::vector<ResourceViewPointer> table_offset;//每个shader外部变量的位置
	//模型加载变量
	Assimp::Importer importer;
	const aiScene *model_need;//assimp模型备份
public:
	PancyModelAssimp(const std::string &desc_file_in, const std::string &pso_in);
private:
	PancystarEngine::EngineFailReason LoadModel(
		const std::string &resource_desc_file,
		std::vector<PancySubModel*> &model_resource,
		std::unordered_map<pancy_object_id, std::unordered_map<TexType, pancy_object_id>> &material_list,
		std::vector<pancy_object_id> &texture_use
	);
};
class scene_test_simple : public scene_root
{
	//管线状态
	ComPtr<ID3D12PipelineState> m_pipelineState;
	uint32_t renderlist_ID;
	//模型测试
	PancystarEngine::GeometryBasic *test_model;
	//视口
	CD3DX12_VIEWPORT view_port;
	CD3DX12_RECT view_rect;
	//帧等待fence号码
	PancyFenceIdGPU broken_fence_id;
	//资源绑定测试
	ResourceViewPointer table_offset[3];
	//模型资源
	PancyModelBasic *new_res;
public:
	scene_test_simple()
	{
	}
	~scene_test_simple();
	PancystarEngine::EngineFailReason Create(int32_t width_in, int32_t height_in);
	PancystarEngine::EngineFailReason ResetScreen(int32_t width_in, int32_t height_in);
	void Display();
	void DisplayNopost() {};
	void DisplayEnvironment(DirectX::XMFLOAT4X4 view_matrix, DirectX::XMFLOAT4X4 proj_matrix);
	void Update(float delta_time);
private:
	void PopulateCommandList();
	void WaitForPreviousFrame();
	inline int ComputeIntersectionArea(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2)
	{
		return max(0, min(ax2, bx2) - max(ax1, bx1)) * max(0, min(ay2, by2) - max(ay1, by1));
	}
};