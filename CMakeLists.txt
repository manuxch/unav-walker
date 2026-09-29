# unav-walker: simulador (src/) y herramientas de análisis (tools/).
#
#   cmake -S . -B build                  # configuración (Release por defecto)
#   cmake --build build -j               # compila; ejecutables en bin/
#   ctest --test-dir build               # pruebas del simulador
#   cmake --build build --target doc     # documentación (Doxygen) en build/doc/
#
# Opciones:
#   -DBOX2D_ROOT=/ruta     instalación de Box2D 2.4.2 (include/ y build/bin/)
#   -DUNAV_STATIC=OFF      enlace dinámico del simulador (por defecto estático)

cmake_minimum_required(VERSION 3.20)
project(unav_walker CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de compilación" FORCE)
endif()

# Todos los ejecutables (simulador y herramientas) quedan en bin/, sin
# seguimiento en el repositorio.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

enable_testing()

add_subdirectory(src)
add_subdirectory(tools)

# Documentación con Doxygen (opcional): cmake --build build --target doc
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
  configure_file(${PROJECT_SOURCE_DIR}/Doxyfile.in
                 ${PROJECT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generando la documentación en ${PROJECT_BINARY_DIR}/doc/html"
    VERBATIM)
endif()
