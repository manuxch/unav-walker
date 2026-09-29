# Simulador unav-walkers (ver README.md).

# ---------------------------------------------------------------------------
# Box2D 2.4.2
# ---------------------------------------------------------------------------
set(BOX2D_ROOT "/home/manuel/Tools/box2d/2-4-2" CACHE PATH
    "Instalación de Box2D 2.4.2 (con include/ y build/bin/libbox2d.a)")
find_library(BOX2D_LIBRARY box2d
             PATHS ${BOX2D_ROOT}/build/bin ${BOX2D_ROOT}/lib
             NO_DEFAULT_PATH)
if(NOT BOX2D_LIBRARY)
  message(FATAL_ERROR "No se encontró libbox2d en ${BOX2D_ROOT}. "
                      "Indique la ruta con -DBOX2D_ROOT=/ruta/a/box2d")
endif()
add_library(box2d STATIC IMPORTED)
set_target_properties(box2d PROPERTIES
  IMPORTED_LOCATION ${BOX2D_LIBRARY}
  INTERFACE_INCLUDE_DIRECTORIES ${BOX2D_ROOT}/include)

option(UNAV_STATIC "Enlazar unav-walkers estáticamente" ON)

# ---------------------------------------------------------------------------
# Versión del código: git_version.hpp define GIT_HASH (hash de git, con
# sufijo -dirty si hay cambios sin commit). Se regenera en cada compilación,
# pero solo se reescribe si cambia.
# ---------------------------------------------------------------------------
set(GIT_VERSION_HPP ${CMAKE_CURRENT_BINARY_DIR}/git_version.hpp)
add_custom_target(git_version
  COMMAND ${CMAKE_COMMAND} -DSRC_DIR=${PROJECT_SOURCE_DIR}
          -DOUT_FILE=${GIT_VERSION_HPP}
          -P ${PROJECT_SOURCE_DIR}/cmake/GitVersion.cmake
  BYPRODUCTS ${GIT_VERSION_HPP}
  COMMENT "Versión de git"
  VERBATIM)

# ---------------------------------------------------------------------------
# Módulos del simulador (biblioteca) y ejecutable
# ---------------------------------------------------------------------------
add_library(sim_core STATIC
  base_friction.cpp
  contacts.cpp
  diagnostics.cpp
  discharge.cpp
  global_setup.cpp
  output.cpp
  rng.cpp
  silo_builder.cpp)
target_include_directories(sim_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                    PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(sim_core PUBLIC box2d)
target_compile_options(sim_core PRIVATE -Wall -Wextra)
add_dependencies(sim_core git_version)

add_executable(unav-walkers main.cpp)
target_include_directories(unav-walkers PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(unav-walkers PRIVATE sim_core)
target_compile_options(unav-walkers PRIVATE -Wall -Wextra)
add_dependencies(unav-walkers git_version)
if(UNAV_STATIC)
  target_link_options(unav-walkers PRIVATE -static)
endif()

# ---------------------------------------------------------------------------
# Pruebas (ctest). Los ejecutables de prueba quedan en el directorio de
# compilación, no en bin/.
# ---------------------------------------------------------------------------
foreach(t test_friction test_reinjection)
  add_executable(${t} tests/${t}.cpp)
  target_link_libraries(${t} PRIVATE sim_core)
  target_compile_options(${t} PRIVATE -Wall -Wextra)
  set_target_properties(${t} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests)
endforeach()

add_test(NAME friccion_con_la_base COMMAND test_friction)
add_test(NAME reinyeccion
         COMMAND test_reinjection ${CMAKE_CURRENT_SOURCE_DIR}/tests/params_test.in)
add_test(NAME lector_de_parametros
         COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_params.sh
                 $<TARGET_FILE:unav-walkers>)
