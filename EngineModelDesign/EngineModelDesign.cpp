#include "EngineModelDesign.h"

EngineModelDesign::EngineModelDesign(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	widget = new D3d12RenderWidget(this);
	widget->resize(1280, 720);
	widget->move(QPoint(0, 150));
	auto new_scene = new scene_test_simple();
	PancystarEngine::EngineFailReason check_error = widget->Create(new_scene);
	if (!check_error.if_succeed) 
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
	QString file_name = QFileDialog::getOpenFileName(0,"load model", "./", "JSON File(*.json);;OBJ model(*.obj);;FBX Files(*.fbx)",0, QFileDialog::Option::ReadOnly);
	if (file_name.toStdString() != "") 
	{
		PancystarEngine::EngineFailReason check_error = widget->LoadModel(file_name.toStdString());
		if (check_error.if_succeed)
		{
			ui.meshpart->clear();
			ui.MeshLod->clear();
			ui.ChooseAnimation->clear();
			for (int i = 0; i < widget->GetRenderMeshNum(); ++i)
			{
				string num = "";
				num += '0' + i;
				ui.meshpart->addItem(tr(num.c_str()));
			}
			for (int i = 0; i < widget->GetLodNum(); ++i)
			{
				string num = "";
				num += '0' + i;
				ui.MeshLod->addItem(tr(num.c_str()));
			}
			if (widget->CheckIfSkinMesh()) 
			{
				QString new_str;
				new_str = QString::fromLocal8Bit("�������");
				ui.label_animation_type->setText(new_str);
				auto skin_anim_name = widget->GetSkinAnimation();
				for (int i = 0; i < skin_anim_name.size(); ++i) 
				{
					ui.ChooseAnimation->addItem(tr(skin_anim_name[i].c_str()));
				}
			}
			else 
			{
				QString new_str;
				new_str = QString::fromLocal8Bit("�޶���");
				ui.label_animation_type->setText(new_str);
			}
		}
	}
	
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
void EngineModelDesign::ShowModelBounding()
{
	widget->ChangeModelBoundboxShow(ui.CheckIfBoundBox->isChecked());
}
void EngineModelDesign::CheckIfModelRenderPart()
{
	if (ui.ShowModelPart->isChecked()) 
	{
		ui.ShowModelLOD->setCheckState(Qt::CheckState::Unchecked);
		ChangeModelRenderPart();
	}
	widget->ChangeModelIfShowPart(ui.ShowModelPart->isChecked());
}
void EngineModelDesign::CheckIfModelRenderLod() 
{
	if (ui.ShowModelLOD->isChecked())
	{
		ui.ShowModelPart->setCheckState(Qt::CheckState::Unchecked);
		ChangeModelRenderLod();
	}
	widget->ChangeModelIfShowPart(ui.ShowModelLOD->isChecked());
}
void EngineModelDesign::ChangeModelRenderPart() 
{
	if (ui.ShowModelPart->isChecked()) 
	{
		std::vector<int32_t> show_list;
		show_list.push_back(ui.meshpart->currentText().toInt());
		widget->ChangeModelNowShowPart(show_list);
	}
}
void EngineModelDesign::ChangeModelRenderLod()
{
	if (ui.ShowModelLOD->isChecked())
	{
		widget->ChangeModelNowShowLod(ui.MeshLod->currentText().toInt());
	}
}
void EngineModelDesign::on_actionsave_triggered()
{
	QString file_name = QFileDialog::getSaveFileName(0, "load model", "./", "JSON File(*.json)", 0, QFileDialog::Option::ReadOnly);
	if (file_name.toStdString() != "")
	{
		PancystarEngine::EngineFailReason check_error = widget->SaveModel(file_name.toStdString());
		if (!check_error.if_succeed)
		{
			return;
		}
	}
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
void EngineModelDesign::ModelAnimationChange()
{
	string animation_name = ui.ChooseAnimation->currentText().toStdString();
	widget->ChangeModelAnimationUsed(animation_name);
	ui.model_animation->setValue(0);
}
void EngineModelDesign::ModelAnimationTimeChange(int size_now)
{
	float now_percent = static_cast<float>(size_now) / 100.0f;
	widget->ChangeModelAnimationTime(now_percent);
	QString now_animation= QString::number(now_percent, 'g', 2);
	ui.label_model_animation->setText(now_animation);
}
void EngineModelDesign::ShowModelNormal() 
{
	bool if_show_normal = ui.CheckIfShowNormal->isChecked();
	bool if_show_normal_point;
	if (ui.show_normal_vertex->isChecked()) 
	{
		if_show_normal_point = true;
	}
	else 
	{
		if_show_normal_point = false;
	}
	widget->ChangeModelNormalShow(if_show_normal, if_show_normal_point);
}