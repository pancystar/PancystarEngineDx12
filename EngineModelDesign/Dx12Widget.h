/*
https://blog.csdn.net/u012234115/article/details/47402277
*/
#pragma once
#include <QWidget>
#include <QMouseEvent>
#include"TestScene.h"
class D3d12RenderWidget : public QWidget
{
	Q_OBJECT
public:
	D3d12RenderWidget(QWidget *parent);
	~D3d12RenderWidget();
	virtual QPaintEngine *paintEngine() const
	{
		return NULL;
	}
	PancystarEngine::EngineFailReason Create(SceneRoot *new_scene_in);
	PancystarEngine::EngineFailReason LoadModel(const std::string &file_name);
	PancystarEngine::EngineFailReason ChangeModelSize(const float &scal_size);
	PancystarEngine::EngineFailReason ChangeModelPosition(const float &pos_x,const float &pos_y, const float &pos_z);
	PancystarEngine::EngineFailReason ChangeModelRotation(const float &rot_x, const float &rot_y, const float &rot_z);
	PancystarEngine::EngineFailReason ChangeModelBoundboxShow(const bool &if_show);

	PancystarEngine::EngineFailReason ChangeModelIfShowPart(const bool &if_show_part);
	PancystarEngine::EngineFailReason ChangeModelNowShowPart(const std::vector<int32_t> &now_show_part);
	PancystarEngine::EngineFailReason ChangeModelNowShowLod(const int32_t &now_show_lod);

	PancystarEngine::EngineFailReason ChangeModelAnimationUsed(const std::string &animation_name);
	PancystarEngine::EngineFailReason ChangeModelAnimationTime(const float &animation_time);
	inline int32_t GetRenderMeshNum() 
	{
		return render_mesh_num;
	};
	inline int32_t GetLodNum()
	{
		return model_lod.size();
	};
	inline bool CheckIfSkinMesh() 
	{
		return if_have_skinmesh;
	};
	inline std::vector<std::string> GetSkinAnimation() 
	{
		return animation_name;
	}
private:
	bool if_build;
	int32_t render_mesh_num;
	std::vector<std::vector<int32_t>> model_lod;
	bool if_have_skinmesh;
	std::vector<std::string> animation_name;
	SceneRoot   *new_scene;
	virtual void paintEvent(QPaintEvent *event);   //窗口绘制函数，用于render三维场景
	virtual void mouseDoubleClickEvent(QMouseEvent *event_need);
private:
};

