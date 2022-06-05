# file misc-basile/glfw/Makefile
CXX=g++
CXXFLAGS= -O -g
.PHONY: all clean
all: glfw-test
clean:
	/bin/rm -vf *.o glfw-test *~ src/*~
glfw-test: src/glad.c main.c
	$(CXX) $(CXXFLAGS) src/glad.c main.cpp -o glfw-test -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl
