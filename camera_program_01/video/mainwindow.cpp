#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QGuiApplication>
#include <QScreen>
#include <QFile>
#include <QCoreApplication>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

void initFFmpeg() {
     avformat_network_init();
}

// 清理FFmpeg
void cleanupFFmpeg() {
    avformat_network_deinit();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    threadPool = new QThreadPool();
    threadPool->setMaxThreadCount(4); // 设置最大线程数为4
    // 初始化FFmpeg
    initFFmpeg();

    layoutInit();
    v4l2DevInit();
    v4l2StreamOn();
}

MainWindow::~MainWindow()
{
    /* 停止视频流 */
    int i;
    if (v4l2_fd != -1) {
        for (i = 0; i < FRAMEBUFFER_COUNT; ++i) {
            if (buffer_infos[i].start) {
                munmap(buffer_infos[i].start, buffer_infos[i].length);
            }
        }

        // 停止视频流
        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(v4l2_fd, VIDIOC_STREAMOFF, &type);
        ::close(v4l2_fd);
        v4l2_fd = -1;
    }

    if (videoCapture.isOpened()) {
        videoCapture.release();  // 释放视频捕获
    }

    if (videoUpdateTimer != nullptr) {
        videoUpdateTimer->stop();  // 停止定时器
        delete videoUpdateTimer;
        videoUpdateTimer = nullptr;
    }

    // 清除滑块
    if (videoSlider != nullptr) {
        videoSlider->setValue(0);
    }

    if (udpSocket) {
        udpSocket->close();  // 关闭UDP连接
        delete udpSocket;    // 释放内存
        udpSocket = nullptr; // 防止野指针
        qDebug() << "UDP socket disconnected.";
    }

    cleanupFFmpeg();
    delete ui;
}

