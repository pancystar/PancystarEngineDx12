#pragma once
#include"PancystarEngineBasicDx12.h"
class scene_root
{
protected:
	DirectX::XMFLOAT3         scene_center_pos;//场景中心
	float                     time_game;       //游戏时间
public:
	scene_root();
	virtual PancystarEngine::EngineFailReason Create(int32_t width_in, int32_t height_in) = 0;
	virtual void Display() = 0;
	virtual void DisplayNopost() = 0;
	virtual void DisplayEnvironment(DirectX::XMFLOAT4X4 view_matrix, DirectX::XMFLOAT4X4 proj_matrix) = 0;
	virtual void Update(float delta_time) = 0;
	virtual ~scene_root()
	{
	};
};