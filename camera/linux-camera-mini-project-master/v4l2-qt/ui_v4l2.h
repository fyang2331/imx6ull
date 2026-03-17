/********************************************************************************
** Form generated from reading UI file 'v4l2.ui'
**
** Created by: Qt User Interface Compiler version 5.12.9
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_V4L2_H
#define UI_V4L2_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_v4l2
{
public:
    QWidget *centralwidget;
    QLabel *label;
    QPushButton *pushButton_take;
    QPushButton *pushButton_open;
    QPushButton *pushButton_back;
    QPushButton *pushButton_photos;
    QLabel *label_2;

    void setupUi(QMainWindow *v4l2)
    {
        if (v4l2->objectName().isEmpty())
            v4l2->setObjectName(QString::fromUtf8("v4l2"));
        v4l2->resize(800, 480);
        QFont font;
        font.setPointSize(13);
        v4l2->setFont(font);
        centralwidget = new QWidget(v4l2);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        label = new QLabel(centralwidget);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(-1, 0, 711, 480));
        QFont font1;
        font1.setPointSize(28);
        font1.setBold(true);
        font1.setWeight(75);
        label->setFont(font1);
        label->setStyleSheet(QString::fromUtf8("color: rgb(245, 121, 0);"));
        label->setAlignment(Qt::AlignCenter);
        pushButton_take = new QPushButton(centralwidget);
        pushButton_take->setObjectName(QString::fromUtf8("pushButton_take"));
        pushButton_take->setGeometry(QRect(699, 280, 101, 61));
        QFont font2;
        font2.setPointSize(16);
        font2.setBold(true);
        font2.setWeight(75);
        pushButton_take->setFont(font2);
        pushButton_take->setStyleSheet(QString::fromUtf8("background-color: rgb(206, 92, 0);"));
        pushButton_open = new QPushButton(centralwidget);
        pushButton_open->setObjectName(QString::fromUtf8("pushButton_open"));
        pushButton_open->setGeometry(QRect(699, 0, 101, 61));
        pushButton_open->setFont(font2);
        pushButton_open->setStyleSheet(QString::fromUtf8("background-color: rgb(206, 92, 0);"));
        pushButton_back = new QPushButton(centralwidget);
        pushButton_back->setObjectName(QString::fromUtf8("pushButton_back"));
        pushButton_back->setGeometry(QRect(699, 419, 101, 61));
        pushButton_back->setFont(font2);
        pushButton_back->setStyleSheet(QString::fromUtf8("background-color: rgb(206, 92, 0);"));
        pushButton_photos = new QPushButton(centralwidget);
        pushButton_photos->setObjectName(QString::fromUtf8("pushButton_photos"));
        pushButton_photos->setGeometry(QRect(699, 140, 101, 61));
        pushButton_photos->setFont(font2);
        pushButton_photos->setStyleSheet(QString::fromUtf8("background-color: rgb(206, 92, 0);"));
        label_2 = new QLabel(centralwidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(0, 0, 800, 480));
        label_2->setStyleSheet(QString::fromUtf8("background-color: rgb(0, 0, 0);"));
        v4l2->setCentralWidget(centralwidget);
        label_2->raise();
        label->raise();
        pushButton_take->raise();
        pushButton_open->raise();
        pushButton_back->raise();
        pushButton_photos->raise();

        retranslateUi(v4l2);

        QMetaObject::connectSlotsByName(v4l2);
    } // setupUi

    void retranslateUi(QMainWindow *v4l2)
    {
        v4l2->setWindowTitle(QApplication::translate("v4l2", "v4l2", nullptr));
        label->setText(QApplication::translate("v4l2", "\345\233\276\345\203\217\345\214\272\345\237\237", nullptr));
        pushButton_take->setText(QApplication::translate("v4l2", "\346\213\215\347\205\247", nullptr));
        pushButton_open->setText(QApplication::translate("v4l2", "\346\211\223\345\274\200", nullptr));
        pushButton_back->setText(QApplication::translate("v4l2", "\350\277\224\345\233\236", nullptr));
        pushButton_photos->setText(QApplication::translate("v4l2", "\347\233\270\345\206\214", nullptr));
        label_2->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class v4l2: public Ui_v4l2 {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_V4L2_H
