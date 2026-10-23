CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native

.PHONY: all demo test
all: build/renderer

build:
	mkdir -p build

build/renderer: renderer.cpp | build
	$(CXX) $(CXXFLAGS) $< -ldl -pthread -o $@

demo: build/renderer
	python3 make-demo.py

build/test_shader_animation: tests/test_shader_animation.cpp shaders.cpp vectors.hpp xD.hpp | build
	$(CXX) $(CXXFLAGS) -fopenmp -ffast-math $< -o $@

test: build/renderer build/test_shader_animation
	./build/test_shader_animation
	python3 tests/test_hot_reload.py
