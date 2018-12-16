# This is a qmake project file, provided as an example on how to use qmake with QtGStreamer.

TEMPLATE = app
TARGET = pi-tracker-client

# produce nice compilation output
CONFIG += silent

# Tell qmake to use pkg-config to find QtGStreamer.
CONFIG += link_pkgconfig

# Now tell qmake to link to QtGStreamer and also use its include path and Cflags.
contains(QT_VERSION, ^4\\..*) {
  PKGCONFIG += QtGStreamer-1.0 QtGStreamerUi-1.0 QtGStreamerUtils-1.0
}
contains(QT_VERSION, ^5\\..*) {
  PKGCONFIG += Qt5GStreamer-1.0 Qt5GStreamerUi-1.0 Qt5GStreamerUtils-1.0
}

# Recommended if you are using g++ 4.5 or later. Must be removed for other compilers.
#QMAKE_CXXFLAGS += -std=c++0x

# Recommended, to avoid possible issues with the "emit" keyword
# You can otherwise also define QT_NO_EMIT, but notice that this is not a documented Qt macro.
#DEFINES += QT_NO_KEYWORDS

# This example has no GUI
QT += core gui network widgets opengl

# Input
SOURCES += main.cpp \
    mainwindow.cpp \
    connectdialog.cpp \
    overlaywidget.cpp \
    appsink.cpp \
    pipeline.cpp \
    connection.cpp

FORMS += \
    mainwindow.ui \
    connectdialog.ui

HEADERS += \
    mainwindow.h \
    common.h \
    connectdialog.h \
    overlaywidget.h \
    appsink.h \
    pipeline.h \
    connection.h
