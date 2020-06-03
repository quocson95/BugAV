QT += core widgets concurrent opengl

CONFIG += c++14
#CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += MAKE_LIB
CONFIG += MAKE_LIB

MAKE_LIB {
    TEMPLATE = lib
}

QMAKE_LFLAGS_RELEASE += /MAP
QMAKE_CFLAGS_RELEASE += /Zi
QMAKE_LFLAGS_RELEASE += /debug /opt:ref

LIBS +=-L$$PWD/ffmpeg/lib -lavutil -lavformat -lavcodec -lswscale -lswresample -lavfilter

INCLUDEPATH += $$PWD/ffmpeg/include

DISTFILES += \
    logo.png \
    rgb2yuv_document.txt

FORMS += \
    form.ui \
    grid.ui \
    statckwidget.ui

HEADERS += \
    BugPlayer/bugplayer.h \
    BugPlayer/bugplayer_p.h \
    Decoder/decoder.h \
    Decoder/videodecoder.h \
    Demuxer/demuxer.h \
    Demuxer/handlerinterupt.h \
    Render/bugfilter.h \
    Render/render.h \
    RenderOuput/IBugAVRenderer.h \
    RenderOuput/bugglrgb.h \
    RenderOuput/bugglwidget.h \     \
    RenderOuput/glvideowidget.h \
    RenderOuput/ibugavdefaultrenderer.h \
    RenderOuput/yuv_rgb.h \
    common/clock.h \
    common/common.h \
    common/define.h \
    common/framequeue.h \
    common/packetqueue.h \
    common/videostate.h \
    form.h \
    grid.h \
    marco.h \
    statckwidget.h \

SOURCES += \
    BugPlayer/bugplayer.cpp \
    BugPlayer/bugplayer_p.cpp \
    Decoder/decoder.cpp \
    Decoder/videodecoder.cpp \
    Demuxer/demuxer.cpp \
    Demuxer/handlerinterupt.cpp \
    Render/bugfilter.cpp \
    Render/render.cpp \
    RenderOuput/IBugAVRenderer.cpp \
    RenderOuput/bugglrgb.cpp \
    RenderOuput/bugglwidget.cpp \
    RenderOuput/glvideowidget.cpp \
    RenderOuput/ibugavdefaultrenderer.cpp \
    RenderOuput/yuv_rgb.c \
    common/clock.cpp \
    common/framequeue.cpp \
    common/packetqueue.cpp \
    common/videostate.cpp \
    form.cpp \
    grid.cpp \
    main.cpp \
    statckwidget.cpp \
