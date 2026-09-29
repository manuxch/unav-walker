#!/usr/bin/bash
# \file  Makefile
# \brief Makefile
# \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar> 
# \versión 0.1 
# \date 2023.12.05
# \note Ver: http://makepp.sourceforge.net/1.19/makepp_tutorial.html


#CXX = clang
CXX = g++
BOX2D = /home/manuel/Tools/box2d/2-4-2
LIBPATH = -L$(BOX2D)/build/bin
INCLUDES = -I$(BOX2D)/include/
LIBS = -lbox2d 
# Opciones para debuggear 
# CXXFLAGS = -fsanitize=address -g -std=c++20 $(INCLUDES)
# LDFLAGS = -fsanitize=address -g -Wall -std=c++20 $(LIBPATH) $(LIBS)
# LDFLAGS = -g -Wall -std=c++20 -static $(LIBPATH) $(LIBS)
# Opciones para correr en producción
# Versión del código (hash de git, con sufijo -dirty si hay cambios sin commit)
GIT_HASH := $(shell git describe --always --dirty 2>/dev/null || echo desconocido)
CXXFLAGS = -O3 -Wall -Wextra -std=c++20 $(INCLUDES) -DGIT_HASH=\"$(GIT_HASH)\"
LDFLAGS = -O3 -Wall -std=c++20 -static $(LIBPATH) $(LIBS)

SOURCES = $(wildcard *.cpp)

#OBJS = $(SOURCES:%.cpp=%.o)
OBJS1 = globalsetup.o rng.o siloAux.o silo-vib.o

# El silo cerrado con fondo de medición (antes walkers-forceWall) se obtiene
# con "fondo_medicion: T" en el archivo de parámetros.
all : silo-vib

silo-vib : $(OBJS1)
	$(CXX) -o $@ $^ $(LDFLAGS)

# silo-vib.o y siloAux.o dependen del hash de git: se recompilan siempre
silo-vib.o siloAux.o : FORCE
FORCE:

%.o : %.cpp globalsetup.hpp siloAux.hpp rng.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o *~ 
 
