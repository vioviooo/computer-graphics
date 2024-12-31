TEMPLATE = app
TARGET = qt_example

INCLUDEPATH += . /opt/homebrew/include

QT += core gui widgets opengl

SOURCES += main.cpp

LIBS += -L/opt/homebrew/lib -lglfw -framework OpenGL