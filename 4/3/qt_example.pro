TEMPLATE = app
TARGET = qt_example
INCLUDEPATH += .

QT += core gui widgets
QT += openglwidgets

LIBS += -L/opt/homebrew/lib -lglfw -framework OpenGL

SOURCES += main.cpp