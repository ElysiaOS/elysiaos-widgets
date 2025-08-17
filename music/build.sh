#!/bin/bash

g++ -std=c++17 -lcurl -o music_widget music_widget.cpp `pkg-config --cflags --libs gtkmm-3.0 gtk+-3.0 gtk-layer-shell-0 libpulse`

