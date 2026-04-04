#-------------------------------------------------
#
# Project created by QtCreator 2025-08-29T16:04:19
#
#-------------------------------------------------

QT       -= gui

TARGET = robot
TEMPLATE = lib

DEFINES += MOTION_CONTROL_LIB_LIBRARY

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
        motion_control_lib.cpp \
    rpmsg.cpp \
    python.cpp

HEADERS += \
        motion_control_lib.h \
        motion_control_lib_global.h \ 
    rpmsg.h \
    error.h \
    global.h \
    python.h


unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    forward_inverse_solution.py


