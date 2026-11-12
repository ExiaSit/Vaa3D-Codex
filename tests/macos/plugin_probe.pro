TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
QT += widgets
SOURCES += $$PWD/plugin_probe.cpp
INCLUDEPATH += $$PWD/../../v3d_main/basic_c_fun
TARGET = plugin-probe
