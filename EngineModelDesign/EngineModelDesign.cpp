#include "EngineModelDesign.h"

EngineModelDesign::EngineModelDesign(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	widget = new D3d12RenderWidget(this);
	widget->resize(1280, 720);
	widget->move(QPoint(0, 100));
	auto new_scene = new scene_test_simple();
	PancystarEngine::EngineFailReason check_error = widget->Create(new_scene);
	//widget->width = 1280;
	//setCentralWidget(widget);
	
}
