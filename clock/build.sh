#!/bin/bash

g++ -std=c++17 clock.cpp -o clock_widget     `pkg-config --cflags --libs gtkmm-3.0 gtk-layer-shell-0 libpulse`
