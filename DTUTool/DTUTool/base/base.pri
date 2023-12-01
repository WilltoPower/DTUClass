QT += network
#指定编译产生的文件分门别类放到对应目录
MOC_DIR     = $$PWD/../../temp/moc
RCC_DIR     = $$PWD/../../temp/rcc
UI_DIR      = $$PWD/../../temp/ui
OBJECTS_DIR = $$PWD/../../temp/obj

#指定编译生成的可执行文件放到源码上一级目录下的bin目录
!android {
!wasm {
DESTDIR = $$PWD/../../bin
}}

#把所有警告都关掉眼不见为净
CONFIG += warn_off
#开启大资源支持
CONFIG += resources_big
#开启后会将打印信息用控制台输出
#CONFIG += console
#开启后不会生成空的 debug release 目录
#CONFIG -= debug_and_release

HEADERS += \
    $$PWD/appdata.h \
    $$PWD/appinit.h \
    $$PWD/base64helper.h \
    $$PWD/iconhelper.h \
    $$PWD/quihelper.h

SOURCES += \
    $$PWD/appdata.cpp \
    $$PWD/appinit.cpp \
    $$PWD/base64helper.cpp \
    $$PWD/iconhelper.cpp \
    $$PWD/quihelper.cpp

#可以指定不加载对应的资源文件
!contains(DEFINES, no_qrc_image) {
RESOURCES += $$PWD/qrc/image.qrc
}

!contains(DEFINES, no_qrc_qm) {
RESOURCES += $$PWD/qrc/qm.qrc
}

!contains(DEFINES, no_qrc_font) {
RESOURCES += $$PWD/qrc/font.qrc
}
