#-------------------------------------------------
#
# Project created by QtCreator 2025-10-27T10:06:38
#
#-------------------------------------------------

QT       += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = bsci_demo
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += \
                $$PWD/../include/ \
                /usr/src/jetson_multimedia_api/include \
                /usr/local/cuda/include

QMAKE_LFLAGS += '-Wl,-rpath-link,../lib'

LIBS += \
        -L"../lib/" -lqcap \
        -L/usr/lib/aarch64-linux-gnu/tegra \
        -lnvbufsurface -lnvbufsurftransform \
        -L/usr/local/cuda/lib64 -lcuda -lcudart


SOURCES += \
    bmpfinder.cpp \
        main.cpp \
        mainwindow.cpp \
    processinference.cpp \
    setpassworddialog.cpp \
    logindialog.cpp \
    aspectratioframe.cpp

HEADERS += \
    bmpfinder.h \
        mainwindow.h \
    processinference.h \
    setpassworddialog.h \
    logindialog.h \
    aspectratioframe.h \
    testkit.h

FORMS += \
        mainwindow.ui \
    setpassworddialog.ui \
    logindialog.ui



