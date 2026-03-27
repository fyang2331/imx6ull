#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QDebug>
#include <QTimer>
#include <QIODevice>
#include <QDateTime>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>
#include <linux/videodev2.h>
#include <stdbool.h>
#include <string.h>

#include <QDateTime>
#include <QMediaPlayer>
#include <QProgressBar>
#include <QSlider>
#include <QFileDialog>
#include <QThreadPool>
#include <QMessageBox>
#include <QUdpSocket>
#include <QImage>
#include <QBuffer>
#include <QByteArray>
#include <QWidget>
#include <QtCore>


#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv/cv.h>
#include <opencv2/opencv.hpp>

#include "graytask.h"
#include "binarytask.h"
#include "edgedetection.h"
#include "processimage.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


/* 命名空间 */
using namespace std;

#define FRAMEBUFFER_COUNT 2
#define PIXWIDTH    640
#define PIXHEIGHT   480

/* buffer描述信息 */
struct buffer_info {
    void *start;
    unsigned int length;
};

/* 摄像头像素格式及其描述信息 */
typedef struct camera_format {
    unsigned char description[32];  //字符串描述信息
    unsigned int pixelformat;       //像素格式
} cam_fmt;

/* 帧描述信息 */
typedef struct Frame_Buffer{
    uchar *buf;
    uint length;
}FrameBuffer;



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


    int v4l2DevInit();
    int v4l2SetFormat();
    int v4l2_init_buffer();
    int v4l2StreamOn();
    int v4l2GetOneFrame(FrameBuffer *framebuf);
    void resumeVideoStream(float fps);
    void updateVideoProgress();
    void updateVideoFrame();
    void stopVideoPlayback();
    void stopVideoStream();

    void sendImageToServer(const QByteArray &byteArray);
    void imageProcessed(const QByteArray &processedData);

    int g_fps = 33;
    // 客户端的成员变量
    QUdpSocket *udpSocket;
    QThreadPool *threadPool;

private:
    Ui::MainWindow *ui;
    QImage resizedImageProcessImage;
    QSlider* videoSlider = nullptr;  // 动态创建的进度条
    QProgressBar* videoProgressBar = nullptr;
    QTimer *timer;
    int v4l2_fd;
    uint16_t sequenceNumber = 0; // 初始化为 0

    bool isRecording = false;  // 标记当前是否正在录制
    bool isGrayProcessing = false;
    bool isBinaryProcessing = false;     // 标志是否正在进行二值化处理
    bool isEdgeProcessing = false;
    bool isAutoExposureEnabled = false;  // 自动曝光开关
    bool isAutoGainEnabled = false;
    bool isGammaCorrectionEnabled = false;
    bool isConnectServer = false;




    cv::VideoWriter videoWriter;  // 用于写入视频的 VideoWriter 对象
    cv::VideoCapture videoCapture;
    cv::Mat currentFrame;

    QMediaPlayer* videoPlayer = nullptr;  // 用于播放视频的媒体播放器

    QTimer* videoUpdateTimer = nullptr;  // 用于定时更新进度条
    QTimer* videoGrayTimer = nullptr;
    QTimer* binaryTimer;         // 定时器，用于定时处理帧
    QTimer* edgeTimer;
    QTimer* ProcessImageTimer = nullptr;

    std::vector<cv::Mat> frames;  // 用来存储每一帧图像的容器
    /* buffer */
    struct buffer_info *buffer_infos;

    cam_fmt cam_fmts[10];

    void layoutInit();

    std::unique_ptr<unsigned char[]> frame_buffer;
    size_t frame_buffer_size;

    std::atomic<bool> is_closing;  // 原子标志位，表示正在关闭
    std::mutex v4l2_mutex;         // 保护 V4L2 资源的互斥锁

private slots:
    void timerStart(float fps);
    void videoShow();
    void on_start_pushButton_clicked();
    void on_close_pushButton_clicked();
    void on_takePicture_pushButton_clicked();
    void on_playPicture_pushButton_clicked();
    void on_closePic_pushButton_clicked();
    void on_videoRecord_pushButton_clicked();
    void on_playVideo_pushButton_clicked();
    void on_closeVideo_pushButton_clicked();
    void on_sliderValueChanged(int value);
    void on_playGray_pushButton_clicked();
    void processGrayImage(const QImage& grayImage);  // 处理灰度化图像
    void startGrayProcessing();  // 启动灰度化任务
    void startProcessImage(); //每 g_fps 秒创建并执行一个新的 ProcessImage 任务

    void on_playBinary_pushButton_clicked();
   void startBinaryProcessing();// 定时触发的槽函数，开始处理下一帧
   void processBinaryImage(const QImage& binaryImage);// 接收二值化任务完成后的图像

   void on_playEdge_pushButton_clicked();
   void startEdgeProcessing();
   void processEdgeImage(const QImage& edgeImage);

   void on_adjustFrame_horizontalSlider_valueChanged(int value);// 滑块移动时的槽函数

   void on_autoExp_pushButton_clicked();
   void on_autoGain_pushButton_clicked();
   void on_gammaCor_pushButton_clicked();
   void on_uploadServer_pushButton_clicked();
   void uploadDataToServer(const QByteArray &data);
};
#endif // MAINWINDOW_H
