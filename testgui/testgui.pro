TARGET = testgui
SOURCES = hidapigui.cpp

QT += widgets

INCLUDEPATH += ../hidapi
HEADERS += ../hidapi/hidapi.h


win32 {
 SOURCES += ../windows/hid.c
 LIBS += -lSetupApi
}

unix {
  mac {
    SOURCES += ../mac/hid.c
    QMAKE_LFLAGS += -framework IOKit -framework CoreFoundation
  }
}
