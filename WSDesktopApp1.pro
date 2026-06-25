QT       += core gui widgets printsupport serialport
CONFIG   += c++17

TARGET = WeatherStation
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    qcustomplot.cpp

HEADERS += \
    controlSum.h \
    mainwindow.h \
    packet.h \
    qcustomplot.h

FORMS += \
    mainwindow.ui
