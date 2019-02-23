#pragma once
#include"..\\PancystarEngineDx12\\PancySceneDesign.h"

#ifdef _DEBUG
#pragma comment(lib,"..\\x64\\Debug\\PancystarEngineDx12.lib")
#else
#pragma comment(lib,"..\\x64\\Release\\PancystarEngineDx12.lib")
#endif

class scene_test_simple : public SceneRoot
{
	bool if_have_previous_frame;
	//管线状态
	ComPtr<ID3D12PipelineState> m_pipelineState;
	std::vector<PancyThreadIdGPU> renderlist_ID;
	//屏幕空间模型
	PancystarEngine::GeometryBasic *test_model;
	//模型测试
	PancystarEngine::PancyBasicModel *test_model_common;
	PancystarEngine::PancyBasicModel *test_model_pointmesh;
	PancystarEngine::PancyBasicModel *test_model_skinmesh;
	//视口
	CD3DX12_VIEWPORT view_port;
	CD3DX12_RECT view_rect;
	//帧等待fence号码
	PancyFenceIdGPU last_broken_fence_id;
	PancyFenceIdGPU broken_fence_id;
	//模型ID号
	uint32_t model_common, model_skinmesh, model_pointmesh;
	//pbr纹理
	pancy_object_id tex_brdf_id;
	pancy_object_id tex_ibl_spec_id;
	pancy_object_id tex_ibl_diffuse_id;
	//psoID
	pancy_object_id PSO_test;
	pancy_object_id PSO_pbr;

public:
	scene_test_simple()
	{
		if_have_previous_frame = false;
		renderlist_ID.clear();
		PancyJsonTool::GetInstance()->SetGlobelVraiable("PbrType_MetallicRoughness", static_cast<int32_t>(PancystarEngine::PbrType_MetallicRoughness), typeid(PancystarEngine::PbrType_MetallicRoughness).name());
		PancyJsonTool::GetInstance()->SetGlobelVraiable("PbrType_SpecularSmoothness", static_cast<int32_t>(PancystarEngine::PbrType_SpecularSmoothness), typeid(PancystarEngine::PbrType_SpecularSmoothness).name());
	}
	~scene_test_simple();
	
	void Display();
	void DisplayNopost() {};
	void DisplayEnvironment(DirectX::XMFLOAT4X4 view_matrix, DirectX::XMFLOAT4X4 proj_matrix);
	void Update(float delta_time);
private:
	PancystarEngine::EngineFailReason ShowFloor();
	PancystarEngine::EngineFailReason ShowModel();
	PancystarEngine::EngineFailReason Init();
	PancystarEngine::EngineFailReason ScreenChange();
	void PopulateCommandListSky();
	void PopulateCommandListModelDeal();
	PancystarEngine::EngineFailReason PretreatBrdf();
	PancystarEngine::EngineFailReason PretreatPbrDescriptor();
	void ClearScreen();
	void WaitForPreviousFrame();
	void updateinput(float delta_time);
	inline int ComputeIntersectionArea(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2)
	{
		return max(0, min(ax2, bx2) - max(ax1, bx1)) * max(0, min(ay2, by2) - max(ay1, by1));
	}
};