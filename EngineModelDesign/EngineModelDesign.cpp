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
	if (!check_error.CheckIfSucceed()) 
	{
		PancystarEngine::EngineFailLog::GetInstance()->PrintLogToconsole();
	}
	setFocusPolicy(Qt::ClickFocus);
	//widget->width = 1280;
	//setCentralWidget(widget);
	
}
void EngineModelDesign::on_actionopen_triggered()
{
	QString file_name = QFileDialog::getOpenFileName(0,"load model", "./", "OBJ model(*.obj);;FBX Files(*.fbx)",0, QFileDialog::Option::ReadOnly);
	widget->LoadModel(file_name.toStdString());
}
void EngineModelDesign::ModelSizeChange(int size_now)
{
	double now_percent = static_cast<double>(size_now) / 100.0;
	double now_scal = 0.1 + pow(now_percent, 3.2)*9.9;
	QString now_scalling = QString::number(now_scal,'g',2);
	ui.scalling->setText(now_scalling);
}
void EngineModelDesign::ModelSIzeComplete(QString size_string)
{
	double now_percent;
	//size_string
}
void EngineModelDesign::on_actionsave_triggered()
{
	int a = 0;
}
void EngineModelDesign::on_actionexportanimation_triggered()
{
	int a = 0;
}
void EngineModelDesign::on_actionadd_metallic_triggered() 
{
	int a = 0;
}
void EngineModelDesign::on_actionadd_roughness_triggered() 
{
	int a = 0;
}
void EngineModelDesign::on_actionadd_ao_triggered() 
{
	int a = 0;
}
