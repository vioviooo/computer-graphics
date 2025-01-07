#!/bin/bash

qmake qt_example.pro

make

cd qt_example.app/Contents/MacOS

./qt_example