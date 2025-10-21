#-------------------------------------------------
#
# Project created by QtCreator 2014-02-02T22:46:38
#
#-------------------------------------------------

QT       += core xml sql

QT       -= gui

TARGET = jxformtomysql
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../3rdparty

greaterThan(QT_MAJOR_VERSION, 5) {
    LIBS += -lquazip1-qt6 -lcsv
    QT += core5compat
} else {
    LIBS += -lquazip1-qt5 -lcsv
}
SOURCES += main.cpp
