#-------------------------------------------------
#
# Project created by QtCreator 2017-05-23T17:55:05
#
#-------------------------------------------------

QT       += core gui
QT       += network

CONFIG += link_pkgconfig
PKGCONFIG += x11

CONFIG += c++14
QMAKE_CXXFLAGS += -std=c++1y

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LinuxRemoteMouse
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \  
    seconddisplay.cpp \      
    udpmousedriver.cpp \
    cursorobserver.cpp

HEADERS  += mainwindow.h \
    seconddisplay.h \   
    udpmousedriver.h \
    cursorobserver.h

FORMS    += mainwindow.ui \
    seconddisplay.ui
