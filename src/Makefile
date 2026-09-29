# Makefile del simulador silo-vib.
#
#   make          compila silo-vib
#   make test     compila y corre las pruebas (tests/)
#   make clean    borra objetos y ejecutables
#
# La ruta de Box2D 2.4.2 se puede cambiar sin editar el archivo:
#   make BOX2D=/ruta/a/box2d
#
# Autor: Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>

CXX = g++
BOX2D ?= /home/manuel/Tools/box2d/2-4-2
LIBPATH = -L$(BOX2D)/build/bin
INCLUDES = -I$(BOX2D)/include/
LIBS = -lbox2d

# Versión del código (hash de git, con sufijo -dirty si hay cambios sin
# commit). Se escribe en la salida estándar y en la cabecera de los archivos.
GIT_HASH := $(shell git describe --always --dirty 2>/dev/null || echo desconocido)

# Producción. Para depurar: CXXFLAGS = -g -O0 -fsanitize=address ... y
# LDFLAGS = -fsanitize=address (sin -static).
CXXFLAGS = -O3 -Wall -Wextra -std=c++20 $(INCLUDES) -DGIT_HASH=\"$(GIT_HASH)\"
LDFLAGS = -O3 -static $(LIBPATH) $(LIBS)

SOURCES = base_friction.cpp contacts.cpp diagnostics.cpp discharge.cpp \
          global_setup.cpp output.cpp rng.cpp silo_builder.cpp main.cpp
OBJS = $(SOURCES:.cpp=.o)
HEADERS = $(wildcard *.hpp)

all: silo-vib

silo-vib: $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# main.o y output.o incluyen el hash de git: se recompilan siempre.
main.o output.o: FORCE
FORCE:

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Pruebas: se enlazan con todos los módulos salvo main.o.
TEST_BINS = tests/test_friction tests/test_reinjection
LIB_OBJS = $(filter-out main.o,$(OBJS))

tests/test_%: tests/test_%.cpp $(LIB_OBJS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -I. $< $(LIB_OBJS) -o $@ $(LDFLAGS)

test: silo-vib $(TEST_BINS)
	@echo "== Fricción con la base (karnopp, pivot_friction)"
	@./tests/test_friction
	@echo "== Reinyección de granos"
	@./tests/test_reinjection tests/params_test.in > tests/reinjection.log; \
	  st=$$?; grep -v '^#' tests/reinjection.log; exit $$st
	@echo "== Lector de parámetros"
	@bash tests/test_params.sh ./silo-vib

clean:
	rm -f *.o *~ silo-vib $(TEST_BINS) tests/reinjection.log

.PHONY: all clean test FORCE
