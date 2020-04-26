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

#LIBS += -lbugav

SOURCES += \
    main.cpp

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../build-BugAV-Desktop_Qt_5_12_6_GCC_64bit-Debug/release/ -lBugAV
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../build-BugAV-Desktop_Qt_5_12_6_GCC_64bit-Debug/debug/ -lBugAV
else:unix: LIBS += -L$$PWD/../build-BugAV-Desktop_Qt_5_12_6_GCC_64bit-Debug/ -lBugAV

INCLUDEPATH += $$PWD/../build-BugAV-Desktop_Qt_5_12_6_GCC_64bit-Debug
DEPENDPATH += $$PWD/../build-BugAV-Desktop_Qt_5_12_6_GCC_64bit-Debug