/* 界面初始化 */
void MainWindow::layoutInit()
{
    QList <QScreen *> list_screen = QGuiApplication::screens();

    /* 适配ARM架构屏幕大小 */
    #if __arm__
    /* 设置全屏 */
    this->resize(list_screen.at(0)->geometry().width(),
                list_screen.at(0)->geometry().height());
    qDebug() << "width:" << list_screen.at(0)->geometry().width();
    qDebug() << "height:" << list_screen.at(0)->geometry().height();

    #else
    /* 如果是PC则设置固定大小为800x480 */
    this->resize(800, 480);
    #endif

    // 设置帧率滑块
    ui->adjustFrame_horizontalSlider->setValue(g_fps);     // 设置当前帧率值
    ui->adjustFrame_horizontalSlider->setRange(30, 100);   // 设置帧率范围

    ui->start_pushButton->setEnabled(true);
    ui->close_pushButton->setEnabled(false);

    /* 设置图片自适应 */
    ui->origal_label->setScaledContents(true);
    ui->gray_label->setScaledContents(true);
    ui->binar_label->setScaledContents(true);
    ui->edge_label->setScaledContents(true);
    ui->centralwidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

int MainWindow::v4l2DevInit()
{
    // 打开设备
    v4l2_fd = ::open("/dev/video2", O_RDWR);
    if (v4l2_fd < 0) {
        qWarning("无法打开设备");
        return false;
    }

    // 查询设备功能
    struct v4l2_capability cap;
    if(::ioctl(v4l2_fd, VIDIOC_QUERYCAP,&cap) < 0 || !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        qWarning("设备不支持视频捕获");
        ::close(v4l2_fd);
        return -1;
    }

    // 输出设备信息
    printf("Device Information:\n");
    printf("driver name: %s\ncard name: %s\n",cap.driver,cap.card);
    printf("------------------------------------\n");

    // 查询支持的格式
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("Support Format:\n");
    /* 清空结构体 */
    memset(&cam_fmts,0,sizeof(cam_fmts));
    while(0 == ::ioctl(v4l2_fd, VIDIOC_ENUM_FMT, &fmtdesc))
    {
        cam_fmts[fmtdesc.index].pixelformat  = fmtdesc.pixelformat;
        memcpy(cam_fmts[fmtdesc.index].description, fmtdesc.description,sizeof(fmtdesc.description));
        fmtdesc.index++;
    }

    // 查询支持的帧大小和帧率
    struct v4l2_frmsizeenum frmsize;
    struct v4l2_frmivalenum frmival;
    int i;
    frmsize.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frmival.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    for(i = 0; cam_fmts[i].pixelformat; i++)
    {
        printf("format<0x%x>, description<%s>\n", cam_fmts[i].pixelformat, cam_fmts[i].description);
        /* 枚举该格式支持的帧大小和帧率 */
        frmsize.index = 0;
        frmsize.pixel_format = cam_fmts[i].pixelformat;
        frmival.pixel_format = cam_fmts[i].pixelformat;
        while (0 == ::ioctl(v4l2_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize)) {
            printf("size<%d*%d> ",frmsize.discrete.width,frmsize.discrete.height);
            frmsize.index++;

            /* 枚举该分辨率支持的帧率 */
            while (0 == ::ioctl(v4l2_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival)) {
                printf("<%dfps>", frmival.discrete.denominator / frmival.discrete.numerator);
                frmival.index++;
            }
            printf("\n");
        }
        printf("\n");
    }
    printf("-------------------------------------\n");

    /* 设置视频格式 */
    if(v4l2SetFormat()<0){
        printf("set format failed\n");
        return -1;
    }
    return 0;
}

int MainWindow::v4l2SetFormat()
{
    // 设置格式为640x480 MJPEG格式
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = PIXWIDTH;
    fmt.fmt.pix.height = PIXHEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG; // 设置为MJPEG格式
    if (::ioctl(v4l2_fd, VIDIOC_S_FMT, &fmt) < 0) {
        qWarning("设置格式失败");
        ::close(v4l2_fd);
        return -1;
    }

    qDebug() << "set format success\r\n";

    /* 检查是否成功设置为MJPEG格式 */
    if (V4L2_PIX_FMT_MJPEG != fmt.fmt.pix.pixelformat) {
        printf("Error: the device does not support MJPEG format!\n");
        return -1;
    }

    /* 输出实际设置的参数 */
    printf("实际设置大小<%d * %d>, 色彩空间:%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height,fmt.fmt.pix.colorspace);

    //获取流参数
    struct v4l2_streamparm streamparm;
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (0 > ::ioctl(v4l2_fd, VIDIOC_G_PARM, &streamparm)) {
        printf("get parm failed\n");
        return -1;
    }

    /* 检查是否支持帧率设置 */
    if (V4L2_CAP_TIMEPERFRAME & streamparm.parm.capture.capability) {
        streamparm.parm.capture.timeperframe.numerator = 1;
        streamparm.parm.capture.timeperframe.denominator = 30;//30fps
        if (0 > ::ioctl(v4l2_fd, VIDIOC_S_PARM, &streamparm)) {
            printf("Error:device do not support set fps\n");
            return -1;
        }
    }
    return 0;
}

int MainWindow::v4l2_init_buffer()
{
    // 申请内核缓冲区
    struct v4l2_requestbuffers reqbuf;
    memset(&reqbuf, 0, sizeof(reqbuf));  // 建议清零
    reqbuf.count = FRAMEBUFFER_COUNT;       // 缓冲区数量
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    
    if (ioctl(v4l2_fd, VIDIOC_REQBUFS, &reqbuf) < 0) {
        perror("request buffer failed");
        return -1;
    }
    printf("request buffer success\n");
    printf("Driver supports up to %d buffers.\n", reqbuf.count);

    /* 映射内核缓冲区到用户空间 */
    struct v4l2_buffer buf;
    unsigned int n_buffers = 0;
    
    /* calloc初始化buffer_infos为0*/
    buffer_infos = (struct buffer_info*)calloc(FRAMEBUFFER_COUNT, sizeof(struct buffer_info));
    if (!buffer_infos) {
        printf("calloc buffer_infos failed\n");
        return -1;
    }

    for (n_buffers = 0; n_buffers < FRAMEBUFFER_COUNT; n_buffers++) {
        memset(&buf, 0, sizeof(buf));
        buf.index = n_buffers;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        
        if (ioctl(v4l2_fd, VIDIOC_QUERYBUF, &buf) < 0) {
            perror("VIDIOC_QUERYBUF failed");
            return -1;
        }

        // 将内核缓冲区映射到用户空间
        buffer_infos[n_buffers].start = mmap(NULL, 
                                              buf.length,
                                              PROT_READ | PROT_WRITE, 
                                              MAP_SHARED, 
                                              v4l2_fd, 
                                              buf.m.offset);
        buffer_infos[n_buffers].length = buf.length;

        if (MAP_FAILED == buffer_infos[n_buffers].start) {
            perror("mmap error");
            return -1;
        }
        
        printf("buffer[%d] mapped at %p, length=%d\n", 
               n_buffers, buffer_infos[n_buffers].start, buf.length);
    }

    printf("memory map success\n");

    // 将缓冲区放入队列 - 修复：为每个缓冲区重新初始化buf
    struct v4l2_buffer qbuf;  // 使用新的变量，避免混淆
    for (int i = 0; i < FRAMEBUFFER_COUNT; i++) {
        memset(&qbuf, 0, sizeof(qbuf));
        qbuf.index = i;
        qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        
        if (ioctl(v4l2_fd, VIDIOC_QBUF, &qbuf) < 0) {
            perror("VIDIOC_QBUF failed");
            return -1;
        }
        printf("buffer[%d] queued\n", i);
    }

    printf("all buffers queued successfully\n");
    return 0;
}

int MainWindow::v4l2StreamOn()
{
    /* 初始化缓冲区 */
    if(v4l2_init_buffer()<0){
        printf("------------------------------------\n");
        return -1;
    }

    // 开始视频流
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (::ioctl(v4l2_fd, VIDIOC_STREAMON, &type) < 0) {
        qWarning("启动流失败");
        return -1;
    }

    printf("open stream success\n");
    return 0;
}

int MainWindow::v4l2GetOneFrame(FrameBuffer *framebuf){
    // 1. 使用select()进行I/O复用
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(v4l2_fd, &fds);

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    // 检查是否正在关闭
    if (is_closing || v4l2_fd < 0) {
        return -1;
    }
    
    int ret = select(v4l2_fd + 1, &fds, NULL, NULL, &tv);
    if (ret <= 0) {
        cout << "select timeout or error" << endl;
        return -1;
    }

    // 2. 先 DQBUF - 获取填充好的缓冲区
    struct v4l2_buffer one_buf;
    memset(&one_buf, 0, sizeof(one_buf));
    one_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    one_buf.memory = V4L2_MEMORY_MMAP;

    std::lock_guard<std::mutex> lock(v4l2_mutex);

    if (ioctl(v4l2_fd, VIDIOC_DQBUF, &one_buf) < 0) {
        cout << "VIDIOC_DQBUF failed: " << strerror(errno) << endl;
        return -1;
    }

    // 3. 现在 one_buf 才有效！可以访问 bytesused 和 index

    // 4. 检查并分配 framebuf->buf（现在 one_buf.bytesused 有效）
    if (framebuf->buf == NULL) {
        frame_buffer = std::unique_ptr<unsigned char[]>(
            new unsigned char[one_buf.bytesused]
        );
            
        if (!frame_buffer) {
            cout << "内存分配失败！" << endl;
            ioctl(v4l2_fd, VIDIOC_QBUF, &one_buf);
            return -1;
        }
        framebuf->buf = frame_buffer.get();
        framebuf->length = one_buf.bytesused;
        frame_buffer_size = one_buf.bytesused;
    }

    // 5. 从buffer_infos中拷贝数据到framebuf中
    memcpy(framebuf->buf, buffer_infos[one_buf.index].start, one_buf.bytesused);
    framebuf->length = one_buf.bytesused;

    // 6. QBUF - 重新入队
    if (ioctl(v4l2_fd, VIDIOC_QBUF, &one_buf) == -1) {
        return -1;
    }

    return 0;
}

void MainWindow::videoShow(){
    // 检查 V4L2 设备
    if (v4l2_fd < 0) {
        qDebug() << "V4L2 device not open";
        return;
    }

    if (!buffer_infos) {
        qDebug() << "buffer_infos is null";
        return;
    }
    
    FrameBuffer frame;
    memset(&frame, 0, sizeof(frame));  // 初始化
    
    // 获取一帧数据
    if (v4l2GetOneFrame(&frame) < 0) {
        qDebug() << "Failed to get frame from V4L2";
        return;
    }

    if (frame.length <= 0) {
        qDebug() << "Frame length is 0";
        return;
    }

    // 检查 frame 数据有效性
    if (!frame.buf) {
        qDebug() << "Frame buffer is null";
        return;
    }
    
    QPixmap pix;
    // 尝试加载 JPEG/MJPEG 数据
    if (!pix.loadFromData(frame.buf, frame.length)) {
        qDebug() << "Failed to load pixmap from data, length:" << frame.length;
        return;
    }
    
    if (pix.isNull()) {
        qDebug() << "Loaded pixmap is null";
        return;
    }
    
    // 将 QPixmap 转换为 QImage
    QImage img = pix.toImage();
    if (img.isNull()) {
        qDebug() << "Failed to convert pixmap to image";
        return;
    }
    
    // qDebug() << "Image format:" << img.format() 
    //          << "size:" << img.width() << "x" << img.height()
    //          << "depth:" << img.depth() << "bits";
    
    // 检查图像数据指针
    if (!img.bits()) {
        qDebug() << "Image bits is null";
        return;
    }
    
    // 创建 OpenCV Mat - 注意 QImage 可能是各种格式
    cv::Mat mat;
    
    // 根据 QImage 格式选择合适的转换
    switch (img.format()) {
        case QImage::Format_RGB888:
            // 直接使用，无需转换
            mat = cv::Mat(img.height(), img.width(), CV_8UC3, 
                         (void*)img.bits(), img.bytesPerLine()).clone();
            break;
            
        case QImage::Format_ARGB32:
        case QImage::Format_RGB32:
            // 需要转换 BGRA 到 BGR
            mat = cv::Mat(img.height(), img.width(), CV_8UC4, 
                         (void*)img.bits(), img.bytesPerLine()).clone();
            break;
            
        default:
            // 其他格式转换为 RGB888
            img = img.convertToFormat(QImage::Format_RGB888);
            if (img.isNull()) {
                qDebug() << "Failed to convert image to RGB888";
                return;
            }
            mat = cv::Mat(img.height(), img.width(), CV_8UC3, 
                         (void*)img.bits(), img.bytesPerLine()).clone();
            break;
    }
    
    if (mat.empty()) {
        qDebug() << "OpenCV mat is empty";
        return;
    }
    
    cv::Mat matBgr;
    
    // 根据原始通道数转换
    if (mat.channels() == 4) {
        cv::cvtColor(mat, matBgr, cv::COLOR_BGRA2BGR);
    } else if (mat.channels() == 3) {
        // 已经是 BGR 或 RGB？需要确认
        // 假设是 RGB，转换为 BGR
        cv::cvtColor(mat, matBgr, cv::COLOR_RGB2BGR);
    } else {
        qDebug() << "Unexpected channel count:" << mat.channels();
        return;
    }
    
    if (matBgr.empty()) {
        qDebug() << "matBgr is empty after conversion";
        return;
    }
    
    // 图像处理 - 添加空检查
    try {
        // 基于HSV的自适应曝光调整
        if (isAutoExposureEnabled && !matBgr.empty()) {
            cv::Mat hsv;
            cv::cvtColor(matBgr, hsv, cv::COLOR_BGR2HSV);
            
            if (!hsv.empty()) {
                std::vector<cv::Mat> hsvChannels;
                cv::split(hsv, hsvChannels);
                
                if (hsvChannels.size() == 3 && !hsvChannels[2].empty()) {
                    // 计算V通道直方图
                    int histSize = 256;
                    float range[] = {0, 256};
                    const float* histRange = {range};
                    cv::Mat hist;
                    
                    cv::calcHist(&hsvChannels[2], 1, 0, cv::Mat(), hist, 1, &histSize, &histRange, true, false);
                    
                    if (!hist.empty()) {
                        // 计算直方图中位数
                        int totalPixels = matBgr.rows * matBgr.cols;
                        float midtotalPixels = totalPixels * 0.5;
                        int sum = 0;
                        static int medianValue = 0;
                        static int frameCounter = 0;
                        const int processInterval = 10;
                        
                        if (frameCounter % processInterval == 0) {
                            for (int i = 0; i < histSize; ++i) {
                                sum += hist.at<float>(i);
                                if (sum > midtotalPixels) {
                                    medianValue = i;
                                    break;
                                }
                            }
                        }
                        frameCounter++;
                        
                        // 计算当前亮度均值
                        cv::Scalar meanScalar = cv::mean(hsvChannels[2]);
                        double currentMean = meanScalar[0];
                        int targetMean = 0;
                        
                        if (currentMean < 100) {
                            targetMean = std::max(medianValue + 30, 180);
                        } else if (currentMean > 200) {
                            targetMean = std::min(medianValue - 30, 120);
                        } else {
                            targetMean = medianValue;
                        }
                        
                        // 调整亮度
                        double exposureScale = targetMean / std::max(currentMean, 1.0);
                        hsvChannels[2].convertTo(hsvChannels[2], -1, exposureScale, 0);
                        
                        // 合并 HSV 通道并转回 BGR
                        cv::merge(hsvChannels, hsv);
                        cv::cvtColor(hsv, matBgr, cv::COLOR_HSV2BGR);
                    }
                }
            }
        }
        
        // 伽马校正
        if (isGammaCorrectionEnabled && !matBgr.empty()) {
            cv::Mat hsv;
            cv::cvtColor(matBgr, hsv, cv::COLOR_BGR2HSV);
            
            if (!hsv.empty()) {
                std::vector<cv::Mat> hsvChannels;
                cv::split(hsv, hsvChannels);
                
                if (hsvChannels.size() == 3 && !hsvChannels[2].empty()) {
                    double meanBrightness = cv::mean(hsvChannels[2])[0];
                    double gamma = 1.2;
                    
                    if (meanBrightness < 100) {
                        gamma = 1.5;
                    } else if (meanBrightness > 200) {
                        gamma = 0.8;
                    }
                    
                    cv::Mat lookUpTable(1, 256, CV_8U);
                    uchar* p = lookUpTable.ptr();
                    
                    for (int i = 0; i < 256; ++i) {
                        p[i] = cv::saturate_cast<uchar>(pow(i / 255.0, gamma) * 255.0);
                    }
                    
                    cv::Mat correctedMat;
                    cv::LUT(matBgr, lookUpTable, correctedMat);
                    
                    if (!correctedMat.empty()) {
                        matBgr = correctedMat;
                    }
                }
            }
        }
        
        // 录制
        if (isRecording && !matBgr.empty()) {
            frames.push_back(matBgr.clone());
        }
        
    } catch (const cv::Exception& e) {
        qDebug() << "OpenCV exception:" << e.what();
        return;
    } catch (const std::exception& e) {
        qDebug() << "Standard exception:" << e.what();
        return;
    }
    
    // 转换为 RGB 用于显示
    if (matBgr.empty()) {
        qDebug() << "matBgr is empty before display";
        return;
    }
    
    cv::Mat matRgb;
    cv::cvtColor(matBgr, matRgb, cv::COLOR_BGR2RGB);
    
    if (matRgb.empty()) {
        qDebug() << "matRgb is empty";
        return;
    }
    
    // 创建 QImage
    QImage processedImg(matRgb.data, matRgb.cols, matRgb.rows, 
                        matRgb.step, QImage::Format_RGB888);
    
    if (processedImg.isNull()) {
        qDebug() << "Processed image is null";
        return;
    }
    
    // 转换为 QPixmap 并显示
    pix = QPixmap::fromImage(processedImg);
    
    if (pix.isNull()) {
        qDebug() << "Final pixmap is null";
        return;
    }
    
    // 检查 UI 元素
    if (!ui || !ui->origal_label) {
        qDebug() << "UI or origal_label is null";
        return;
    }
    
    // 缩放并显示
    pix = pix.scaled(ui->origal_label->width(), 
                     ui->origal_label->height(), 
                     Qt::KeepAspectRatio, 
                     Qt::SmoothTransformation);
    
    ui->origal_label->setPixmap(pix);
    
    // qDebug() << "Frame displayed successfully";
}

/* 定时器启动 */
void MainWindow::timerStart(float fps){
    // 1000/33约等于30,根据帧率设置间隔
    timer->start(fps);
}

/* 开始按钮点击槽函数 */
void MainWindow::on_start_pushButton_clicked()
{
    timer = new QTimer(this);
    this->timerStart(1000 / g_fps);
    ui->start_pushButton->setEnabled(false);
    ui->close_pushButton->setEnabled(true);
    is_closing = false;  // 重置关闭标志
    //连接定时器信号槽
    connect(timer, &QTimer::timeout, this, &MainWindow::videoShow);
}

void MainWindow::on_close_pushButton_clicked() {
    cout << "Stopping video stream..." << endl;
    
    // 1. 设置关闭标志，阻止新的采集
    is_closing = true;
    
    // 2. 停止定时器
    timer->stop();
    
    // 3. 给正在执行的 v4l2GetOneFrame 一些时间完成
    // 发送一个中断信号给 select（可选）
    // 可以通过写入一个管道或使用 signal 来中断 select
    
    // 4. 等待当前正在执行的帧采集完成
    for (int i = 0; i < 10; i++) {  // 最多等待1秒
        if (v4l2_mutex.try_lock()) {
            // 成功获取锁，说明没有 v4l2GetOneFrame 在执行
            cout << "V4L2 operation completed, releasing resources..." << endl;
            
            // 释放 V4L2 资源
            if (v4l2_fd != -1) {
                // 先停止视频流
                int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                ioctl(v4l2_fd, VIDIOC_STREAMOFF, &type);
                
                // 然后释放缓冲区
                for (int i = 0; i < FRAMEBUFFER_COUNT; ++i) {
                    if (buffer_infos[i].start) {
                        munmap(buffer_infos[i].start, buffer_infos[i].length);
                        buffer_infos[i].start = nullptr;
                    }
                }
                
                // 最后关闭文件描述符
                ::close(v4l2_fd);
                v4l2_fd = -1;
            }
            
            v4l2_mutex.unlock();
            break;
        } else {
            // 无法获取锁，说明 v4l2GetOneFrame 正在执行
            cout << "Waiting for V4L2 operation to complete..." << endl;
            this->thread()->msleep(100);  // 等待100ms
        }
    }
    
    this->close();
}

// 恢复视频显示
void MainWindow::resumeVideoStream(float fps)
{
    //从origal_label 继续显示
    this->timerStart(fps);  // 恢复10ms间隔显示图像
}

void MainWindow::on_takePicture_pushButton_clicked()
{
    // 1. 获取当前显示的图像
    const QPixmap* pixmap = ui->origal_label->pixmap();
    if (!pixmap || pixmap->isNull()) {
        qWarning("没有可显示的图像");
        return;
    }

    // 暂停视频流
    if (timer && timer->isActive()) {
        timer->stop();
    }

    // 2. 深拷贝 QPixmap，避免后续修改
    QPixmap copyPixmap = *pixmap;
    
    // 3. 转换为 QImage（确保格式正确）
    QImage img = copyPixmap.toImage();
    if (img.isNull()) {
        qWarning("图像转换失败");
        resumeVideoStream(1000 / g_fps);
        return;
    }

    // 4. 转换为 OpenCV Mat（深拷贝，避免内存问题）
    cv::Mat matBgr;
    
    // 转换为适合 OpenCV 的格式
    QImage convertedImg = img.convertToFormat(QImage::Format_RGB32);
    
    cv::Mat mat(convertedImg.height(), convertedImg.width(), 
                CV_8UC4, (void*)convertedImg.bits(), convertedImg.bytesPerLine());
    
    // 转换为 BGR 格式（OpenCV 默认格式）
    cv::cvtColor(mat, matBgr, cv::COLOR_BGRA2BGR);
    
    // 5. 生成默认文件名
    QString defaultFileName = QString("picture_%1.jpg")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    
    // 6. 保存对话框
    QString fileName = QFileDialog::getSaveFileName(this, 
                                                     tr("保存图片"),
                                                     defaultFileName,
                                                     tr("JPEG Files (*.jpg);;PNG Files (*.png)"));
    
    if (!fileName.isEmpty()) {
        // 7. 保存图像
        std::string stdFileName = fileName.toStdString();
        
        // 根据文件扩展名决定保存参数
        std::vector<int> params;
        if (fileName.endsWith(".jpg", Qt::CaseInsensitive) || 
            fileName.endsWith(".jpeg", Qt::CaseInsensitive)) {
            params = {cv::IMWRITE_JPEG_QUALITY, 95};  // JPEG 质量 95%
        }
        
        if (cv::imwrite(stdFileName, matBgr, params)) {
            qDebug() << "图片已保存:" << fileName;
            
            // 可选：显示保存成功提示
            QMessageBox::information(this, tr("保存成功"), 
                                    tr("图片已保存到:\n%1").arg(fileName));
        } else {
            qWarning() << "保存图片失败:" << fileName;
            QMessageBox::warning(this, tr("保存失败"), 
                                tr("无法保存图片到:\n%1").arg(fileName));
        }
    }
    
    // 8. 恢复视频流
    resumeVideoStream(1000 / g_fps);
}

void MainWindow::on_playPicture_pushButton_clicked()
{
    // 暂停视频流
    timer->stop();

    // 1. 弹出文件选择对话框让用户选择图片
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开图片"), "", tr("JPEG Files (*.jpg);;PNG Files (*.png);;All Files (*)"));
    if (fileName.isEmpty()) {
        return; // 用户取消选择
    }

    // 2. 使用 OpenCV 加载图像
    cv::Mat img = cv::imread(fileName.toStdString()); // 使用 OpenCV 加载图像
    if (img.empty()) {
        qWarning("加载图像失败");
        return;
    }

    // 3. 将 OpenCV Mat 转换为 QImage
    // 将 BGR 格式的图片转换为 RGB 格式
    cv::Mat imgRgb;
    if (img.channels() == 3) {
        cv::cvtColor(img, imgRgb, cv::COLOR_BGR2RGB); // BGR 转 RGB
    } else {
        imgRgb = img; // 如果不是三通道则直接使用
    }

    // 将 OpenCV Mat 转换为 QImage
    QImage qImg((const uchar*)imgRgb.data, imgRgb.cols, imgRgb.rows, imgRgb.step, QImage::Format_RGB888);

    // 4. 显示图像到 QLabel
    if (qImg.isNull()) {
        qWarning("图像转换失败");
        return;
    }

    // 设置到对应的 QLabel
    ui->origal_label->setPixmap(QPixmap::fromImage(qImg));
}

void MainWindow::on_closePic_pushButton_clicked()
{
    resumeVideoStream(1000/g_fps);
}

void MainWindow::on_videoRecord_pushButton_clicked()
{
    if (isRecording) {
        // 停止录制
        isRecording = false;

        // 停止录制并保存文件
        QString defaultFileName = QString("video_%1.mp4").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

        // 弹出文件保存对话框让用户选择保存位置和文件名
        QString fileName = QFileDialog::getSaveFileName(this, tr("保存视频"), defaultFileName, tr("MP4 Files (*.mp4);;AVI Files (*.avi)"));
        if (fileName.isEmpty()) {
            return; // 用户取消保存
        }

        // 使用 VideoWriter 初始化并将帧写入文件
        videoWriter.open(fileName.toStdString(), cv::VideoWriter::fourcc('H', '2', '6', '4'), 1000/g_fps, frames[0].size()); // 30 FPS, H.264 编码

        if (!videoWriter.isOpened()) {
            qWarning("视频保存失败");
            return;
        }

        // 写入所有帧
        for (const auto& frame : frames) {
            videoWriter.write(frame);  // 写入视频帧
        }

        videoWriter.release();  // 释放 VideoWriter
        qDebug() << "视频已保存:" << fileName;

        // 清空帧容器
        frames.clear();

    } else {
        // 开始录制
        isRecording = true;

        // 清空之前的帧
        frames.clear();

        // 输出开始信息
        qDebug() << "开始录制..";
    }
}

// 播放视频按钮槽函数
void MainWindow::on_playVideo_pushButton_clicked()
{
    // 暂停当前视频流
    stopVideoStream();

    // 弹出文件选择对话框让用户选择视频文件
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开视频"), "", tr("MP4 Files (*.mp4);;AVI Files (*.avi);;All Files (*)"));
    if (fileName.isEmpty()) {
        return; // 用户取消选择
    }

    // 打开视频文件
    videoCapture.open(fileName.toStdString());
    if (!videoCapture.isOpened()) {
        qWarning("无法打开视频文件");
        return;
    }

    // 获取视频的总帧数和帧率
    int frameCount = static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_COUNT));
    double fps = videoCapture.get(cv::CAP_PROP_FPS);

    // 创建进度滑块
    videoSlider = new QSlider(Qt::Horizontal, this);
    videoSlider->setRange(0, frameCount);  // 设置滑块范围
    videoSlider->setValue(0);  // 初始值为0
    ui->video_verticalLayout->addWidget(videoSlider);  // 添加到垂直布局中
    // 连接滑块信号与槽函数
    connect(videoSlider, &QSlider::valueChanged, this, &MainWindow::on_sliderValueChanged);

    // 创建进度条
    videoProgressBar = new QProgressBar(this);
    videoProgressBar->setRange(0, frameCount);  // 设置进度条范围
    videoProgressBar->setValue(0);  // 初始值为0
    ui->video_verticalLayout->addWidget(videoProgressBar);  // 将进度条添加到布局中
    // 显示视频第一帧到 origal_label
    updateVideoFrame();

    // 启动定时器更新视频进度
    videoUpdateTimer = new QTimer(this);
    connect(videoUpdateTimer, &QTimer::timeout, this, &MainWindow::updateVideoProgress);
    videoUpdateTimer->start(1000 / fps);  // 根据视频帧率设置定时器间隔
}

