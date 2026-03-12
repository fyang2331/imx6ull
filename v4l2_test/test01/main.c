#include <stdio.h>
#include <stdlib.h>
#include <string.h>      // 添加这个！解决 memset 警告
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <pthread.h>


void *thread_bight(void *args)
{
    int fd = (int)args;
    unsigned char opt;
    int brightness = 0;
    int data = 0;

    struct v4l2_queryctrl queryctrl;
    queryctrl.id = V4L2_CID_BRIGHTNESS;
    if (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl))
    {        
        if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
        {            
            printf("Brightness control is disabled\n");
            return NULL;
        }
    }

    /* 打印亮度控制可以设置的最大值和最小值 */
    printf("Brightness control range: %d - %d\n", queryctrl.minimum, queryctrl.maximum);
    data = (queryctrl.maximum - queryctrl.minimum) / 10;

    while (1)
    {
        opt = getchar();  // getchar 在 raw 模式下立即返回
        struct v4l2_control control;
        control.id = V4L2_CID_BRIGHTNESS;

        /* 获取当前摄像头亮度 */
        if (0 == ioctl(fd, VIDIOC_G_CTRL, &control))
        {
            brightness = control.value;
            printf("Current brightness: %d\n", brightness);
        }

        if(opt == 'u' || opt == 'U')
        {
            brightness += data;

        }
        else if(opt == 'd' || opt == 'D')
        {
            brightness -= data;
        }

        if (brightness > queryctrl.maximum)
        {
            brightness = queryctrl.maximum;
        }
        else if (brightness < queryctrl.minimum)
        {
            brightness = queryctrl.minimum;
        }
        
        control.value = brightness;
        if (0 == ioctl(fd, VIDIOC_S_CTRL, &control))
        {
            printf("Brightness add/down to value: %d\n", control.value);
        }
        else
        {
            printf("Failed to reset brightness\n");
        }
    }
    
}

