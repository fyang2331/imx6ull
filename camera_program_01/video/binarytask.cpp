#include "binarytask.h"

// 构造函数：接收输入图像
BinaryTask::BinaryTask(const QImage& inputImage) : inputImage(inputImage) {}

// 执行任务
void BinaryTask::run() {
    // 调用二值化处理函数
    QImage binaryImage = binary(inputImage);

    // 发射信号，将处理结果传递给主线程
    emit binaryImageReady(binaryImage);
}

// 二值化处理逻辑
QImage BinaryTask::binary(const QImage& image, uchar threshold) {
    // 检查输入图像是否为空
    if (image.isNull()) {
        return QImage();
    }

    // 转换图像为灰度格式
    QImage grayImage = image.convertToFormat(QImage::Format_Grayscale8);

    // 创建与灰度图像相同大小的二值化图像
    QImage binaryImage(grayImage.size(), QImage::Format_Grayscale8);

    // 遍历每个像素，进行二值化处理
    for (int y = 0; y < grayImage.height(); ++y) {
        uchar* binaryLine = binaryImage.scanLine(y);  // 获取二值化图像的扫描行
        const uchar* grayLine = grayImage.constScanLine(y);  // 获取灰度图像的扫描行

        for (int x = 0; x < grayImage.width(); ++x) {
            // 使用动态阈值，将灰度值与指定阈值比较
            binaryLine[x] = grayLine[x] > threshold ? 255 : 0;
        }
    }

    return binaryImage;  // 返回处理后的二值化图像
}