// 更新视频进度条
void MainWindow::updateVideoProgress()
{
    if (videoCapture.isOpened()) {
        // 获取当前帧索引
        int currentFrameIndex = static_cast<int>(videoCapture.get(cv::CAP_PROP_POS_FRAMES));

        // 更新滑块和进度条的值
        videoSlider->setValue(currentFrameIndex);
        videoProgressBar->setValue(currentFrameIndex);

        // 读取并显示视频帧到 origal_label
        updateVideoFrame();
    }
}

// 更新视频帧显示
void MainWindow::updateVideoFrame()
{
    if (!videoCapture.isOpened()) {
        return;
    }

    // 获取当前帧索引
    int currentFrameIndex = static_cast<int>(videoCapture.get(cv::CAP_PROP_POS_FRAMES));
    int totalFrames = static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_COUNT));

    // 如果已经播放到最后，则保持最后一帧
    if (currentFrameIndex >= totalFrames) {
        videoCapture.set(cv::CAP_PROP_POS_FRAMES, totalFrames - 1);  // 设置到最后一帧
        videoCapture >> currentFrame;  // 读取最后一帧
    } else {
        // 获取视频的下一帧
        videoCapture >> currentFrame;
    }

    if (currentFrame.empty()) {
        qWarning("视频帧读取失败或视频结束");
        return;
    }

    // 将 OpenCV Mat 转换为 QImage 并显示到 origal_label
    QImage img = QImage(currentFrame.data, currentFrame.cols, currentFrame.rows, currentFrame.step, QImage::Format_RGB888);
    ui->origal_label->setPixmap(QPixmap::fromImage(img));  // 显示视频帧
}

