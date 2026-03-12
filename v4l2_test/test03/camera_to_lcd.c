#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <linux/fb.h>
#include <linux/videodev2.h>

// 根据 LCD 参数定义的 RGB565 转换宏
#define LCD_RGB565(r, g, b) ((((r >> 3) & 0x1F) << 11) | \
                              (((g >> 2) & 0x3F) << 5)  | \
                              ((b >> 3) & 0x1F))

// 黑色
#define BLACK LCD_RGB565(0, 0, 0)

// YUYV 转 RGB565（指定输出位置）
void yuyv_to_lcd_rgb565(unsigned char *yuyv, unsigned short *fb, 
                         int cam_w, int cam_h, 
                         int lcd_w, int lcd_h,
                         int start_x, int start_y)
{
    int i, j;
    int y0, u, y1, v;
    int r, g, b;
    unsigned short *fb_ptr;
    unsigned char *yuyv_ptr;
    
    for (i = 0; i < cam_h; i++) {
        // 计算在 LCD 上的起始位置
        fb_ptr = fb + (start_y + i) * lcd_w + start_x;
        yuyv_ptr = yuyv + i * cam_w * 2;  // YUYV 每像素2字节
        
        for (j = 0; j < cam_w / 2; j++) {
            // 每个宏像素包含2个Y和1个U、1个V
            y0 = yuyv_ptr[0];
            u  = yuyv_ptr[1] - 128;
            y1 = yuyv_ptr[2];
            v  = yuyv_ptr[3] - 128;
            
            // 第一个像素 (Y0)
            r = y0 + ((359 * v) >> 8);
            g = y0 - ((88 * u) >> 8) - ((183 * v) >> 8);
            b = y0 + ((454 * u) >> 8);
            
            if (r < 0) r = 0; if (r > 255) r = 255;
            if (g < 0) g = 0; if (g > 255) g = 255;
            if (b < 0) b = 0; if (b > 255) b = 255;
            
            fb_ptr[0] = LCD_RGB565(r, g, b);
            
            // 第二个像素 (Y1)
            r = y1 + ((359 * v) >> 8);
            g = y1 - ((88 * u) >> 8) - ((183 * v) >> 8);
            b = y1 + ((454 * u) >> 8);
            
            if (r < 0) r = 0; if (r > 255) r = 255;
            if (g < 0) g = 0; if (g > 255) g = 255;
            if (b < 0) b = 0; if (b > 255) b = 255;
            
            fb_ptr[1] = LCD_RGB565(r, g, b);
            
            fb_ptr += 2;
            yuyv_ptr += 4;
        }
    }
}

// 清屏函数（填充黑色）
void clear_screen(unsigned short *fb, int width, int height)
{
    int i;
    for (i = 0; i < width * height; i++) {
        fb[i] = BLACK;
    }
}

