#include "graytask.h"

// 构造函数
GrayTask::GrayTask(const QImage& image, QObject* parent)
    : QObject(parent), inputImage(image) {}

// 执行任务
void GrayTask::run() {
    // 灰度化图像
    QImage grayImage = gray(inputImage);

    // 发射信号，将灰度化的图像传递给主线程
    emit grayImageReady(grayImage);
}

// 灰度化方法
QImage GrayTask::gray(const QImage& image) {
    // 检查输入图像格式，转换为适合处理的格式
    QImage convertedImage = image.convertToFormat(QImage::Format_Grayscale8);

    // 返回处理后的灰度图像
    return convertedImage;
}
