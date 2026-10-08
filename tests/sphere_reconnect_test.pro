QT += core gui widgets
CONFIG += console c++11
CONFIG -= app_bundle
TARGET = sphere_reconnect_test
DEFINES += USE_Qt5
SOURCES += $$PWD/sphere_reconnect_test.cpp
INCLUDEPATH += $$PWD/../v3d_main/basic_c_fun
# Local debug build dependencies, when available.
exists($$PWD/../.local-deps/sysroot/usr/include) {
    INCLUDEPATH += $$PWD/../.local-deps/sysroot/usr/include
    LIBS += -L$$PWD/../.local-deps/sysroot/usr/lib/x86_64-linux-gnu
}