int main(int argc, char *argv[])
{
    int camera_fd = 0;
    int lcd_fd = 0;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers reqbuf;
    struct v4l2_buffer buf;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    unsigned short *fb_mem = NULL;
    void *camera_buffers[4];
    int count = 0;
    int i;
    struct pollfd fds[1];
    int start_x, start_y;
    int cam_width, cam_height;
    
    if (argc != 3) {
        printf("Usage: %s <camera_device> <width>x<height>\n", argv[0]);
        printf("Example: %s /dev/video2 640x480\n", argv[0]);
        printf("Example: %s /dev/video2 1024x600 (全屏)\n", argv[0]);
        return -1;
    }
    
    // 解析摄像头分辨率参数
    if (sscanf(argv[2], "%dx%d", &cam_width, &cam_height) != 2) {
        printf("Invalid resolution format. Use WIDTHxHEIGHT\n");
        return -1;
    }
    
    // 1. 打开 LCD
    lcd_fd = open("/dev/fb0", O_RDWR);
    if (lcd_fd < 0) {
        perror("Failed to open LCD");
        return -1;
    }
    
    // 获取 LCD 信息
    if (ioctl(lcd_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("Failed to get LCD info");
        close(lcd_fd);
        return -1;
    }
    
    printf("\n=== LCD 屏幕信息 ===\n");
    printf("分辨率: %d x %d\n", vinfo.xres, vinfo.yres);
    printf("色深: %d bpp\n", vinfo.bits_per_pixel);
    printf("摄像头分辨率: %d x %d\n", cam_width, cam_height);
    
    // 计算居中位置
    start_x = (vinfo.xres - cam_width) / 2;
    start_y = (vinfo.yres - cam_height) / 2;
    
    // 检查是否超出屏幕
    if (start_x < 0 || start_y < 0) {
        printf("Warning: Camera resolution larger than LCD, will be cropped\n");
        if (start_x < 0) {
            cam_width = vinfo.xres;
            start_x = 0;
        }
        if (start_y < 0) {
            cam_height = vinfo.yres;
            start_y = 0;
        }
    }
    
    printf("显示位置: 左上角 (%d, %d)\n", start_x, start_y);
    printf("有效显示区域: %d x %d\n", cam_width, cam_height);
    printf("黑边: 左右 %d 像素, 上下 %d 像素\n", 
           start_x, start_y);
    printf("===================\n\n");
    
    // 映射 LCD 显存
    int screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    fb_mem = (unsigned short *)mmap(NULL, screensize,
                                     PROT_READ | PROT_WRITE, 
                                     MAP_SHARED, lcd_fd, 0);
    if (fb_mem == MAP_FAILED) {
        perror("Failed to mmap LCD");
        close(lcd_fd);
        return -1;
    }
    
    // 清屏为黑色
    clear_screen(fb_mem, vinfo.xres, vinfo.yres);
    
    // 2. 打开摄像头
    camera_fd = open(argv[1], O_RDWR);
    if (camera_fd < 0) {
        perror("Failed to open camera");
        goto cleanup_lcd;
    }
    
    // 3. 设置摄像头格式
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = cam_width;
    fmt.fmt.pix.height = cam_height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    
    printf("尝试设置摄像头为: %dx%d YUYV\n", cam_width, cam_height);
    
    if (ioctl(camera_fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("Failed to set camera format");
        goto cleanup_all;
    }
    
    // 摄像头可能调整了分辨率
    if (fmt.fmt.pix.width != cam_width || fmt.fmt.pix.height != cam_height) {
        printf("摄像头实际设置为: %dx%d\n", 
               fmt.fmt.pix.width, fmt.fmt.pix.height);
        // 重新计算居中位置
        cam_width = fmt.fmt.pix.width;
        cam_height = fmt.fmt.pix.height;
        start_x = (vinfo.xres - cam_width) / 2;
        start_y = (vinfo.yres - cam_height) / 2;
        printf("调整后显示位置: (%d, %d)\n", start_x, start_y);
    }
    
    // 4. 申请摄像头缓冲区
    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.count = 4;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    
    if (ioctl(camera_fd, VIDIOC_REQBUFS, &reqbuf) < 0) {
        perror("Failed to request buffers");
        goto cleanup_all;
    }
    
    // 5. 映射摄像头缓冲区
    for (i = 0; i < reqbuf.count; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        
        if (ioctl(camera_fd, VIDIOC_QUERYBUF, &buf) < 0) {
            perror("Failed to query buffer");
            goto cleanup_all;
        }
        
        camera_buffers[i] = mmap(NULL, buf.length,
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED, camera_fd, buf.m.offset);
        if (camera_buffers[i] == MAP_FAILED) {
            perror("Failed to mmap camera buffer");
            goto cleanup_all;
        }
        count++;
    }
    
    // 6. 放入队列
    for (i = 0; i < count; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        
        if (ioctl(camera_fd, VIDIOC_QBUF, &buf) < 0) {
            perror("Failed to queue buffer");
            goto cleanup_all;
        }
    }
    
    // 7. 启动摄像头
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(camera_fd, VIDIOC_STREAMON, &type) < 0) {
        perror("Failed to start streaming");
        goto cleanup_all;
    }
    
    printf("\n开始显示摄像头画面到 LCD...\n");
    printf("按 Ctrl+C 退出\n\n");
    
    // 8. 主循环
    int frame_count = 0;
    while (1) {
        memset(fds, 0, sizeof(fds));
        fds[0].fd = camera_fd;
        fds[0].events = POLLIN;
        
        int ret = poll(fds, 1, 1000);
        if (ret > 0 && (fds[0].revents & POLLIN)) {
            memset(&buf, 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            
            if (ioctl(camera_fd, VIDIOC_DQBUF, &buf) == 0) {
                // 转换并显示到指定位置
                yuyv_to_lcd_rgb565(camera_buffers[buf.index], fb_mem,
                                   cam_width, cam_height,
                                   vinfo.xres, vinfo.yres,
                                   start_x, start_y);
                
                frame_count++;
                if (frame_count % 30 == 0) {
                    printf("已显示 %d 帧\r", frame_count);
                    fflush(stdout);
                }
                
                // 重新放入队列
                ioctl(camera_fd, VIDIOC_QBUF, &buf);
            }
        }
    }
    
cleanup_all:
    // 停止摄像头
    ioctl(camera_fd, VIDIOC_STREAMOFF, &type);
    
    // 释放摄像头资源
    for (i = 0; i < count; i++) {
        if (camera_buffers[i]) {
            munmap(camera_buffers[i], buf.length);
        }
    }
    close(camera_fd);
    
cleanup_lcd:
    munmap(fb_mem, screensize);
    close(lcd_fd);
    
    return 0;
}