QT += core sql

CONFIG += console c++17
TARGET = DbSetup
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp

# 发布时需要 qsqlmysql 驱动 + libmysql.dll（与服务端相同）
