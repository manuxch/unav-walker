# Herramientas de análisis (ver README.md). Se compilan desde el
# CMakeLists.txt raíz; los ejecutables quedan en bin/.

find_package(Threads REQUIRED)

# -----------------------------------------------------------------------
# Biblioteca estática: dem_reader
# -----------------------------------------------------------------------
add_library(dem_reader STATIC
    dem_reader.cpp
)
target_include_directories(dem_reader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(dem_reader PRIVATE -Wall -Wextra)

# -----------------------------------------------------------------------
# Ejecutable de demostración / verificación
# -----------------------------------------------------------------------
add_executable(read_demo read_demo.cpp)
target_link_libraries(read_demo PRIVATE dem_reader)
target_compile_options(read_demo PRIVATE -Wall -Wextra)

# -----------------------------------------------------------------------
# Perfil de velocidad concurrente
# -----------------------------------------------------------------------
add_executable(vel_profile vel_profile.cpp)
target_link_libraries(vel_profile PRIVATE dem_reader Threads::Threads)
target_compile_options(vel_profile PRIVATE -Wall -Wextra)

# -----------------------------------------------------------------------
# Perfil de fuerza concurrente
# -----------------------------------------------------------------------
add_executable(force_profile force_profile.cpp)
target_link_libraries(force_profile PRIVATE dem_reader Threads::Threads)
target_compile_options(force_profile PRIVATE -Wall -Wextra)

# -----------------------------------------------------------------------
# Mapa de fuerza 2D concurrente
# -----------------------------------------------------------------------
add_executable(force_map2d force_map2d.cpp)
target_link_libraries(force_map2d PRIVATE dem_reader Threads::Threads)
target_compile_options(force_map2d PRIVATE -Wall -Wextra)

# -----------------------------------------------------------------------
# Perfil del tensor de estrés (archivos .sxy, simulador >= 3.0)
# -----------------------------------------------------------------------
add_executable(stress_profile stress_profile.cpp)
target_link_libraries(stress_profile PRIVATE dem_reader Threads::Threads)
target_compile_options(stress_profile PRIVATE -Wall -Wextra)
