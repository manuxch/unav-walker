#!/usr/bin/bash
# \file  Makefile
# \brief Makefile
# \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar> 
# \versión 0.1 
# \date 2023.12.05
# \note Ver: http://makepp.sourceforge.net/1.19/makepp_tutorial.html


#CXX = clang
CXX = g++
BOX2D = /home/manuel/Tools/box2d/2-4-1
LIBPATH = -L$(BOX2D)/build/bin
INCLUDES = -I$(BOX2D)/include/
LIBS = -lbox2d 
#CXXFLAGS = -g -std=c++20 $(INCLUDES)
#LDFLAGS = -g -Wall -std=c++20 -static $(LIBPATH) $(LIBS)
CXXFLAGS = -O3 -std=c++20 $(INCLUDES)
LDFLAGS = -O3 -Wall -std=c++20 -static $(LIBPATH) $(LIBS)

SOURCES = $(wildcard *.cpp)

#OBJS = $(SOURCES:%.cpp=%.o)
OBJS1 = globalsetup.o rng.o siloAux.o silo-vib.o 

all : silo-vib

silo-vib : $(OBJS1)
	$(CXX) -o $@ $^ $(LDFLAGS) 

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $< -o $@

clean:
	rm -f *.o *~ 
 
