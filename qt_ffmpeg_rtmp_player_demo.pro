QT       += core gui opengl multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    FFmpegPlayer.cpp \
    QGLPlayerWidget.cpp \
    QPlayerWidget.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    FFmpegPlayer.h \
    QAudioPlayer.h \
    QGLPlayerWidget.h \
    QPlayerWidget.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += -lopengl32  -lGLU32
LIBS += -L$$PWD/../ffmpeg-master-latest-win64-gpl-shared/lib/ -lavutil -lavformat -lavcodec -lswscale

INCLUDEPATH += $$PWD/../ffmpeg-master-latest-win64-gpl-shared/include
DEPENDPATH += $$PWD/../ffmpeg-master-latest-win64-gpl-shared/include
