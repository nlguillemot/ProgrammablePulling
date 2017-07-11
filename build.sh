#!/bin/sh
c++ -o ProgrammablePulling/progpulling  -I./include -std=c++11 ProgrammablePulling/*.cpp ProgrammablePulling/imgui/*.cpp -lglfw -lm -lGL -lGLEW -lstdc++
