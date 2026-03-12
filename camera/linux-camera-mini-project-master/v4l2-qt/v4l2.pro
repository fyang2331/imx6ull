QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# 交叉编译配置
CROSS_COMPILE = arm-linux-gnueabihf-
QMAKE_CC = $${CROSS_COMPILE}gcc
QMAKE_CXX = $${CROSS_COMPILE}g++
QMAKE_LINK = $${CROSS_COMPILE}g++
QMAKE_LINK_SHLIB = $${CROSS_COMPILE}g++

# 指定 ARM 架构
QMAKE_CFLAGS += -march=armv7-a -mfpu=vfpv3-d16
QMAKE_CXXFLAGS += -march=armv7-a -mfpu=vfpv3-d16

# 告诉 qmake 使用 ARM 版的 Qt 库
# 注意：这里假设你已经有 ARM 版的 Qt 库
# 如果没有，需要先解决 Qt 库的问题
QMAKE_LIBDIR += /usr/lib/arm-linux-gnueabihf
INCLUDEPATH += /usr/include/arm-linux-gnueabihf/qt5

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    showphoto.cpp \
    v4l2.cpp

HEADERS += \
    showphoto.h \
    v4l2.h

FORMS += \
    showphoto.ui \
    v4l2.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target