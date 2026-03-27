#include "processimage.h"
#include <arpa/inet.h>
#include "avcodec.h"
#include <QDebug>

ProcessImage::ProcessImage(const QImage &image, int fps)
    : image(image), fps(fps) {}

ProcessImage::~ProcessImage() {
    if (codecContext) {
        avcodec_close(codecContext);
        avcodec_free_context(&codecContext);
    }
    if (pkt) {
        av_packet_free(&pkt);
    }
    if (avFrame) {
        av_frame_free(&avFrame);
    }
}

void ProcessImage::run() {
    if(initializeEncoder() != true) {
        qDebug() << "初始化编码器失败";
        return;
    }
    sendFrame(image);
}

bool ProcessImage::initializeEncoder() {
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        qDebug() << "无法找到 H.264 编码器";
        return false;
    }

    codecContext = avcodec_alloc_context3(codec);
    codecContext->bit_rate = 500000; // 500kbps 码率
    codecContext->width = 640;
    codecContext->height = 480;
    codecContext->time_base = (AVRational){1, 33};  // FFmpeg 3.0 使用这种赋值方式
    codecContext->framerate = (AVRational){33, 1};
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    codecContext->gop_size = 10;  // 关键帧间隔
    codecContext->max_b_frames = 0;  // 不使用B帧，减少延迟

    // 在打开编码器之前设置 "zerolatency" 模式
    if (av_opt_set(codecContext->priv_data, "tune", "zerolatency", 0) < 0) {
        qDebug() << "无法设置零延迟模式";
        // 不是致命错误，继续执行
    }

    // 设置编码器参数以减少延迟
    av_opt_set(codecContext->priv_data, "preset", "ultrafast", 0);
    av_opt_set(codecContext->priv_data, "profile", "baseline", 0);

    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        qDebug() << "无法打开 H.264 编码器";
        return false;
    }

    pkt = av_packet_alloc();
    avFrame = av_frame_alloc();
    avFrame->format = codecContext->pix_fmt;
    avFrame->width = codecContext->width;
    avFrame->height = codecContext->height;
    
    // 为帧分配缓冲区
    if (av_frame_get_buffer(avFrame, 32) < 0) {
        qDebug() << "无法为帧分配缓冲区";
        return false;
    }
    
    qDebug() << "initializeEncoder is ok";
    return true;
}

void ProcessImage::sendFrame(const QImage &image) {
    qDebug() << "sendFrame is start";
    
    // 将 QImage 转换为 OpenCV Mat
    Mat frame = Mat(image.height(), image.width(), CV_8UC3, (void *)image.bits(), image.bytesPerLine()).clone();
    cv::resize(frame, frame, Size(640, 480));
    
    if (frame.empty()) {
        qDebug() << "frame is empty!";
        return;
    } else {
        qDebug() << "frame is not empty, size:" << frame.cols << "x" << frame.rows;
    }

    QVector<uchar> encodedData;

    if (!encodeFrameH264(frame, encodedData)) {
        qDebug() << "H.264 编码失败！";
        return;
    }

    sendUdpPackets(encodedData);
}

bool ProcessImage::encodeFrameH264(const Mat &frame, QVector<uchar> &encodedData) {
    qDebug() << "encodeFrameH264 started";
    
    Mat yuv;
    cvtColor(frame, yuv, COLOR_BGR2YUV_I420);
    
    if (yuv.empty()) {
        qDebug() << "yuv is empty!";
        return false;
    } else {
        qDebug() << "yuv is not empty, size:" << yuv.cols << "x" << yuv.rows;
    }

    static int frame_count = 0;
    avFrame->pts = frame_count++;  // 更新帧计数器并设置 PTS

    // 确保帧是可写的
    if (av_frame_make_writable(avFrame) < 0) {
        qDebug() << "av_frame_make_writable failed";
        return false;
    }

    // 复制YUV数据到AVFrame
    int y_size = codecContext->width * codecContext->height;
    int uv_size = y_size / 4;
    
    memcpy(avFrame->data[0], yuv.data, y_size);  // Y平面
    memcpy(avFrame->data[1], yuv.data + y_size, uv_size);  // U平面
    memcpy(avFrame->data[2], yuv.data + y_size + uv_size, uv_size);  // V平面

    encodedData.clear();

    // FFmpeg 3.0 使用旧版编码API
    int got_packet = 0;
    int ret = avcodec_encode_video2(codecContext, pkt, avFrame, &got_packet);
    
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "avcodec_encode_video2 failed with error:" << errbuf << "code:" << ret;
        return false;
    }

    if (got_packet && pkt->size > 0) {
        qDebug() << "Received packet, size:" << pkt->size;
        encodedData.resize(pkt->size);
        memcpy(encodedData.data(), pkt->data, pkt->size);
        av_packet_unref(pkt);  // 清除数据包
    } else {
        qDebug() << "No packet received from encoder";
        return false;
    }

    qDebug() << "After avcodec_encode_video2";
    return !encodedData.empty();
}

void ProcessImage::sendUdpPackets(const QVector<uchar> &data) {
    static quint16 sequenceNumber = 0;  // 改为static以保持序列号连续

    // 将 QVector<uchar> 转换为 QByteArray
    QByteArray packet(reinterpret_cast<const char*>(data.data()), data.size());

    const int maxRtpPayloadSize = 1400;  // 设定每个 RTP 包的最大负载大小

    // 计算数据包数量
    int numPackets = (packet.size() + maxRtpPayloadSize - 1) / maxRtpPayloadSize;
    qDebug() << "Splitting" << packet.size() << "bytes into" << numPackets << "RTP packets";

    // 分割数据包并逐个发送
    for (int packetIndex = 0; packetIndex < numPackets; ++packetIndex) {
        int offset = packetIndex * maxRtpPayloadSize;
        int size = qMin(maxRtpPayloadSize, packet.size() - offset);

        QByteArray packetFragment = packet.mid(offset, size);
        bool isLastPacket = (packetIndex == numPackets - 1);

        // 创建 RTP 包头
        QByteArray rtpPacket;
        rtpPacket.resize(12 + packetFragment.size());
        RTPHeader *rtpHeader = reinterpret_cast<RTPHeader *>(rtpPacket.data());

        // 初始化所有字段
        memset(rtpHeader, 0, 12);
        
        // 设置 RTP 包头
        rtpHeader->version_p_x_cc = (2 << 6); // 版本号2，其他位为0
        
        // 设置负载类型和Marker位
        if (isLastPacket) {
            rtpHeader->m_pt = (1 << 7) | 96;  // Marker位(第8位)为1，负载类型为96
        } else {
            rtpHeader->m_pt = 96;  // 负载类型为96
        }
        
        // 设置序列号（网络字节序）
        rtpHeader->sequenceNumber = htons(sequenceNumber);
        
        // 设置时间戳（网络字节序）
        quint32 timestamp = QDateTime::currentMSecsSinceEpoch() & 0xFFFFFFFF;
        rtpHeader->timestamp = htonl(timestamp);
        
        // 设置SSRC（网络字节序）
        rtpHeader->ssrc = htonl(0x12345678);

        // 将数据复制到 RTP 包
        memcpy(rtpPacket.data() + 12, packetFragment.data(), packetFragment.size());

        // 发送RTP包
        emit dataReadyToUpload(rtpPacket);
        
        sequenceNumber++;
        
        qDebug() << "Sent packet" << packetIndex + 1 << "/" << numPackets 
                 << "seq:" << (sequenceNumber - 1) 
                 << "size:" << packetFragment.size()
                 << "last:" << isLastPacket;
    }
}