#!/bin/bash

g++ -std=c++17 visualizer-dark.cpp -o visualizer-dark     `pkg-config --cflags --libs gtkmm-3.0 gtk-layer-shell-0 libpulse`

g++ -std=c++17 visualizer.cpp -o visualizer     `pkg-config --cflags --libs gtkmm-3.0 gtk-layer-shell-0 libpulse`
