#include "v4l2.h"
#include "ui_v4l2.h"
#include <QtGui/QScreen>
#include <QDateTime>
#include <QRandomGenerator>
#include <QDebug>

v4l2::v4l2(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::v4l2)
{
    printf("v4l2 ui运行\n");
    ui->setupUi(this);

    //每隔固定的时间显示一帧
    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(video_show()));

    if(0 == v4l2_open()){
        printf("打开相机成功!\n");
        // 由于摄像头默认30帧每秒,虽然10ms定时执行一次,但实际上1秒内最多有30次可以执行成功
        // 其余都会在ioctl处阻塞
        timer->start(10);
        start = 1;
        ui->pushButton_open->setText("关闭");
    }

    // 获取当前目录
    QString currentDir = QDir::currentPath();
    // 拼接photo文件夹路径
    QString photoDirPath = currentDir + "/photo";
    // 创建QDir对象
    QDir photoDir(photoDirPath);
    // 判断photo文件夹是否存在
    if (!photoDir.exists()){
        // 创建photo文件夹
        if (photoDir.mkdir(photoDirPath))
            printf("创建 photo 文件夹成功\n");
        else
            printf("创建 photo 文件夹失败\n");
    }
    else
        printf("photo 文件夹已存在\n");
}

v4l2::~v4l2()
{
    delete ui;
}

void v4l2::video_show()
{
    QPixmap pix;

    /* 采集图片数据 */
    //定义结构体变量，用于获取内核队列数据
    struct v4l2_buffer buffer;
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    /* 从内核中捕获好的输出队列中取出一个 */
    if(0 == ioctl(video_fd, VIDIOC_DQBUF, &buffer)){
        /* 显示在label控件上 */
        //获取一帧显示
        pix.loadFromData((unsigned char *)userbuff[buffer.index], buffer.bytesused);
        pix.scaled(ui->label->width(),ui->label->height(),Qt::KeepAspectRatio);
        ui->label->setPixmap(pix);
    }
    /* 将使用后的缓冲区放回到内核的输入队列中 (VIDIOC_QBUF) */
    if(0 > ioctl(video_fd, VIDIOC_QBUF, &buffer)){
        perror("返回队列失败！");
    }
}

//打开 & 设置相机
int v4l2::v4l2_open()
{
    /* 1.打开摄像头设备 /dev/video0 */
    video_fd = open("/dev/video2", O_RDWR);
    if(video_fd < 0){
        perror("打开摄像头设备失败");
        return -1;
    }
    /* 3.获取摄像头的能力 (VIDIOC_QUERYCAP：是否支持视频采集、内存映射等) */
    struct v4l2_capability capability;
    if(0 == ioctl(video_fd, VIDIOC_QUERYCAP, &capability)){
        if((capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0){
            perror("该摄像头设备不支持视频采集！");
            ::close(video_fd);
            return -2;
        }
        if((capability.capabilities & V4L2_MEMORY_MMAP) == 0){
            perror("该摄像头设备不支持mmap内存映射！");
            ::close(video_fd);
            return -3;
        }
    }

    /* 4.枚举摄像头支持的格式           (VIDIOC_ENUM_FMT：MJPG、YUYV等)
      列举出每种格式下支持的分辨率      (VIDIOC_ENUM_FRAMESIZES) */
    struct v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc, 0, sizeof(fmtdesc));
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  //设置视频采集设备类型
    int i = 0;
    while(1){
        fmtdesc.index = i++;
        // 获取支持格式
        if(0 == ioctl(video_fd, VIDIOC_ENUM_FMT, &fmtdesc)){
            printf("支持格式：%s, %c%c%c%c\n", fmtdesc.description,
                                            fmtdesc.pixelformat & 0xff,
                                            fmtdesc.pixelformat >> 8 & 0xff,
                                            fmtdesc.pixelformat >> 16 & 0xff,
                                            fmtdesc.pixelformat >> 24 & 0xff);
            // 列出该格式下支持的分辨率             VIDIOC_ENUM_FRAMESIZES & 默认帧率 VIDIOC_G_PARM
            // 1.默认帧率
            struct v4l2_streamparm streamparm;
            streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if(0 == ioctl(video_fd, VIDIOC_G_PARM, &streamparm))
                printf("该格式默认帧率 %d fps\n", streamparm.parm.capture.timeperframe.denominator);
            // 2.循环列出支持的分辨率
            struct v4l2_frmsizeenum frmsizeenum;
            frmsizeenum.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            frmsizeenum.pixel_format = fmtdesc.pixelformat;   //设置成对应的格式
            int j = 0;
            printf("支持的分辨率有：\n");
            while(1){
                frmsizeenum.index = j++;
                if(0 == ioctl(video_fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum))
                    printf("%d x %d\n", frmsizeenum.discrete.width, frmsizeenum.discrete.height);
                else break;
            }
            printf("\n");
        }else break;
    }

    /* 5.设置摄像头类型为捕获、设置分辨率、视频采集格式 (VIDIOC_S_FMT) */
    struct v4l2_format format;
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   /* 视频采集 */
    format.fmt.pix.width = video_width;          /* 宽 */
    format.fmt.pix.height = video_height;    	 /* 高 */
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;   /* 设置输出类型：MJPG */
    if(0 > ioctl(video_fd, VIDIOC_S_FMT, &format)){
        perror("设置摄像头参数失败！");
        ::close(video_fd);
        return -4;
    }

    /* 6.向内核申请内存 (VIDIOC_REQBUFS：个数、映射方式为mmap)
         将申请到的缓存加入内核队列 (VIDIOC_QBUF)
         将内核内存映射到用户空间 (mmap) */
    struct v4l2_requestbuffers requestbuffers;
    requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestbuffers.count = 4;    //申请缓存个数
    requestbuffers.memory = V4L2_MEMORY_MMAP;     //申请为物理连续的内存空间
    if(0 == ioctl(video_fd, VIDIOC_REQBUFS, &requestbuffers)){
        /* 申请到内存后 */
        for(int i = 0; i < requestbuffers.count; i++){
            /* 将申请到的缓存加入内核队列 (VIDIOC_QBUF)              */
            struct v4l2_buffer buffer;
            buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buffer.index = i;
            buffer.memory = V4L2_MEMORY_MMAP;
            if(0 == ioctl(video_fd, VIDIOC_QBUF, &buffer)){
                /* 加入内核队列成功后，将内存映射到用户空间 (mmap) */
                userbuff[i] = (char *)mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, buffer.m.offset);
                userbuff_length[i] = buffer.length;
            }
        }
    }
    else{
        perror("申请内存失败！");
        ::close(video_fd);
        return -5;
    }
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(0 > ioctl(video_fd, VIDIOC_STREAMON, &type)){
        perror("打开视频流失败！");
        return -6;
    }
    return 0;
}

