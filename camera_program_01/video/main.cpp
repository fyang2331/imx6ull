#include "mainwindow.h"

#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置线程池的最大线程数
    QThreadPool::globalInstance()->setMaxThreadCount(5);

    MainWindow w;
    w.show();
    return a.exec();
}
