# The tests, built by `make test` only. With QtDBus, so the portal's bus
# gate is compiled and can be checked.
#
# At -Os, as the library ships (qtkit.pro) and as a consumer builds it: a
# suite at qmake's default -O2 has never seen the code that ships. beerssh
# measured why -- a rewrap fault passed 860 tests at -O2.

TEMPLATE = app
TARGET = qtkit-tests
CONFIG += console
CONFIG -= app_bundle
QT += dbus

include(../qtkit.pri)

QMAKE_CXXFLAGS += -Wall -Wextra
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -Os
SOURCES += main.cpp
