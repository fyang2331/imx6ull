#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QByteArray>
#include <QHostAddress>
#include <QImage>
#include <QPixmap>
#include <QVector>
#include <QMap>
#include <QDebug>
#include <QtCore>



extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/opt.h>
}


struct RTPHeader {
    quint8 version_p_x_cc;  // 版本 (2 bits) | P(1 bit) | X(1 bit) | CC(4 bits)
    quint8 m_pt;            // PT (7 bits)
     quint8 m_marker;        // Marker 位 (1 bit)
    quint16 sequenceNumber; // 序列号
    quint32 timestamp;      // 时间戳
    quint32 ssrc;           // SSRC 标识
};


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    void initializeDecoder(); // Initialize FFmpeg decoder
        QUdpSocket *udpSocket;   // UDP socket for communication
        AVCodecContext *codecContext;  // Codec context for video decoding
        AVFrame *avFrame;            // Decoded frame
        AVPacket *pkt;                // Packet for holding received data
        SwsContext *swsContext;     // SwsContext for pixel format conversion

        QMap<int, QByteArray> packetBuffer; // Buffer to hold packet data (using packet index)
        int expectedNumPackets = 0;

private slots:
    void receiveDatagram();
    void decodeFrame();
    void displayFrame();

};
#endif // MAINWINDOW_H
