/********************************************************************************
** Form generated from reading UI file 'showphoto.ui'
**
** Created by: Qt User Interface Compiler version 5.12.9
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SHOWPHOTO_H
#define UI_SHOWPHOTO_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_showphoto
{
public:
    QWidget *centralwidget;
    QLabel *label;
    QPushButton *pushButton_front;
    QPushButton *pushButton_next;
    QLabel *label_2;
    QPushButton *pushButton_back;

    void setupUi(QMainWindow *showphoto)
    {
        if (showphoto->objectName().isEmpty())
            showphoto->setObjectName(QString::fromUtf8("showphoto"));
        showphoto->resize(800, 480);
        centralwidget = new QWidget(showphoto);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        label = new QLabel(centralwidget);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(80, 0, 640, 480));
        QFont font;
        font.setPointSize(28);
        label->setFont(font);
        label->setStyleSheet(QString::fromUtf8("color: rgb(204, 0, 0);"));
        label->setAlignment(Qt::AlignCenter);
        pushButton_front = new QPushButton(centralwidget);
        pushButton_front->setObjectName(QString::fromUtf8("pushButton_front"));
        pushButton_front->setGeometry(QRect(0, 190, 81, 91));
        QFont font1;
        font1.setPointSize(24);
        pushButton_front->setFont(font1);
        pushButton_front->setStyleSheet(QString::fromUtf8("color: rgb(85, 87, 83);\n"
"background-color: rgb(245, 121, 0);"));
        pushButton_next = new QPushButton(centralwidget);
        pushButton_next->setObjectName(QString::fromUtf8("pushButton_next"));
        pushButton_next->setGeometry(QRect(720, 190, 81, 91));
        pushButton_next->setFont(font1);
        pushButton_next->setStyleSheet(QString::fromUtf8("color: rgb(85, 87, 83);\n"
"background-color: rgb(245, 121, 0);"));
        label_2 = new QLabel(centralwidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(0, 0, 800, 480));
        label_2->setStyleSheet(QString::fromUtf8("background-color: rgb(0, 0, 0);"));
        pushButton_back = new QPushButton(centralwidget);
        pushButton_back->setObjectName(QString::fromUtf8("pushButton_back"));
        pushButton_back->setGeometry(QRect(0, 0, 81, 51));
        pushButton_back->setFont(font1);
        pushButton_back->setStyleSheet(QString::fromUtf8("color: rgb(85, 87, 83);\n"
"background-color: rgb(245, 121, 0);"));
        showphoto->setCentralWidget(centralwidget);
        label_2->raise();
        label->raise();
        pushButton_front->raise();
        pushButton_next->raise();
        pushButton_back->raise();

        retranslateUi(showphoto);

        QMetaObject::connectSlotsByName(showphoto);
    } // setupUi

    void retranslateUi(QMainWindow *showphoto)
    {
        showphoto->setWindowTitle(QApplication::translate("showphoto", "MainWindow", nullptr));
        label->setText(QApplication::translate("showphoto", "\346\227\240\345\233\276\347\211\207", nullptr));
        pushButton_front->setText(QApplication::translate("showphoto", "<", nullptr));
        pushButton_next->setText(QApplication::translate("showphoto", ">", nullptr));
        label_2->setText(QString());
        pushButton_back->setText(QApplication::translate("showphoto", "<<", nullptr));
    } // retranslateUi

};

namespace Ui {
    class showphoto: public Ui_showphoto {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SHOWPHOTO_H
