# file misc-basile/glfw/Makefile
CXX=g++
CXXFLAGS= -O -g
CC=gcc
CFLAGS= -O -g
.PHONY: all clean
all: glfw-test glfw-testxx
clean:
	/bin/rm -vf *.o glfw-test glfw-testxx *~ src/*~
glfw-test: src/glad.c main.c
	$(CC) $(CFLAGS) src/glad.c main.c -o $@ -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl
glfw-testxx: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp src/glad.c -o $@ -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl
