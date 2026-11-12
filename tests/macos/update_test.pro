TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
QT += widgets network
QMAKE_CXXFLAGS += -UNDEBUG
SOURCES += $$PWD/update_test.cpp
TARGET = update-test
