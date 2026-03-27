#ifndef BINARYTASK_H
#define BINARYTASK_H

#include <QObject>
#include <QRunnable>
#include <QImage>

class BinaryTask : public QObject, public QRunnable {
    Q_OBJECT
public:
    explicit BinaryTask(const QImage& inputImage); // 构造函数

    // 重写 run() 函数以处理二值化任务
    void run() override;

signals:
    // 处理完成后发送二值化图像信号
    void binaryImageReady(const QImage& binaryImage);

private:
    QImage inputImage;

    // 二值化处理函数
    QImage binary(const QImage& image, uchar threshold = 127);
};

#endif // BINARYTASK_H
