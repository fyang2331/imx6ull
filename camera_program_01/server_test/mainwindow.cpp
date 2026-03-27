#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->origal_label->setGeometry(10, 10, 640, 480);

    udpSocket = new QUdpSocket(this);

    udpSocket->bind(QHostAddress::Any, 20000);  // Listen on port 20000
    connect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::receiveDatagram);
    qDebug() << "initializeDecoder";
     initializeDecoder();

}

MainWindow::~MainWindow()
{
    avcodec_free_context(&codecContext);
    av_packet_unref(pkt);
    av_frame_free(&avFrame);
    sws_freeContext(swsContext);
    delete ui;
}

void MainWindow::initializeDecoder() {

    codecContext = avcodec_alloc_context3(avcodec_find_decoder(AV_CODEC_ID_H264));
    if (!codecContext) {
        qDebug() << "Failed to allocate codec context.";
        return;
    }


    codecContext->bit_rate = 500000; // 500kbps 码率
    codecContext->width = 640;
    codecContext->height = 480;
    codecContext->time_base = {1, 33};
    codecContext->framerate = {33, 1};
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P;



    qDebug() << "avcodec_open2";

    if (avcodec_open2(codecContext, avcodec_find_decoder(AV_CODEC_ID_H264), nullptr) < 0) {
         qDebug() << "Failed to open H.264 codec.";
         return;
     }

    pkt = av_packet_alloc();
    avFrame = av_frame_alloc();
    swsContext = nullptr;
}

void MainWindow::receiveDatagram() {
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        qDebug() << "Received UDP packet of size:" << datagram.size();

        RTPHeader *rtpHeader = reinterpret_cast<RTPHeader *>(datagram.data());

        // 根据 RTP 包头的序列号进行数据包拼接
        int packetIndex = qFromBigEndian(rtpHeader->sequenceNumber);
        qDebug() << "packetIndex:" << packetIndex;
        qDebug() << "rtpHeader->sequenceNumber" << qFromBigEndian(rtpHeader->sequenceNumber);
        QByteArray packetData = datagram.mid(12);  // 从 RTP 包头之后的数据部分

        packetBuffer[packetIndex] = packetData;

        qDebug() << "packetBuffer size: " << packetBuffer.size();

        qDebug() << "rtpHeader->m_marker:" << rtpHeader->m_marker;
        bool isLastPacket = rtpHeader->m_marker & 1;
        qDebug() << "Marker bit: " << rtpHeader->m_marker;


        if (isLastPacket) {
            qDebug() << "isLastPacket";
            QByteArray fullData;

            // 按序拼接数据包
            for (auto it = packetBuffer.begin(); it != packetBuffer.end(); ++it) {
                fullData.append(it.value());
                qDebug() << "Appending packet " << it.key() << " with size " << it.value().size();
            }

            // 重组完整数据包
            pkt->data = reinterpret_cast<uint8_t *>(fullData.data());
            pkt->size = fullData.size();

            packetBuffer.clear();  // 清空缓冲区

            decodeFrame();  // 解码完整数据
        }
    }
}

void MainWindow::decodeFrame() {
    qDebug() << "decodeFrame";
    
    // 检查 packet 有效性
    if (!pkt || pkt->data == nullptr || pkt->size <= 0) {
        qDebug() << "Packet data is empty or invalid!";
        return;
    }
    
    qDebug() << "Packet data size: " << pkt->size;
    
    // 打印前几个字节用于调试
    for (int i = 0; i < pkt->size && i < 10; i++) {
        qDebug() << "Byte " << i << ": " << static_cast<int>(pkt->data[i]);
    }
    
    // 旧版 API：直接解码
    int got_frame = 0;
    int len = avcodec_decode_video2(codecContext, avFrame, &got_frame, pkt);
    
    if (len < 0) {
        qDebug() << "Error decoding packet: " << len;
        av_packet_unref(pkt);  // 释放 packet 引用，但不释放指针本身
        return;
    }
    
    qDebug() << "Decoded bytes: " << len;
    qDebug() << "pkt size: " << pkt->size;
    
    // 检查是否成功解码出一帧
    if (got_frame) {
        qDebug() << "Frame decoded successfully";
        
        // 计算帧大小（YUV420P）
        int frameSize = avFrame->linesize[0] * avFrame->height +
                        avFrame->linesize[1] * avFrame->height / 2 +
                        avFrame->linesize[2] * avFrame->height / 2;
        qDebug() << "AVFrame size (YUV420P): " << frameSize;
        
        // 显示帧
        displayFrame();
        
        // 注意：旧 API 不会自动释放 frame，需要手动释放
        // 但 displayFrame() 可能会使用 frame，所以在这里不释放
    } else {
        qDebug() << "No frame decoded from this packet";
    }
    
    // 如果 packet 中的数据没有完全消耗（这种情况较少见）
    if (pkt->size > len) {
        qDebug() << "Packet has remaining data: " << (pkt->size - len) << " bytes";
        // 可以选择递归处理剩余数据
        AVPacket remainingPkt = *pkt;
        remainingPkt.data += len;
        remainingPkt.size -= len;
        // 递归调用自己处理剩余数据（注意避免无限递归）
        // decodeFrame(&remainingPkt);
    }
    
    // 释放 packet 引用
    av_packet_unref(pkt);
}

void MainWindow::displayFrame() {
    // Convert AVFrame to QImage for display
    int width = avFrame->width;
    int height = avFrame->height;

    // Allocate buffer for the converted frame (YUV420 to RGB)
    uint8_t *rgbData = new uint8_t[width * height * 3];

    // Convert the frame to RGB using swscale
    if (!swsContext) {
        swsContext = sws_getContext(width, height, codecContext->pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BICUBIC, nullptr, nullptr, nullptr);
    }

    uint8_t *srcData[4] = {avFrame->data[0], avFrame->data[1], avFrame->data[2], nullptr};
    int srcStride[4] = {avFrame->linesize[0], avFrame->linesize[1], avFrame->linesize[2], 0};

    uint8_t *dstData[4] = {rgbData, nullptr, nullptr, nullptr};
    int dstStride[4] = {width * 3, 0, 0, 0};

    sws_scale(swsContext, srcData, srcStride, 0, height, dstData, dstStride);

    // Create QImage from the RGB data
    QImage img(rgbData, width, height, QImage::Format_RGB888);
    ui->origal_label->setPixmap(QPixmap::fromImage(img));

    delete[] rgbData;
}
