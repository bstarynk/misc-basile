# file misc-basile/glfw/Makefile
CC=g++
CFLAGS= -O -g
.PHONY: all clean
all: glfw-test
clean:
	/bin/rm -vf *.o glfw-test *~ src/*~
glfw-test: src/glad.c main.c
	$(CC) $(CFLAGS) src/glad.c main.cpp -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl
