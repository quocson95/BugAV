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

LIBS += -lavutil -lavformat -lavcodec -lswscale -lswresample -lavfilter

DISTFILES += \
    logo.png

FORMS += \
    form.ui \
    grid.ui

HEADERS += \
    BugPlayer/bugplayer.h \
    Decoder/decoder.h \
    Decoder/videodecoder.h \
    Demuxer/demuxer.h \
    Demuxer/handlerinterupt.h \
    Render/bugfilter.h \
    Render/render.h \
    RenderOuput/IBugAVRenderer.h \
    RenderOuput/bugglwidget.h \
    RenderOuput/ibugavdefaultrenderer.h \
    common/clock.h \
    common/common.h \
    common/define.h \
    common/framequeue.h \
    common/packetqueue.h \
    common/videostate.h \
    form.h \
    grid.h \
    marco.h \
    taskscheduler.h

SOURCES += \
    BugPlayer/bugplayer.cpp \
    Decoder/decoder.cpp \
    Decoder/videodecoder.cpp \
    Demuxer/demuxer.cpp \
    Demuxer/handlerinterupt.cpp \
    Render/bugfilter.cpp \
    Render/render.cpp \
    RenderOuput/IBugAVRenderer.cpp \
    RenderOuput/bugglwidget.cpp \
    RenderOuput/ibugavdefaultrenderer.cpp \
    common/clock.cpp \
    common/framequeue.cpp \
    common/packetqueue.cpp \
    common/videostate.cpp \
    form.cpp \
    grid.cpp \
    main.cpp \
    taskscheduler.cpp



