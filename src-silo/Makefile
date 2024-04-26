#!/usr/bin/bash
# \file  Makefile
# \brief Makefile
# \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar> 
# \versión 0.1 
# \date 2023.12.05
# \note Ver: http://makepp.sourceforge.net/1.19/makepp_tutorial.html


#CXX = clang
CXX = g++
LIBS = -lbox2d 
CXXFLAGS = -O3 -std=c++20
LDFLAGS = -O3 -Wall -std=c++20 -static $(LIBS)

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
 
