# The tests, built by `make test` only. With QtDBus, so the portal's bus
# gate is compiled and can be checked.

TEMPLATE = app
TARGET = qtkit-tests
CONFIG += console
CONFIG -= app_bundle
QT += dbus

include(../qtkit.pri)

QMAKE_CXXFLAGS += -Wall -Wextra
SOURCES += main.cpp
