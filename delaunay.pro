#-------------------------------------------------
#
# Project created by QtCreator 2019-02-06T12:32:49
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = delaunay
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    graphicscene.cpp \
    graphics_view_zoom.cpp \
    tr_object.cpp \
    rectdetails.cpp \
    circledetails.cpp \
    pointszone.cpp \
    visualization.cpp

HEADERS += \
        mainwindow.h \
    graphicscene.h \
    graphics_view_zoom.h \
    tr_object.h \
    rectdetails.h \
    circledetails.h \
    pointszone.h \
    visualization.h

FORMS += \
        mainwindow.ui \
    rectdetails.ui \
    circledetails.ui \
    pointszone.ui \
    visualization.ui
RC_ICONS = triangle.ico
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/x64/ -lfade2D_x64_v141_Release
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/x64/ -lfade2D_x64_v141_Debug

INCLUDEPATH += $$PWD/include_fade2d
DEPENDPATH += $$PWD/include_fade2d

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include


unix|win32: LIBS += -L$$PWD/examples/lib_win64/ -llapack_win64_MT
INCLUDEPATH += $$PWD/examples/lib_win64
DEPENDPATH += $$PWD/examples/lib_win64

unix|win32: LIBS += -L$$PWD/examples/lib_win64/ -lblas_win64_MT
INCLUDEPATH += $$PWD/examples/lib_win64
DEPENDPATH += $$PWD/examples/lib_win64

QMAKE_EXTRA_TARGETS += before_build makefilehook

makefilehook.target = $(MAKEFILE)
makefilehook.depends = .beforebuild

PRE_TARGETDEPS += .beforebuild

before_build.target = .beforebuild
before_build.depends = FORCE
before_build.commands = chcp 1251
