QT += widgets
CONFIG += console c++11
CONFIG -= app_bundle
TARGET = fast_file_dialog_test
DEFINES += USE_Qt5
SOURCES += $$PWD/fast_file_dialog_test.cpp
LIBS += -lpthread
exists($$PWD/../.local-deps/sysroot/usr/include) {
    INCLUDEPATH += $$PWD/../.local-deps/sysroot/usr/include
    LIBS += -L$$PWD/../.local-deps/sysroot/usr/lib/x86_64-linux-gnu
}