// 滑块值改变时的槽函数
void MainWindow::on_sliderValueChanged(int value)
{
    if (videoCapture.isOpened()) {
        // 将视频跳转到指定帧位置
        videoCapture.set(cv::CAP_PROP_POS_FRAMES, value);
        updateVideoFrame();  // 更新显示帧
    }
}

void MainWindow::on_closeVideo_pushButton_clicked()
{
    // 停止视频播放和相关控件
    stopVideoPlayback();

    // 删除滑块
    if (videoSlider != nullptr) {
        ui->video_verticalLayout->removeWidget(videoSlider);  // 从布局中移除
        delete videoSlider;  // 删除滑块
        videoSlider = nullptr;  // 置空指针
    }

    resumeVideoStream(1000/g_fps);  // 恢复摄像头实时显示
}

// 停止视频播放
void MainWindow::stopVideoPlayback()
{
    if (videoCapture.isOpened()) {
        videoCapture.release();  // 释放视频捕获
    }

    if (videoUpdateTimer != nullptr) {
        videoUpdateTimer->stop();  // 停止定时器
        delete videoUpdateTimer;
        videoUpdateTimer = nullptr;
    }

    // 清除滑块
    if (videoSlider != nullptr) {
        videoSlider->setValue(0);
    }

    // 删除进度条
    if (videoProgressBar != nullptr) {
        ui->video_verticalLayout->removeWidget(videoProgressBar);  // 从布局中移除
        delete videoProgressBar;  // 删除进度条
        videoProgressBar = nullptr;  // 置空指针
    }
}

