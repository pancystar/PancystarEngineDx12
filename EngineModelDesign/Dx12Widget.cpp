#include "Dx12Widget.h"
D3d12RenderWidget::D3d12RenderWidget(QWidget *parent) : QWidget(parent)
{
	setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_NativeWindow, true);
	if_build = false;
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
PancystarEngine::EngineFailReason D3d12RenderWidget::LoadModel(std::string file_name)
{
	auto scene_son = dynamic_cast<scene_test_simple*>(new_scene);
	return scene_son->LoadDealModel(file_name);
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
		new_scene->Update(0);
		new_scene->Display();
	}
	update();
}