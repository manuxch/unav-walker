# Genera OUT_FILE con la versión del código (hash de git):
#   #define GIT_HASH "<hash>[-dirty]"
# Uso: cmake -DSRC_DIR=<repo> -DOUT_FILE=<archivo> -P GitVersion.cmake
# El archivo solo se reescribe si el contenido cambia, para no forzar
# recompilaciones innecesarias.

execute_process(
  COMMAND git describe --always --dirty
  WORKING_DIRECTORY ${SRC_DIR}
  OUTPUT_VARIABLE hash
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE result
  ERROR_QUIET)
if(NOT result EQUAL 0 OR hash STREQUAL "")
  set(hash "desconocido")
endif()

set(content "// Generado por cmake/GitVersion.cmake: no editar.\n#pragma once\n#define GIT_HASH \"${hash}\"\n")
set(old "")
if(EXISTS ${OUT_FILE})
  file(READ ${OUT_FILE} old)
endif()
if(NOT old STREQUAL content)
  file(WRITE ${OUT_FILE} "${content}")
endif()