// 暂停当前视频流
void MainWindow::stopVideoStream()
{
    timer->stop();
    qDebug() << "暂停视频流";
}

// 灰度处理按钮槽函数
void MainWindow::on_playGray_pushButton_clicked() {
    if (isGrayProcessing) {
        // 停止灰度处理
        videoGrayTimer->stop();
        delete videoGrayTimer;
        videoGrayTimer = nullptr;
        isGrayProcessing = false;
        ui->gray_label->clear();  // 清空图像
        ui->gray_label->setText("灰度图像");  // 设置提示文字

        ui->playGray_pushButton->setText("开始灰度处理");
    } else {
        // 开始灰度处理
        videoGrayTimer = new QTimer(this);
        connect(videoGrayTimer, &QTimer::timeout, this, &MainWindow::startGrayProcessing);
        videoGrayTimer->start(1000 / g_fps);  // 根据帧率设置定时
        isGrayProcessing = true;
        ui->playGray_pushButton->setText("停止灰度处理");
    }
}

// 启动灰度处理任务
void MainWindow::startGrayProcessing() {
    if (!isGrayProcessing) {
        // 如果不在处理状态则直接返回
        return;
    }

    // 从 QLabel 获取当前图像
    const QPixmap* gray_origPixmap = ui->origal_label->pixmap();
    if (!gray_origPixmap || gray_origPixmap->isNull()) {
        QMessageBox::warning(this, "提示", "没有可用的图像进行处理");
        return;
    }

    // 转换 QPixmap 为 QImage
    QImage inputImage = gray_origPixmap->toImage();

    // 创建灰度处理任务
    GrayTask* task = new GrayTask(inputImage);
    connect(task, &GrayTask::grayImageReady, this, &MainWindow::processGrayImage);

    // 将任务加入线程池
    QThreadPool::globalInstance()->start(task);
}

