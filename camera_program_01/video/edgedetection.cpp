#include "edgedetection.h"
#include <QtDebug>
#include <QColor>
#include <opencv2/opencv.hpp>

// 构造函数
EdgeDetection::EdgeDetection(const QImage& inputImage) : inputImage(inputImage) {}

// 重写 run() 方法
void EdgeDetection::run() {
    if (inputImage.isNull()) {
        qWarning() << "Input image is null!";
        return;
    }

    // 进行边缘检测
    QImage edgeImage = detectEdges(inputImage);

    // 发射信号，通知主线程
    emit edgeImageReady(edgeImage);
}

// 边缘检测实现
QImage EdgeDetection::detectEdges(const QImage& image) {
    // 将 QImage 转换为 cv::Mat 格式
    cv::Mat inputMat = cv::Mat(image.height(), image.width(), CV_8UC4, const_cast<uchar*>(image.bits()), image.bytesPerLine());
    cv::Mat bgrMat;
    cv::cvtColor(inputMat, bgrMat, cv::COLOR_BGRA2BGR); // 如果输入是带 Alpha 通道的图像

    // 转换为灰度图
    cv::Mat grayMat;
    cv::cvtColor(bgrMat, grayMat, cv::COLOR_BGR2GRAY);

    // 使用 Canny 进行边缘检测
    cv::Mat edgesMat;
    double lowThreshold = 50;  // 低阈值
    double highThreshold = 150; // 高阈值
    cv::Canny(grayMat, edgesMat, lowThreshold, highThreshold);

    // 将结果 cv::Mat 转换回 QImage
    QImage edgeImage(edgesMat.data, edgesMat.cols, edgesMat.rows, edgesMat.step, QImage::Format_Grayscale8);

    return edgeImage.copy(); // 确保数据安全，返回拷贝
}
