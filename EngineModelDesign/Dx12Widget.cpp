#include "Dx12Widget.h"
D3d12RenderWidget::D3d12RenderWidget(QWidget *parent) : QWidget(parent)
{
	setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_NativeWindow, true);
	setFocusPolicy(Qt::ClickFocus);
	if_build = false;
	render_mesh_num = 0;
}

D3d12RenderWidget::~D3d12RenderWidget()
{
	//在这里释放接口
	//CleanUp();
	delete new_scene;

	PancystarEngine::EngineFailLog::GetInstance()->PrintLogToconsole();
	delete PancystarEngine::EngineFailLog::GetInstance();
	delete ThreadPoolGPUControl::GetInstance();
	delete PancyShaderControl::GetInstance();
	delete PancyRootSignatureControl::GetInstance();
	delete PancyEffectGraphic::GetInstance();
	delete PancyJsonTool::GetInstance();
	delete MemoryHeapGpuControl::GetInstance();
	delete PancyDescriptorHeapControl::GetInstance();
	delete PancystarEngine::PancyTextureControl::GetInstance();
	delete SubresourceControl::GetInstance();
	delete PancystarEngine::FileBuildRepeatCheck::GetInstance();
	delete PancyInput::GetInstance();
	delete PancyCamera::GetInstance();
	delete PancyDx12DeviceBasic::GetInstance();
}

PancystarEngine::EngineFailReason D3d12RenderWidget::Create(SceneRoot *new_scene_in)
{
	//创建directx设备
	PancystarEngine::EngineFailReason check_error;
	check_error = PancyDx12DeviceBasic::SingleCreate((HWND)winId(), width(), height());
	if (!check_error.CheckIfSucceed())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not Build D3d12 Device");
		return error_message;
	}
	//注册单例
	PancyShaderControl::GetInstance();
	PancyRootSignatureControl::GetInstance();
	PancyEffectGraphic::GetInstance();
	PancyJsonTool::GetInstance();
	MemoryHeapGpuControl::GetInstance();
	PancyDescriptorHeapControl::GetInstance();
	MemoryHeapGpuControl::GetInstance();
	SubresourceControl::GetInstance();
	PancyInput::SingleCreate((HWND)winId(), GetModuleHandle(0));
	PancyCamera::GetInstance();
	//创建线程池管理
	check_error = ThreadPoolGPUControl::SingleCreate();
	if (!check_error.CheckIfSucceed())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not Build d3d12 GPU pool Device");
		return error_message;
	}
	//创建场景
	new_scene = new_scene_in;
	//new_scene = new scene_test_simple();

	check_error = new_scene->Create(width(), height());
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if_build = true;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason D3d12RenderWidget::LoadModel(const std::string &file_name)
{
	auto scene_son = dynamic_cast<scene_test_simple*>(new_scene);
	if_have_skinmesh = false;
	return scene_son->LoadDealModel(file_name, render_mesh_num, model_lod, if_have_skinmesh, animation_name);

}
PancystarEngine::EngineFailReason D3d12RenderWidget::SaveModel(const std::string &file_name)
{
	auto scene_son = dynamic_cast<scene_test_simple*>(new_scene);
	if_have_skinmesh = false;
	return scene_son->SaveDealModel(file_name);
}
PancystarEngine::EngineFailReason D3d12RenderWidget::ChangeModelSize(const float &scal_size)
{
	auto scene_son = dynamic_cast<scene_test_simple*>(new_scene);
	scene_son->ResetDealModelScal(scal_size);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason D3d12RenderWidget::ChangeModelPosition(const float &pos_x, const float &pos_y, const float &pos_z)
{
	auto scene_son = dynamic_cast<scene_test_simple*>(new_scene);
	scene_son->ResetDealModelTranslation(pos_x, pos_y, pos_z);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason D3d12RenderWidget::ChangeModelRotation(const float &rot_x, const float &rot_y, const float &rot_z)
{
	auto scene_son = dynamic_cast<scene_test_simple*>(new_scene);
	scene_son->ResetDealModelRotaiton(rot_x, rot_y, rot_z);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason D3d12RenderWidget::ChangeModelBoundboxShow(const bool &if_show)
{
	auto scene_son = dynamic_cast<scene_test_simple*>(new_scene);
	scene_son->ResetDealModelBoundboxShow(if_show);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason D3d12RenderWidget::ChangeModelIfShowPart(const bool &if_show_part)
{
	auto scene_son = dynamic_cast<scene_test_simple*>(new_scene);
	scene_son->ResetDealModelIfPartShow(if_show_part);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason D3d12RenderWidget::ChangeModelNowShowPart(const std::vector<int32_t> &now_show_part)
{
	auto scene_son = dynamic_cast<scene_test_simple*>(new_scene);
	scene_son->ResetDealModelNowShowPart(now_show_part);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason D3d12RenderWidget::ChangeModelNowShowLod(const int32_t &now_show_lod)
{
	auto scene_son = dynamic_cast<scene_test_simple*>(new_scene);
	scene_son->ResetDealModelNowShowPart(model_lod[now_show_lod]);
	return PancystarEngine::succeed;
}

PancystarEngine::EngineFailReason D3d12RenderWidget::ChangeModelAnimationUsed(const std::string &animation_name) 
{
	auto scene_son = dynamic_cast<scene_test_simple*>(new_scene);
	scene_son->ResetDealModelAnimation(animation_name);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason D3d12RenderWidget::ChangeModelAnimationTime(const float &animation_time)
{
	auto scene_son = dynamic_cast<scene_test_simple*>(new_scene);
	scene_son->ResetDealModelAnimationTime(animation_time);
	return PancystarEngine::succeed;
}

PancystarEngine::EngineFailReason D3d12RenderWidget::ChangeModelNormalShow(const bool &if_show_normal_in, const bool &if_show_normal_point_in)
{
	auto scene_son = dynamic_cast<scene_test_simple*>(new_scene);
	scene_son->ResetDealModelShowNormal(if_show_normal_in, if_show_normal_point_in);
	return PancystarEngine::succeed;
}

void D3d12RenderWidget::mouseDoubleClickEvent(QMouseEvent *event_need)
{
	//click_pos_x = event_need->x();
	//click_pos_y = event_need->y();
	auto scene_son = dynamic_cast<scene_test_simple*>(new_scene);
	scene_son->PointWindow(event_need->x(), event_need->y());
}
void D3d12RenderWidget::paintEvent(QPaintEvent *event)
{
	if (if_build) 
	{
		//bool check = hasFocus();
		//if (check)
		//{
			new_scene->Update(0);
			new_scene->Display();
		//}
	}
	update();
}