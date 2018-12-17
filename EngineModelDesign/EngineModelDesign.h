#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_EngineModelDesign.h"
#include "Dx12Widget.h"
class EngineModelDesign : public QMainWindow
{
	Q_OBJECT

public:
	EngineModelDesign(QWidget *parent = Q_NULLPTR);
private:
	D3d12RenderWidget *widget;
	Ui::EngineModelDesignClass ui;
};
