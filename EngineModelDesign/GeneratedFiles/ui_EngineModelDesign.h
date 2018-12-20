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
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_EngineModelDesignClass
{
public:
    QAction *actionopen;
    QAction *actionsave;
    QAction *actionexportanimation;
    QAction *actionadd_metallic;
    QAction *actionadd_roughness;
    QAction *actionadd_ao;
    QWidget *centralWidget;
    QComboBox *comboBox;
    QLabel *label;
    QSlider *model_scal;
    QLabel *label_1;
    QLabel *label_model_scal;
    QCheckBox *CheckIfBoundBox;
    QLabel *label_4;
    QSlider *model_animation;
    QLabel *label_model_animation;
    QRadioButton *show_normal_vertex;
    QRadioButton *show_normal_face;
    QCheckBox *CheckIfShowNormal;
    QLabel *label_now_face;
    QLabel *label_2;
    QLabel *label_3;
    QTextBrowser *face_point_message;
    QLabel *label_5;
    QLabel *label_face_normal;
    QLabel *label_animation_type;
    QLabel *label_6;
    QMenuBar *menuBar;
    QMenu *menu_file;
    QMenu *menu_edit;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *EngineModelDesignClass)
    {
        if (EngineModelDesignClass->objectName().isEmpty())
            EngineModelDesignClass->setObjectName(QStringLiteral("EngineModelDesignClass"));
        EngineModelDesignClass->resize(1600, 1000);
        actionopen = new QAction(EngineModelDesignClass);
        actionopen->setObjectName(QStringLiteral("actionopen"));
        actionsave = new QAction(EngineModelDesignClass);
        actionsave->setObjectName(QStringLiteral("actionsave"));
        actionexportanimation = new QAction(EngineModelDesignClass);
        actionexportanimation->setObjectName(QStringLiteral("actionexportanimation"));
        actionadd_metallic = new QAction(EngineModelDesignClass);
        actionadd_metallic->setObjectName(QStringLiteral("actionadd_metallic"));
        actionadd_roughness = new QAction(EngineModelDesignClass);
        actionadd_roughness->setObjectName(QStringLiteral("actionadd_roughness"));
        actionadd_ao = new QAction(EngineModelDesignClass);
        actionadd_ao->setObjectName(QStringLiteral("actionadd_ao"));
        centralWidget = new QWidget(EngineModelDesignClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        comboBox = new QComboBox(centralWidget);
        comboBox->setObjectName(QStringLiteral("comboBox"));
        comboBox->setGeometry(QRect(100, 20, 121, 22));
        label = new QLabel(centralWidget);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(0, 20, 91, 16));
        model_scal = new QSlider(centralWidget);
        model_scal->setObjectName(QStringLiteral("model_scal"));
        model_scal->setGeometry(QRect(1390, 90, 160, 22));
        model_scal->setValue(50);
        model_scal->setOrientation(Qt::Horizontal);
        label_1 = new QLabel(centralWidget);
        label_1->setObjectName(QStringLiteral("label_1"));
        label_1->setGeometry(QRect(1321, 90, 81, 21));
        label_model_scal = new QLabel(centralWidget);
        label_model_scal->setObjectName(QStringLiteral("label_model_scal"));
        label_model_scal->setGeometry(QRect(1450, 70, 51, 16));
        CheckIfBoundBox = new QCheckBox(centralWidget);
        CheckIfBoundBox->setObjectName(QStringLiteral("CheckIfBoundBox"));
        CheckIfBoundBox->setGeometry(QRect(1330, 200, 101, 19));
        label_4 = new QLabel(centralWidget);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setGeometry(QRect(1321, 140, 81, 21));
        model_animation = new QSlider(centralWidget);
        model_animation->setObjectName(QStringLiteral("model_animation"));
        model_animation->setGeometry(QRect(1390, 140, 160, 22));
        model_animation->setValue(0);
        model_animation->setOrientation(Qt::Horizontal);
        label_model_animation = new QLabel(centralWidget);
        label_model_animation->setObjectName(QStringLiteral("label_model_animation"));
        label_model_animation->setGeometry(QRect(1450, 120, 51, 16));
        show_normal_vertex = new QRadioButton(centralWidget);
        show_normal_vertex->setObjectName(QStringLiteral("show_normal_vertex"));
        show_normal_vertex->setGeometry(QRect(1350, 270, 115, 19));
        show_normal_face = new QRadioButton(centralWidget);
        show_normal_face->setObjectName(QStringLiteral("show_normal_face"));
        show_normal_face->setGeometry(QRect(1350, 300, 115, 19));
        CheckIfShowNormal = new QCheckBox(centralWidget);
        CheckIfShowNormal->setObjectName(QStringLiteral("CheckIfShowNormal"));
        CheckIfShowNormal->setGeometry(QRect(1330, 240, 91, 16));
        label_now_face = new QLabel(centralWidget);
        label_now_face->setObjectName(QStringLiteral("label_now_face"));
        label_now_face->setGeometry(QRect(1490, 330, 81, 20));
        label_2 = new QLabel(centralWidget);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(1370, 330, 101, 21));
        label_3 = new QLabel(centralWidget);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setGeometry(QRect(1370, 360, 151, 16));
        face_point_message = new QTextBrowser(centralWidget);
        face_point_message->setObjectName(QStringLiteral("face_point_message"));
        face_point_message->setGeometry(QRect(1370, 380, 211, 61));
        label_5 = new QLabel(centralWidget);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setGeometry(QRect(1370, 450, 121, 16));
        label_face_normal = new QLabel(centralWidget);
        label_face_normal->setObjectName(QStringLiteral("label_face_normal"));
        label_face_normal->setGeometry(QRect(1490, 450, 91, 16));
        label_animation_type = new QLabel(centralWidget);
        label_animation_type->setObjectName(QStringLiteral("label_animation_type"));
        label_animation_type->setGeometry(QRect(1490, 170, 61, 20));
        label_6 = new QLabel(centralWidget);
        label_6->setObjectName(QStringLiteral("label_6"));
        label_6->setGeometry(QRect(1410, 170, 71, 21));
        EngineModelDesignClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(EngineModelDesignClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 1600, 26));
        menu_file = new QMenu(menuBar);
        menu_file->setObjectName(QStringLiteral("menu_file"));
        menu_edit = new QMenu(menuBar);
        menu_edit->setObjectName(QStringLiteral("menu_edit"));
        EngineModelDesignClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(EngineModelDesignClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        EngineModelDesignClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(EngineModelDesignClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        EngineModelDesignClass->setStatusBar(statusBar);

        menuBar->addAction(menu_file->menuAction());
        menuBar->addAction(menu_edit->menuAction());
        menu_file->addAction(actionopen);
        menu_file->addAction(actionsave);
        menu_file->addAction(actionexportanimation);
        menu_edit->addAction(actionadd_metallic);
        menu_edit->addAction(actionadd_roughness);
        menu_edit->addAction(actionadd_ao);

        retranslateUi(EngineModelDesignClass);

        QMetaObject::connectSlotsByName(EngineModelDesignClass);
    } // setupUi

    void retranslateUi(QMainWindow *EngineModelDesignClass)
    {
        EngineModelDesignClass->setWindowTitle(QApplication::translate("EngineModelDesignClass", "EngineModelDesign", nullptr));
        actionopen->setText(QApplication::translate("EngineModelDesignClass", "\346\211\223\345\274\200\346\226\207\344\273\266", nullptr));
        actionsave->setText(QApplication::translate("EngineModelDesignClass", "\345\257\274\345\207\272\346\250\241\345\236\213", nullptr));
        actionexportanimation->setText(QApplication::translate("EngineModelDesignClass", "\345\257\274\345\207\272\345\212\250\347\224\273", nullptr));
        actionadd_metallic->setText(QApplication::translate("EngineModelDesignClass", "\345\257\274\345\205\245\351\207\221\345\261\236\345\272\246\345\210\260\345\275\223\345\211\215\346\250\241\345\236\213\345\235\227", nullptr));
        actionadd_roughness->setText(QApplication::translate("EngineModelDesignClass", "\345\257\274\345\205\245\347\262\227\347\263\231\345\272\246\345\210\260\345\275\223\345\211\215\346\250\241\345\236\213\345\235\227", nullptr));
        actionadd_ao->setText(QApplication::translate("EngineModelDesignClass", "\345\257\274\345\205\245ao\350\264\264\345\233\276\345\210\260\345\275\223\345\211\215\346\250\241\345\236\213\345\235\227", nullptr));
        label->setText(QApplication::translate("EngineModelDesignClass", "\351\200\211\346\213\251\346\250\241\345\236\213\345\235\227", nullptr));
        label_1->setText(QApplication::translate("EngineModelDesignClass", "\346\250\241\345\236\213\347\274\251\346\224\276", nullptr));
        label_model_scal->setText(QApplication::translate("EngineModelDesignClass", "1.0", nullptr));
        CheckIfBoundBox->setText(QApplication::translate("EngineModelDesignClass", "\346\230\276\347\244\272\345\214\205\345\233\264\347\233\222", nullptr));
        label_4->setText(QApplication::translate("EngineModelDesignClass", "\346\250\241\345\236\213\345\212\250\347\224\273", nullptr));
        label_model_animation->setText(QApplication::translate("EngineModelDesignClass", "0.0", nullptr));
        show_normal_vertex->setText(QApplication::translate("EngineModelDesignClass", "\351\241\266\347\202\271\346\263\225\347\272\277", nullptr));
        show_normal_face->setText(QApplication::translate("EngineModelDesignClass", "\351\235\242\346\263\225\347\272\277", nullptr));
        CheckIfShowNormal->setText(QApplication::translate("EngineModelDesignClass", "\346\230\276\347\244\272\346\263\225\347\272\277", nullptr));
        label_now_face->setText(QApplication::translate("EngineModelDesignClass", "-1", nullptr));
        label_2->setText(QApplication::translate("EngineModelDesignClass", "\345\275\223\345\211\215\346\211\200\351\200\211\351\235\242ID:", nullptr));
        label_3->setText(QApplication::translate("EngineModelDesignClass", "\345\275\223\345\211\215\346\211\200\351\200\211\351\235\242\351\241\266\347\202\271\344\277\241\346\201\257:", nullptr));
        face_point_message->setHtml(QApplication::translate("EngineModelDesignClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'SimSun'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">point1:(0,0,0);</p>\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">point2:(0,0,0);</p>\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">point3:(0,0,0);</p></body></html>", nullptr));
        label_5->setText(QApplication::translate("EngineModelDesignClass", "\345\275\223\345\211\215\346\211\200\351\200\211\351\235\242\346\263\225\347\272\277:", nullptr));
        label_face_normal->setText(QApplication::translate("EngineModelDesignClass", "(0,0,0)", nullptr));
        label_animation_type->setText(QApplication::translate("EngineModelDesignClass", "\351\252\250\351\252\274\345\212\250\347\224\273", nullptr));
        label_6->setText(QApplication::translate("EngineModelDesignClass", "\345\212\250\347\224\273\347\261\273\345\236\213\357\274\232", nullptr));
        menu_file->setTitle(QApplication::translate("EngineModelDesignClass", "\346\226\207\344\273\266", nullptr));
        menu_edit->setTitle(QApplication::translate("EngineModelDesignClass", "\347\274\226\350\276\221", nullptr));
    } // retranslateUi

};

namespace Ui {
    class EngineModelDesignClass: public Ui_EngineModelDesignClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ENGINEMODELDESIGN_H
