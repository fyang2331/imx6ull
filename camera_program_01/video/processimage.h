#ifndef PROCESSIMAGE_H
#define PROCESSIMAGE_H

#include <QObject>
#include <QRunnable>
#include <QUdpSocket>
#include <QImage>
#include <QThread>
#include <QtEndian>
#include <QDateTime>
#include <opencv2/opencv.hpp>


#include <QVector>
#include <QtGlobal>


extern "C" {
#include <QtCore/qbytearray.h>
#include <libavutil/avutil.h>
#include <libavutil/pixfmt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <openssl/evp.h>
#include <libswscale/swscale.h>

}


struct RTPHeader {
    quint8 version_p_x_cc;  // 版本 (2 bits) | P(1 bit) | X(1 bit) | CC(4 bits)
    quint8 m_pt;            // PT (7 bits)
     quint8 m_marker;        // Marker 位 (1 bit)
    quint16 sequenceNumber; // 序列号
    quint32 timestamp;      // 时间戳
    quint32 ssrc;           // SSRC 标识
};

using namespace cv;

class ProcessImage : public QObject, public QRunnable {
    Q_OBJECT

public:
    ProcessImage(const QImage &image, int fps);
    ~ProcessImage();
    void sendFrame(const QImage &image);
    void run() override;

private:

    QImage image;
    int fps;

    Mat frame;

    AVCodec *codec;
    AVCodecContext *codecContext;
    AVPacket *pkt;
    AVFrame *avFrame;

    bool initializeEncoder();
    bool encodeFrameH264(const Mat &frame, QVector<uchar> &encodedData);
    void sendUdpPackets(const QVector<uchar> &data);

signals:
    void dataReadyToUpload(const QByteArray &data);

};

#endif // PROCESSIMAGE_H
