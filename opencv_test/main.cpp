#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // 打开摄像头
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "错误：无法打开摄像头" << std::endl;
        return -1;
    }

    // 设置分辨率
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    cv::Mat frame;
    std::cout << "摄像头已打开，按 'q' 键退出，按 's' 键保存图片。" << std::endl;

    while (true) {
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "无法接收视频帧" << std::endl;
            break;
        }

        cv::imshow("Camera Feed (IMX6U C++)", frame);

        char key = cv::waitKey(1);
        if (key == 'q') {
            break;
        } else if (key == 's') {
            cv::imwrite("capture_cpp.jpg", frame);
            std::cout << "图片已保存为 capture_cpp.jpg" << std::endl;
        }
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}