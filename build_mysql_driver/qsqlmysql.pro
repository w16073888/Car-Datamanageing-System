TEMPLATE = lib
TARGET = qsqlmysql
CONFIG += plugin
CONFIG += c++17

QT += core sql
QT += sql-private core-private

SOURCES = \
    main.cpp \
    qsql_mysql.cpp

HEADERS = \
    qsql_mysql_p.h

OTHER_FILES = \
    mysql.json

INCLUDEPATH += /d/mysql-8.0.46-winx64/include
LIBS += /d/mysql-8.0.46-winx64/lib/libmysql.lib

# Output directly to Qt's plugin directory
DESTDIR = $$[QT_INSTALL_PLUGINS]/sqldrivers

DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII QT_NO_CONTEXTLESS_CONNECT QT_PLUGIN
