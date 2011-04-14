include(../config.pri)
TEMPLATE = app

QT       += core network testlib script
QT       -= gui

CONFIG   += console
CONFIG   -= app_bundle

INCLUDEPATH = . ../pillowcore
DEPENDPATH = . ../pillowcore
LIBS += -L../lib -l$${PILLOWCORE_LIB_NAME}
unix: POST_TARGETDEPS += ../lib/lib$${PILLOWCORE_LIB_NAME}.a
win32: POST_TARGETDEPS += ../lib/$${PILLOWCORE_LIB_NAME}.lib

SOURCES += main.cpp \
    HttpServerTest.cpp \
    HttpConnectionTest.cpp \
    HttpHandlerTest.cpp \
    HttpsServerTest.cpp

HEADERS += \
    HttpServerTest.h \
    HttpConnectionTest.h \
    HttpHandlerTest.h \
    HttpsServerTest.h

RESOURCES += tests.qrc

