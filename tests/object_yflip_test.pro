QT += widgets
CONFIG += console c++11
CONFIG -= app_bundle
TARGET = object_yflip_test
DEFINES += USE_Qt5
SOURCES += $$PWD/object_yflip_test.cpp
LIBS += -lpthread
exists($$PWD/../.local-deps/sysroot/usr/include) {
    INCLUDEPATH += $$PWD/../.local-deps/sysroot/usr/include
    LIBS += -L$$PWD/../.local-deps/sysroot/usr/lib/x86_64-linux-gnu
}
