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
LIBS = -lbox2d -lm
CXXFLAGS = -O3 $(INCLUDES) -std=c++20
LDFLAGS = -O3 -Wall -std=c++20 $(LIBPATH) -static $(LIBS)

SOURCES = $(wildcard *.cpp)

#OBJS = $(SOURCES:%.cpp=%.o)
OBJS1 = globalsetup.o simulacion.o rng.o

all : test

test : $(OBJS1)
	$(CXX) -o $@ $^ $(LDFLAGS) 

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $< -o $@

clean:
	rm -f *.o *~ 
 
