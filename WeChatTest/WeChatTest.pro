#-------------------------------------------------
#
# Project created by QtCreator 2021-05-11T14:38:30
#
#-------------------------------------------------

QT       += core gui


RC_ICONS = message.ico

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = WeChatTest
TEMPLATE = app

include(./netapi/netapi.pri)
include(./RecordVideo/RecordVideo.pri)
include(./RecordAudio/RecordAudio.pri)
include(./uiapi/uiapi.pri)

INCLUDEPATH += ./netapi/
INCLUDEPATH += ./RecordVideo/
INCLUDEPATH += ./RecordAudio/
INCLUDEPATH += ./uiapi/


SOURCES += main.cpp\
        wechat.cpp \
    ckernel.cpp \
    chatdialog.cpp \
    logindialog.cpp \
    roomdialog.cpp \
    useritem.cpp \
    videoitem.cpp \
    notify.cpp \
    fileitem.cpp \
    bqform.cpp

HEADERS  += wechat.h \
    ckernel.h \
    chatdialog.h \
    logindialog.h \
    roomdialog.h \
    useritem.h \
    videoitem.h \
    notify.h \
    fileitem.h \
    bqform.h

FORMS    += wechat.ui \
    chatdialog.ui \
    logindialog.ui \
    roomdialog.ui \
    useritem.ui \
    videoitem.ui \
    notify.ui \
    fileitem.ui \
    bqform.ui

RESOURCES += \
    resource.qrc