int main(int argc, char *argv[])
{
    int fd = 0;
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_frmsizeenum fsenum;
    int fmt_index = 0;
    int frame_index = 0;
    int i = 0;
    void *buffers[32];
    int count = 0;
    struct pollfd fds[1];
    char file_name[32];
    int file_cnt = 0;
    pthread_t thread_id;

    if (argc != 2)
    {
        printf("Usage: %s <device>\n", argv[0]);

        return -1;
    }

    fd = open(argv[1], O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open device");
        return -1;
    }
    
    while (1)
    {
        memset(&fmtdesc, 0, sizeof(fmtdesc));  // 添加：清零结构体
        fmtdesc.index = fmt_index;
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) < 0)
        {
            // perror("Failed to enumerate formats");  // 注释掉这行，因为到最后没有格式时也会出错
            break;
        }

        frame_index = 0;
        while (1)
        {
            memset(&fsenum, 0, sizeof(fsenum));  // 添加：清零结构体
            fsenum.pixel_format = fmtdesc.pixelformat;
            fsenum.index = frame_index;

            if (0 == ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsenum))
            {
                printf("Format: %s %d, Frame Size: %d x %d\n", 
                    fmtdesc.description, fmtdesc.pixelformat, fsenum.discrete.width, fsenum.discrete.height);
                frame_index++;  // 移动到下一行
            }
            else
            {
                break;
            }
        }
        fmt_index ++;
    }

    /* 3.获取摄像头的能力 (VIDIOC_QUERYCAP：是否支持视频采集、内存映射等) */
    struct v4l2_capability capability;
    memset(&capability, 0, sizeof(capability));  // 添加：清零结构体
    if(0 == ioctl(fd, VIDIOC_QUERYCAP, &capability)){
        if((capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0){
            perror("该摄像头设备不支持视频采集！");
            return -1;
        }
        if((capability.capabilities & V4L2_CAP_STREAMING) == 0){
            perror("该摄像头设备不支持视频采集！");
            return -2;
        }
        if((capability.capabilities & V4L2_MEMORY_MMAP) == 0){
            perror("该摄像头设备不支持mmap内存映射！");
            return -3;
        }
    }
    else
    {
        printf("Failed to query device capabilities\n");
    }

    pthread_create(&thread_id, NULL, thread_bight, (void *)fd);

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(struct v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 1024;
    fmt.fmt.pix.height = 768;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    if(0 == ioctl(fd, VIDIOC_S_FMT, &fmt))
    {
        printf("Set format successfully: %d x %d, pixelformat: %d\n", fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.pixelformat);
    }
    else
    {
        printf("Failed to set format\n");
    }

    /*  申请buffer */
    struct v4l2_requestbuffers reqbuf;
    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.count = 32;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;

    if(0 == ioctl(fd, VIDIOC_REQBUFS, &reqbuf))
    {
        printf("Request buffers successfully: %d\n", reqbuf.count);
        /* 申请成功之后映射这些buffer */

        for (i = 0; i < reqbuf.count; i ++)
        {
            struct v4l2_buffer buf;
            memset(&buf, 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.index = i;
            buf.memory = V4L2_MEMORY_MMAP;

            count = reqbuf.count;
            if (0 == ioctl(fd, VIDIOC_QUERYBUF, &buf))
            {
                buffers[i] = mmap(NULL, 
                                buf.length, 
                                PROT_READ | PROT_WRITE, MAP_SHARED, 
                                fd, 
                                buf.m.offset);
                if(buffers[i] == MAP_FAILED)
                {
                    printf("Failed to mmap buffer %d\n", i);
                    buffers[i] = NULL;
                }
            }
            else
            {
                printf("Failed to query buffer %d\n", i);
            }
        }
         /* 映射成功*/
        printf("Mmap %d buffer success!\n", count);
    }
    else
    {
        printf("Failed to request buffers\n");
    }

    /* 把这些buffer放入空闲链表 */
    for (i = 0; i < count; i ++)
    {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.index = i;
        buf.memory = V4L2_MEMORY_MMAP;

        if(0 == ioctl(fd, VIDIOC_QBUF, &buf))
        {
            printf("Queue buffer %d successfully\n", i);
        }
        else
        {
            printf("Failed to queue buffer %d\n", i);
        }
    }

    printf("Queue buffer successfully\n");

    /* 启动摄像头 */
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(0 == ioctl(fd, VIDIOC_STREAMON, &type))
    {
        printf("Start streaming successfully\n");
    }
    else
    {
        printf("Failed to start streaming\n");
    }

    while (1)
    {
        /* poll */
        memset(fds, 0, sizeof(fds));  // 添加：清零结构体
        fds[0].fd = fd;
        fds[0].events = POLLIN;
        int ret = poll(fds, 1, -1);
        if (ret > 0)
        {
            if (fds[0].revents & POLLIN)
            {
                struct v4l2_buffer buf;
                memset(&buf, 0, sizeof(buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;

                if (0 == ioctl(fd, VIDIOC_DQBUF, &buf))
                {
                    // printf("Dequeue buffer %d successfully\n", buf.index);
                    /* 取出数据*/

                    /* 把buffer里面的数据存为文件 */
                    snprintf(file_name, sizeof(file_name), "frame_%d.jpg", file_cnt);
                    file_cnt++;
                    int fd_out = open(file_name, O_RDWR | O_CREAT, 0666);
                    if (fd_out < 0)
                    {
                        printf("Failed to open output file\n");
                    }
                    else
                    {
                        write(fd_out, buffers[buf.index], buf.bytesused);
                        close(fd_out);
                        fd_out = -1;
                        // printf("Saved frame to %s\n", file_name);
                    }

                    /* 把buffer放入队列 */
                    if(0 == ioctl(fd, VIDIOC_QBUF, &buf))
                    {
                        // printf("Queue buffer %d successfully\n", buf.index);
                    }
                    else
                    {
                        // printf("Failed to queue buffer %d\n", buf.index);
                    }
                }
                else
                {
                    printf("Failed to dequeue buffer\n");
                }
            }
        }
    }
    

    close(fd);  // 添加：关闭文件
    fd = -1;  // 添加：重置文件描述符
    return 0;
}