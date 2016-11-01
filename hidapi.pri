# Idea from https://code.google.com/p/lcdhost/source/browse/linkdata/lh_api5/hidapi/HID.pri

INCLUDEPATH += $$PWD/hidapi

HEADERS += $$PWD/hidapi/hidapi.h

win32: SOURCES += $$PWD/windows/hid.c
win32: LIBS += -lsetupapi

unix:!macx: SOURCES += $$PWD/linux/hid.c
unix:!macx: LIBS += -ludev

macx: SOURCES += $$PWD/mac/hid.c
macx: LIBS += -framework CoreFoundation -framework IOKit
