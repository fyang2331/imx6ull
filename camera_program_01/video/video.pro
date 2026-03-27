QT       += core gui multimedia concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    binarytask.cpp \
    edgedetection.cpp \
    graytask.cpp \
    main.cpp \
    mainwindow.cpp \
    processimage.cpp

HEADERS += \
    binarytask.h \
    edgedetection.h \
    graytask.h \
    mainwindow.h \
    processimage.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += /opt/fsl-imx-x11/4.1.15-2.1.0/sysroots/cortexa7hf-neon-poky-linux-gnueabi/usr/include
INCLUDEPATH += /opt/fsl-imx-x11/4.1.15-2.1.0/sysroots/cortexa7hf-neon-poky-linux-gnueabi/usr/include/libavcodec

#INCLUDEPATH += /home/book/100ask_imx6ull-sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/arm-buildroot-linux-gnueabihf/sysroot/usr/include/c++/7
INCLUDEPATH += /home/book/videoTools/ffmpeg_build/include
INCLUDEPATH += /home/book/videoTools/x264_build/include
INCLUDEPATH += /home/book/videoTools/openssl/include
INCLUDEPATH += /home/book/videoTools/opencv_build/include


LIBS += -L/home/book/videoTools/ffmpeg_build/lib \
        -L/home/book/videoTools/x264_build/lib \
        -L/home/book/videoTools/opencv_build/lib \
        -L/home/book/videoTools/openssl_build/lib


# 链接 FFmpeg 库
LIBS += -lavcodec -lavformat -lavutil -lswscale -lswresample

# 链接 x264 库
LIBS += -lx264

# 链接 OpenCV 库
LIBS += -lopencv_core \
        -lopencv_highgui \
        -lopencv_imgproc \
        -lopencv_videoio \
        -lopencv_imgcodecs

# 链接 OpenSSL 库
LIBS += -lssl -lcrypto

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
