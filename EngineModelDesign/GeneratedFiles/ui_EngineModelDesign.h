/********************************************************************************
** Form generated from reading UI file 'EngineModelDesign.ui'
**
** Created by: Qt User Interface Compiler version 5.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ENGINEMODELDESIGN_H
#define UI_ENGINEMODELDESIGN_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_EngineModelDesignClass
{
public:
    QWidget *centralWidget;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *EngineModelDesignClass)
    {
        if (EngineModelDesignClass->objectName().isEmpty())
            EngineModelDesignClass->setObjectName(QStringLiteral("EngineModelDesignClass"));
        EngineModelDesignClass->resize(1600, 1000);
        centralWidget = new QWidget(EngineModelDesignClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        EngineModelDesignClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(EngineModelDesignClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 1600, 26));
        EngineModelDesignClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(EngineModelDesignClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        EngineModelDesignClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(EngineModelDesignClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        EngineModelDesignClass->setStatusBar(statusBar);

        retranslateUi(EngineModelDesignClass);

        QMetaObject::connectSlotsByName(EngineModelDesignClass);
    } // setupUi

    void retranslateUi(QMainWindow *EngineModelDesignClass)
    {
        EngineModelDesignClass->setWindowTitle(QApplication::translate("EngineModelDesignClass", "EngineModelDesign", nullptr));
    } // retranslateUi

};

namespace Ui {
    class EngineModelDesignClass: public Ui_EngineModelDesignClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ENGINEMODELDESIGN_H
