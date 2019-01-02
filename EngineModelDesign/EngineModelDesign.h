#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_EngineModelDesign.h"
#include "QFileDialog"
#include "Dx12Widget.h"
class EngineModelDesign : public QMainWindow
{
	Q_OBJECT

public:
	EngineModelDesign(QWidget *parent = Q_NULLPTR);
private:
	D3d12RenderWidget *widget;
	Ui::EngineModelDesignClass ui;
public slots: 
	void on_actionopen_triggered();
	void on_actionsave_triggered();
	void on_actionexportanimation_triggered();
	void on_actionadd_metallic_triggered();
	void on_actionadd_roughness_triggered();
	void on_actionadd_ao_triggered();
	void ModelSizeChange(int size_now);
	void ModelSIzeComplete(QString size_string);

};
