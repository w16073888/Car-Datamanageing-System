QT += core network sql

CONFIG += console c++17
TARGET = 4s-server
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    src/db/DbManager.cpp \
    src/net/ServerCore.cpp \
    src/auth/AuthManager.cpp \
    src/cmd/CommandDispatcher.cpp \
    src/cmd/QueryCommands.cpp \
    src/cmd/DataDelete.cpp

HEADERS += \
    src/db/DbManager.h \
    src/net/JsonProtocol.h \
    src/net/ServerCore.h \
    src/auth/AuthManager.h \
    src/cmd/CommandDispatcher.h \
    src/cmd/QueryCommands.h \
    src/cmd/DataDelete.h