int v4l2::v4l2_close()
{
    /* 8.停止采集，关闭视频流 (VIDIOC_STREAMOFF)
      关闭摄像头设备 & 关闭LCD设备 */
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(0 == ioctl(video_fd, VIDIOC_STREAMOFF, &type)){
        /* 9.释放映射 */
        for(int i = 0; i < 4; i++)
            munmap(userbuff[i], userbuff_length[i]);
        ::close(video_fd);
        printf("关闭相机成功!\n");
        return 0;
    }
    return -1;
}

/* 控制相机打开和关闭 */
void v4l2::on_pushButton_open_clicked()
{
    if(start == 0){
        // 使用v4l2打开 & 设置相机成功
        if(0 == v4l2_open()){
            printf("打开相机成功!\n");
            // 由于摄像头默认30帧每秒,虽然10ms定时执行一次,但实际上1秒内最多有30次可以执行成功
            // 其余都会在ioctl处阻塞
            timer->start(10);
            start = 1;
            ui->pushButton_open->setText("关闭");
        }
    }else{
        if(0 == v4l2_close()){
            start = 0;
            timer->stop();
            ui->pushButton_open->setText("打开");
        }
    }
}

/* 拍照 */
void v4l2::on_pushButton_take_clicked()
{
    //随机产生10个数字,作为照片的名字
    QString randomNumbers;
    for(int i=0; i<10; i++) {
        int randomNumber = QRandomGenerator::global()->bounded(10);
        randomNumbers.append(QString::number(randomNumber));
    }
    QString str = "./photo/photo_" + randomNumbers + ".jpg";

    /* 采集图片数据 */
    //定义结构体变量，用于获取内核队列数据
    struct v4l2_buffer buffer;
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    /* 从内核中捕获好的输出队列中取出一个 */
    if(0 == ioctl(video_fd, VIDIOC_DQBUF, &buffer)){
        //保存到本地
        int fd = open(str.toStdString().data(), O_RDWR | O_CREAT, 0777);   //打开并创建一个新文件
        write(fd, userbuff[buffer.index], buffer.bytesused);
        printf("%s\n", str.toStdString().data());
        ::close(fd);
    }
    /* 将使用后的缓冲区放回到内核的输入队列中 (VIDIOC_QBUF) */
    if(0 > ioctl(video_fd, VIDIOC_QBUF, &buffer)){
        perror("返回队列失败！");
    }

    //清空label,相当于拍照效果提示
    ui->label->clear();
}

/* 跳转到 showphoto ui界面 */
void v4l2::on_pushButton_photos_clicked()
{
    if(0 == v4l2_close()){
        timer->stop();
    }
    this->close();
    showphoto *s = new showphoto();
    s->show();
}
