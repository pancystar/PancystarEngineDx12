#include "Dx12Widget.h"
D3d12RenderWidget::D3d12RenderWidget(QWidget *parent) : QWidget(parent)
{

	//设置窗口属性，关键步骤，否则D3D绘制出问题
	setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_NativeWindow, true);
	//在这里初始化D3D和场景
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
	return PancystarEngine::succeed;
}
void D3d12RenderWidget::paintEvent(QPaintEvent *event)
{
	//计算fps
	//frameCount++;
	//if (getTime() > 1.0f)
	//{
	//	fps = frameCount;
	//	frameCount = 0;
	//	startFPStimer();
		//设置父窗口标题显示fps值
	//	parentWidget()->setWindowTitle("FPS: " + QString::number(fps));
	//}
	//frameTime = getFrameTime();
	//更新场景和渲染场景
	//UpdateScene(frameTime);
	//RenderScene();
	//保证此函数体每一帧都调用
	static int check = 0;
	if (check == 0) 
	{
		new_scene->Update(0);
		new_scene->Display();
	}
	
	update();
}