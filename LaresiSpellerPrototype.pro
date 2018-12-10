#-------------------------------------------------
#
# Project created by QtCreator 2016-09-04T03:46:54
#
#-------------------------------------------------

QT       += core gui network opengl

CONFIG += c++11 console

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LaresiBCI
TEMPLATE = app


SOURCES += src\main.cpp\
        src\configpanel.cpp \
    src\matrixlayout.cpp \
    src\ovmarkersender.cpp \
    src\randomflashsequence.cpp \
    src\ssvep.cpp \
    src\hybridstimulation.cpp \
    src\ellipselayout.cpp \
    src\hybridgridstimulation.cpp \
    src\ssvepgl.cpp \
    src\utils.cpp \
    src\speller.cpp \
    src\test.cpp \
    src/flashingspeller.cpp \
    src/facespeller.cpp \
    src/paradigm.cpp \
    src/erp.cpp \
    src/jsonserializer.cpp

HEADERS  += include\configpanel.h \
    include\ovtk_stimulations.h \
    include\matrixlayout.h \
    include\ovmarkersender.h \
    include\randomflashsequence.h \
    include\ssvep.h \
    include\hybridstimulation.h \
    include\ellipselayout.h \
    include\hybridgridstimulation.h \
    include\ssvepgl.h \
    include\utils.h \
    include\speller.h \
    include\test.h \
    include\glutils.h \
    include/flashingspeller.h \
    include/facespeller.h \
    include/paradigm.h \
    include/erp.h \
    include/serializable.h \
    include/serializer.h \
    include/jsonserializer.h

INCLUDEPATH += include\

FORMS    += src\ui\configpanel.ui \
    src\ui\ssvep.ui \
    src\ui\hybridstimulation.ui \
    src\ui\hybridgridstimulation.ui \
    src\ui\ssvepsingle.ui \
    src\ui\speller.ui

RESOURCES +=  assets\asset.qrc

DISTFILES += \
    MyOVBox.py \
    src\shaders\sh_v.vert \
    src\shaders\sh_f.frag \
    .gitignore
