QT += core gui
QT += core sql

CONFIG += release

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = corefunction
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    basedataapi.cpp

HEADERS += \
        mainwindow.h \
    basedataapi.h

FORMS += \
        mainwindow.ui
