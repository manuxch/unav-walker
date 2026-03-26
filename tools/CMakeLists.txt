cmake_minimum_required(VERSION 3.20)
project(dem_tools CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# -----------------------------------------------------------------------
# Biblioteca estática: dem_reader
# -----------------------------------------------------------------------
add_library(dem_reader STATIC
    dem_reader.cpp
)
target_include_directories(dem_reader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(dem_reader PRIVATE -Wall -Wextra -O2)

# -----------------------------------------------------------------------
# Ejecutable de demostración / verificación
# -----------------------------------------------------------------------
add_executable(read_demo read_demo.cpp)
target_link_libraries(read_demo PRIVATE dem_reader)
target_compile_options(read_demo PRIVATE -Wall -Wextra -O2)

# -----------------------------------------------------------------------
# Perfil de velocidad concurrente
# -----------------------------------------------------------------------
find_package(Threads REQUIRED)

add_executable(vel_profile vel_profile.cpp)
target_link_libraries(vel_profile PRIVATE dem_reader Threads::Threads)
target_compile_options(vel_profile PRIVATE -Wall -Wextra -O2)

# -----------------------------------------------------------------------
# Perfil de fuerza concurrente
# -----------------------------------------------------------------------
add_executable(force_profile force_profile.cpp)
target_link_libraries(force_profile PRIVATE dem_reader Threads::Threads)
target_compile_options(force_profile PRIVATE -Wall -Wextra -O2)
