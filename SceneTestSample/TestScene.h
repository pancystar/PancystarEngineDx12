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