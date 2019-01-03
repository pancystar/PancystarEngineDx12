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
	number_check_list.insert('0');
	number_check_list.insert('1');
	number_check_list.insert('2');
	number_check_list.insert('3');
	number_check_list.insert('4');
	number_check_list.insert('5');
	number_check_list.insert('6');
	number_check_list.insert('7');
	number_check_list.insert('8');
	number_check_list.insert('9');
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
	ModelSIzeComplete();
}
bool EngineModelDesign::check_if_number(string str_in)
{
	int32_t start_pos = 0;
	bool if_have_float_point = false;
	if (str_in[0] == '-') 
	{
		start_pos = 1;
	}
	for (int32_t i = start_pos; i < str_in.size(); ++i) 
	{
		auto check_index = number_check_list.find(str_in[i]);
		if (check_index == number_check_list.end()) 
		{
			if (if_have_float_point) 
			{
				return false;
			}
			else 
			{
				if (str_in[i] == '.') 
				{
					if_have_float_point = true;
				}
				else 
				{
					return false;
				}
			}
		}
	}
	return true;
}
void EngineModelDesign::ModelSIzeComplete()
{
	if (!check_if_number(ui.scalling->text().toStdString())) 
	{
		return;
	}
	float now_percent = ui.scalling->text().toFloat();
	if (now_percent > 0.01f) 
	{
		widget->ChangeModelSize(now_percent);
	}
	//size_string
}
void EngineModelDesign::ModelPositionChange() 
{
	if (!check_if_number(ui.translation_x->text().toStdString()) || !check_if_number(ui.translation_y->text().toStdString()) || !check_if_number(ui.translation_z->text().toStdString()))
	{
		return;
	}
	float x_pos, y_pos, z_pos;
	x_pos = ui.translation_x->text().toFloat();
	y_pos = ui.translation_y->text().toFloat();
	z_pos = ui.translation_z->text().toFloat();
	widget->ChangeModelPosition(x_pos,y_pos,z_pos);
}
void EngineModelDesign::ModelRotationChange() 
{
	if (!check_if_number(ui.rotation_x->text().toStdString()) || !check_if_number(ui.rotation_y->text().toStdString()) || !check_if_number(ui.rotation_z->text().toStdString()))
	{
		return;
	}
	float x_pos, y_pos, z_pos;
	x_pos = ui.rotation_x->text().toFloat();
	y_pos = ui.rotation_y->text().toFloat();
	z_pos = ui.rotation_z->text().toFloat();
	widget->ChangeModelRotation(x_pos, y_pos, z_pos);
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
