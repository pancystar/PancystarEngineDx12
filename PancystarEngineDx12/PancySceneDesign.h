#pragma once
#include"PancystarEngineBasicDx12.h"
#include"PancyDx12Basic.h"
#include"PancyGeometryDx12.h"
#include"PancyDescriptor.h"
#include"PancyShaderDx12.h"
#include"PancyTextureDx12.h"
#include"PancyThreadBasic.h"
#include"PancyInput.h"
#include"PancyCamera.h"
#include"PancyRenderParam.h"
#include"PancyAnimationBasic.h"
#include"PancyDescriptor.h"
#include"PancyMaterial.h"
#include"PancyModelBasic.h"
class SceneRoot
{
private:
	std::vector<std::unordered_map<pancy_object_id, std::unordered_map<std::string, PancyConstantBuffer *>>> frame_constant_buffer;
protected:
	int32_t                               back_buffer_num;
	DirectX::XMFLOAT3                     scene_center_pos;//场景中心
	float                                 time_game;       //游戏时间
	int32_t                               Scene_width;
	int32_t                               Scene_height;
	bool                                  If_dsv_loaded;
	std::vector<PancystarEngine::VirtualResourcePointer>   Default_depthstencil_buffer;

	//与屏幕大小一致的几种纹理格式(普通RGB，SRGB，浮点纹理，标准D24S8深度缓冲区)
	PancystarEngine::PancyCommonTextureDesc default_tex_desc_RGB;
	PancystarEngine::PancyCommonTextureDesc default_tex_desc_SRGB;
	PancystarEngine::PancyCommonTextureDesc default_tex_desc_float;
	PancystarEngine::PancyCommonTextureDesc default_tex_desc_depthstencil;
public:
	SceneRoot();
	PancystarEngine::EngineFailReason Create(int32_t width_in, int32_t height_in);
	PancystarEngine::EngineFailReason ResetScreen(int32_t width_in, int32_t height_in);
	virtual void Display() = 0;
	virtual void DisplayNopost() = 0;
	virtual void DisplayEnvironment(DirectX::XMFLOAT4X4 view_matrix, DirectX::XMFLOAT4X4 proj_matrix) = 0;
	virtual void Update(float delta_time) = 0;
	virtual ~SceneRoot()
	{
		for (int32_t i = 0; i < frame_constant_buffer.size(); ++i) 
		{
			for (auto cbuffer_map = frame_constant_buffer[i].begin(); cbuffer_map != frame_constant_buffer[i].end(); ++cbuffer_map)
			{
				for (auto cbuffer_data = cbuffer_map->second.begin(); cbuffer_data != cbuffer_map->second.end(); ++cbuffer_data) 
				{
					delete cbuffer_data->second;
				}
			}
		}
	};
private:
	virtual PancystarEngine::EngineFailReason Init() = 0;
	virtual PancystarEngine::EngineFailReason ScreenChange() = 0;
protected:
	//todo：全局cbuffer不应该从PSO中获取，而应当从文件中获取
	PancystarEngine::EngineFailReason GetGlobelCbuffer(
		const pancy_object_id &PSO_id,
		const std::string &cbuffer_name,
		PancyConstantBuffer ** cbuffer_data
	);
	PancystarEngine::EngineFailReason GetGlobelCbuffer(
		const pancy_object_id &PSO_id, 
		const std::string &cbuffer_name, 
		std::vector<PancyConstantBuffer*> &cbuffer_data
	);
private:
	PancystarEngine::EngineFailReason GetGlobelCbufferByFrame(
		const pancy_object_id &frame_id,
		const pancy_object_id &PSO_id,
		const std::string &cbuffer_name,
		PancyConstantBuffer ** cbuffer_data,
		bool &if_create
	);
};

class engine_windows_main
{
	HWND         hwnd;                                                  //指向windows类的句柄。
	MSG          msg;                                                   //存储消息的结构。
	WNDCLASS     wndclass;
	int32_t      window_width;
	int32_t      window_height;
	HINSTANCE    hInstance;
	HINSTANCE    hPrevInstance;
	PSTR         szCmdLine;
	int32_t      iCmdShow;
	SceneRoot   *new_scene;
public:
	engine_windows_main(HINSTANCE hInstance_need, HINSTANCE hPrevInstance_need, PSTR szCmdLine_need, int iCmdShow_need);
	HRESULT game_create(SceneRoot   *new_scene_in);
	HRESULT game_loop();
	WPARAM game_end();
	static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
};