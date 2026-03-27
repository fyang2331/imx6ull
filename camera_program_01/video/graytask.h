#ifndef GRAYTASK_H
#define GRAYTASK_H

#include <QObject>
#include <QRunnable>
#include <QImage>

class GrayTask : public QObject, public QRunnable {
    Q_OBJECT
public:
    explicit GrayTask(const QImage& image, QObject* parent = nullptr);

    // 重载 run 方法，供线程池调用
    void run() override;

signals:
    // 信号：灰度化图像准备完毕
    void grayImageReady(const QImage& grayImage);

private:
    // 灰度化函数
    QImage gray(const QImage& image);

    QImage inputImage; // 输入图像
};

#endif // GRAYTASK_H
