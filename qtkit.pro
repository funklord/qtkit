# The library on its own: what `make` builds, so the tree compiles without a
# consumer. Applications use qtkit.pri instead.

TEMPLATE = lib
CONFIG += staticlib
TARGET = qtkit
DESTDIR = lib
QT += dbus

include(qtkit.pri)

# Size over speed, as every project here builds (build-and-commit.md); the
# sections so a consumer's linker can drop what it does not call.
QMAKE_CXXFLAGS += -Wall -Wextra -ffunction-sections -fdata-sections
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -Os
