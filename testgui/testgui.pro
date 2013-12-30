TARGET = testgui
SOURCES = hidapigui.cpp

QT += widgets

INCLUDEPATH += ../hidapi
HEADERS += ../hidapi/hidapi.h

mac {
 SOURCES += ../mac/hid.c
 QMAKE_LFLAGS += -framework IOKit -framework CoreFoundation
}
