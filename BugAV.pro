QT += core widgets concurrent

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

SOURCES += \
#        cmdutils.c \
#        compat/atomics/pthread/stdatomic.c \
#        compat/djgpp/math.c \
#        compat/getopt.c \
#        compat/msvcrt/snprintf.c \
#        compat/strtod.c \
        clock.cpp \
        demux.cpp \
        demuxer.cpp \
        framequeue.cpp \
        main.cpp \
        packetqueue.cpp \
        streaminfo.cpp
HEADERS += \
    clock.h \
    common.h \
#    compat/aix/math.h \
#    compat/atomics/dummy/stdatomic.h \
#    compat/atomics/gcc/stdatomic.h \
#    compat/atomics/pthread/stdatomic.h \
#    compat/atomics/suncc/stdatomic.h \
#    compat/atomics/win32/stdatomic.h \
#    compat/avisynth/avisynth_c.h \
#    compat/avisynth/avs/capi.h \
#    compat/avisynth/avs/config.h \
#    compat/avisynth/avs/types.h \
#    compat/avisynth/avxsynth_c.h \
#    compat/avisynth/windowsPorts/basicDataTypeConversions.h \
#    compat/avisynth/windowsPorts/windows2linux.h \
#    compat/cuda/cuda_runtime.h \
#    compat/cuda/dynlink_loader.h \
#    compat/dispatch_semaphore/semaphore.h \
#    compat/djgpp/math.h \
#    compat/float/float.h \
#    compat/float/limits.h \
#    compat/msvcrt/snprintf.h \
#    compat/os2threads.h \
#    compat/va_copy.h \
#    compat/w32dlfcn.h \
#    compat/w32pthreads.h \
    config.h \
    define.h \
    demux.h \
    demuxer.h \
    framequeue.h \
    packetqueue.h \
    streaminfo.h


#INCLUDEPATH += D:/libbase/ffmpeg/include
#DEPENDPATH += D:/libbase/ffmpeg/include
#LIBS += -LD:/libbase/ffmpeg/lib -lavutil -lavformat -lavcodec

LIBS += -lSDL2 -lavutil -lavformat -lavcodec -lswscale -lswresample -lavfilter

#INCLUDEPATH += /home/sondq/Documents/dev/qt/QtAV/ffmpeg/include
#LIBS += -L/home/sondq/Documents/dev/qt/QtAV/ffmpeg/lib
DISTFILES += \
    compat/cuda/ptx2c.sh \
    compat/solaris/make_sunver.pl \
    compat/windows/makedef \
    compat/windows/mslink
