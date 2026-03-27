#ifndef EDGEDETECTION_H
#define EDGEDETECTION_H

#include <QObject>
#include <QRunnable>
#include <QImage>
#include <cmath>  // 包含 sqrt 的声明
#include <QtMath>
#include <QtGlobal>  // 包含 qBound 的声明

class EdgeDetection : public QObject, public QRunnable {
    Q_OBJECT
public:
    explicit EdgeDetection(const QImage& inputImage);

    // 重写 run() 方法，用于执行边缘检测任务
    void run() override;

signals:
    // 任务完成时发送信号
    void edgeImageReady(const QImage& edgeImage);

private:
    QImage inputImage;

    // 边缘检测函数
    static QImage detectEdges(const QImage& image);
};

#endif // EDGEDETECTION_H
