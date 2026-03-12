#ifndef V4L2_H
#define V4L2_H

#include <QMainWindow>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include <poll.h>
#include <linux/fb.h>
#include <stdlib.h>
#include <string.h>

#include <QTimer>

#include <showphoto.h>

#define		video_width		640
#define 	video_height	480

QT_BEGIN_NAMESPACE
namespace Ui { class v4l2; }
QT_END_NAMESPACE

class v4l2 : public QMainWindow
{
    Q_OBJECT

public:
    v4l2(QWidget *parent = nullptr);
    ~v4l2();

private slots:
    void on_pushButton_open_clicked();
    void on_pushButton_take_clicked();

    void on_pushButton_photos_clicked();

private:
    Ui::v4l2 *ui;

    char *userbuff[4];        /* 存放摄像头设备内核缓冲区映射后的用户内存地址 */
    int userbuff_length[4];   /* 保存映射后的数据长度，释放缓存时要用 */

    int show_flag = 1;    /* 1:保存到本地   2:显示在lcd上 */
    int video_fd;
    int lcd_fd;
    int lcd_xres, lcd_yres;
    int lcd_realvirtual;

    int start = 0;

    QTimer *timer;

//    showphoto s;

    int v4l2_open();   //初始化相机参数
    int v4l2_close();

public slots:
    void video_show();

};
#endif // V4L2_H
