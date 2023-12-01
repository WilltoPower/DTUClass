QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat

TARGET      = DTUTools
TEMPLATE    = app

#QMAKE_MSC_VER = 1900

HEADERS     += $$PWD/src/head.h
SOURCES     += $$PWD/src/main.cpp
RESOURCES   += $$PWD/src/other/main.qrc
RESOURCES   += $$PWD/qss/qss.qrc

INCLUDEPATH += $$PWD/src
INCLUDEPATH += $$PWD/src/form
include ($$PWD/src/form/form.pri)

INCLUDEPATH += $$PWD/base
include ($$PWD/base/base.pri)
