#!/bin/bash

g++ -std=c++17 about_system.cpp -o about_system     `pkg-config --cflags --libs gtkmm-3.0 gtk-layer-shell-0 libpulse`

g++ -std=c++17 keybinds.cpp -o ElysiaOSKeybinds     `pkg-config --cflags --libs gtkmm-3.0 gtk-layer-shell-0 libpulse`