// 处理灰度图像并显示到 QLabel
void MainWindow::processGrayImage(const QImage& grayImage) {
    if (!ui->gray_label) return;

    static QSize labelGraySize;
    // 调整图像大小并显示
    static bool isFirstFrame = true;
    if (isFirstFrame) {
        QPixmap pixmap = QPixmap::fromImage(grayImage);
        ui->gray_label->setPixmap(pixmap);
        ui->gray_label->adjustSize();  // 自动调整 QLabel 的大小
        labelGraySize = ui->gray_label->size();
        isFirstFrame = false;
    } else {
        labelGraySize = ui->gray_label->size();
        // 缩放图像以适应 QLabel 大小
        QPixmap scaledGrayPixmap = QPixmap::fromImage(grayImage).scaled(labelGraySize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->gray_label->setPixmap(scaledGrayPixmap);
    }
}

// 二值化按钮槽函数
void MainWindow::on_playBinary_pushButton_clicked() {
    if (isBinaryProcessing) {
        // 停止二值化
        binaryTimer->stop();
        delete binaryTimer;
        binaryTimer = nullptr;
        isBinaryProcessing = false;

        // 清空 binar_label 的显示
        ui->binar_label->clear();
        ui->binar_label->setText("二值图像");

        ui->playBinary_pushButton->setText("开始二值化");
    } else {
        // 开始二值化
        binaryTimer = new QTimer(this);
        connect(binaryTimer, &QTimer::timeout, this, &MainWindow::startBinaryProcessing);

        // 设置定时器间隔根据当前帧率动态调整
        binaryTimer->start(1000 / g_fps);
        isBinaryProcessing = true;
        ui->playBinary_pushButton->setText("停止二值化");
    }
}

// 启动二值化处理任务
void MainWindow::startBinaryProcessing() {
    // 从origal_label 获取当前图像
    const QPixmap* binary_origPixmap = ui->origal_label->pixmap();
    if (!binary_origPixmap || binary_origPixmap->isNull()) {
        QMessageBox::warning(this, "提示", "没有可用的图像进行处理");
        return;
    }
    // 转换 QPixmap 为 QImage
    QImage inputImage = binary_origPixmap->toImage();

    // 创建 BinaryTask 任务
    BinaryTask* task = new BinaryTask(inputImage);

    // 连接信号槽接收处理结果
    connect(task, &BinaryTask::binaryImageReady, this, &MainWindow::processBinaryImage);

    // 将任务加入线程池
    QThreadPool::globalInstance()->start(task);
}

// 接收二值化图像并显示到 binar_label
void MainWindow::processBinaryImage(const QImage& binaryImage) {
    if (!ui->binar_label)
    {
        qDebug() << "找不到二值图像标签";
        return;
    }
    static QSize labelBinarySize;       // 存储二值图像 QLabel 的大小
    static bool isFirstBinay = true;

    if(isFirstBinay)
    {
        QPixmap binaryPixmap = QPixmap::fromImage(binaryImage);
        ui->binar_label->setPixmap(binaryPixmap);
        ui->binar_label->adjustSize();  // 自动调整 QLabel 的大小
        labelBinarySize = ui->gray_label->size();
        isFirstBinay = false;
    }
    else{
        // 获取 QLabel 的当前大小并缩放图像
        QSize labelBinarySize = ui->binar_label->size();
        QPixmap scaledBinaryPixmap = QPixmap::fromImage(binaryImage).scaled(labelBinarySize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // 更新 QLabel 显示
        ui->binar_label->setPixmap(scaledBinaryPixmap);
    }
}

void MainWindow::on_playEdge_pushButton_clicked()
{
    if (isEdgeProcessing) {
        // 停止边缘检测
        edgeTimer->stop();
        delete edgeTimer;
        edgeTimer = nullptr;
        isEdgeProcessing = false;

        // 清空 edge_label 的显示
        ui->edge_label->clear();
        ui->edge_label->setText("边缘图像");

        ui->playEdge_pushButton->setText("开始边缘检测");
    } else {
        // 开始边缘检测
        edgeTimer = new QTimer(this);
        connect(edgeTimer, &QTimer::timeout, this, &MainWindow::startEdgeProcessing);
        edgeTimer->start(1000 / g_fps);
        isEdgeProcessing = true;

        ui->playEdge_pushButton->setText("停止边缘检测");
    }
}

void MainWindow::startEdgeProcessing() {
    // 获取当前图像
    const QPixmap* origEdgePixmap = ui->origal_label->pixmap();
    if (!origEdgePixmap || origEdgePixmap->isNull()) {
        qWarning() << "No image found in origal_label!";
        return;
    }

    // 转换 QPixmap 为 QImage
    QImage inputEdgeImage = origEdgePixmap->toImage();

    // 创建边缘检测任务
    EdgeDetection* task = new EdgeDetection(inputEdgeImage);
    connect(task, &EdgeDetection::edgeImageReady, this, &MainWindow::processEdgeImage);

    // 将任务加入线程池
    QThreadPool::globalInstance()->start(task);
}

void MainWindow::processEdgeImage(const QImage& edgeImage) {
    if (!ui->edge_label) return;

    static QSize labelEdgeSize;
    static bool isFirstEdge = true;

    if(isFirstEdge)
    {
        QPixmap edgePixmap = QPixmap::fromImage(edgeImage);
        ui->edge_label->setPixmap(edgePixmap);
        ui->edge_label->adjustSize();
        labelEdgeSize = ui->edge_label->size();
        isFirstEdge = false;
    }
    else
    {
        QPixmap scaleEdgePixmap = QPixmap::fromImage(edgeImage).scaled(labelEdgeSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->edge_label->setPixmap(scaleEdgePixmap);
    }
}

void MainWindow::on_adjustFrame_horizontalSlider_valueChanged(int value)
{
    g_fps = value; // 更新全局帧率
    ui->frame_lineEdit->setText(QString::number(g_fps, 'f', 2)); // 显示到文本框

    qDebug() << "Frame rate adjusted to:" << g_fps;
}

void MainWindow::on_autoExp_pushButton_clicked()
{
    isAutoExposureEnabled = !isAutoExposureEnabled;

    if (isAutoExposureEnabled) {
        ui->autoExp_pushButton->setText("关闭");
    } else {
        ui->autoExp_pushButton->setText("开启");
    }
}

void MainWindow::on_autoGain_pushButton_clicked()
{
    isAutoGainEnabled = !isAutoGainEnabled;  // 切换状态
    // 更新按钮文字
    if (isAutoGainEnabled) {
        ui->autoGain_pushButton->setText("关闭");
    } else {
        ui->autoGain_pushButton->setText("开启");
    }
}

void MainWindow::on_gammaCor_pushButton_clicked()
{
    isGammaCorrectionEnabled = !isGammaCorrectionEnabled;  // 切换伽马校正状态
    // 更新按钮文字
    if (isGammaCorrectionEnabled) {
        ui->gammaCor_pushButton->setText("关闭伽马校正");
    } else {
        ui->gammaCor_pushButton->setText("开启伽马校正");
    }
}

void MainWindow::on_uploadServer_pushButton_clicked() {
    if (!isConnectServer) {
        // 初始化UDP连接
        udpSocket = new QUdpSocket();
        isConnectServer = true;
        ui->uploadServer_pushButton->setText("断开连接");
        qDebug() << "服务器连接成功";

        // 启动定时器处理图像
        if (!ProcessImageTimer) {  // 如果定时器不存在则创建
            ProcessImageTimer = new QTimer();
            connect(ProcessImageTimer, &QTimer::timeout, this, &MainWindow::startProcessImage);
            ProcessImageTimer->start(g_fps*10);
        }
    } else {
        // 断开连接
        isConnectServer = false;
        ui->uploadServer_pushButton->setText("连接服务器");
        qDebug() << "断开服务器连接";
    }
}

void MainWindow::startProcessImage() {
    if (isConnectServer) {
        // 从`origal_label`获取图像
        QLabel *origalLabel = ui->origal_label;
        QImage image = origalLabel->pixmap()->toImage();
        // 调整图像大小为640x480
        resizedImageProcessImage = image.scaled(640, 480, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // 创建图像处理任务
        ProcessImage *ProcessImageTask = new ProcessImage(resizedImageProcessImage, g_fps);
        connect(ProcessImageTask, &ProcessImage::dataReadyToUpload, this, &MainWindow::uploadDataToServer);

        threadPool->start(ProcessImageTask);
    } else {
        // 停止定时器
        if (ProcessImageTimer) {
            ProcessImageTimer->stop();
            delete ProcessImageTimer;
            ProcessImageTimer = nullptr;
        }
    }
}

// 发送RTP数据
void MainWindow::uploadDataToServer(const QByteArray &data) {
    qDebug() << "Uploading data to server, size:" << data.size();

    // 通过UDP发送RTP数据
    if (udpSocket->writeDatagram(data, QHostAddress("192.168.5.11"), 20000) == -1) {
        qDebug() << "Error sending RTP packet:" << udpSocket->errorString();
    } else {
        qDebug() << "RTP packet sent successfully!";
    }
}