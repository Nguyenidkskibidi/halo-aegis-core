cmake_minimum_required(VERSION 3.20)
project(halo_aegis_core VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Interface Library (Header-Only Core)
add_library(halo_aegis_core INTERFACE)
target_include_directories(halo_aegis_core INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Common Compiler & Linker Options for High Performance
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
  # Host architecture flag: only apply -march=native when not cross-compiling for embedded targets
  set(HALO_ARCH_FLAGS "")
  if (NOT CMAKE_CROSSCOMPILING AND NOT HALO_TARGET_ESP32)
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|arm64|aarch64")
      set(HALO_ARCH_FLAGS "-march=native")
    endif()
  endif()

  target_compile_options(halo_aegis_core INTERFACE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Release>:-O3 ${HALO_ARCH_FLAGS} -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions>
  )
  if (APPLE)
    target_link_options(halo_aegis_core INTERFACE
      $<$<CONFIG:Release>:-Wl,-dead_strip>
    )
  else()
    target_link_options(halo_aegis_core INTERFACE
      $<$<CONFIG:Release>:-Wl,--gc-sections>
    )
  endif()
endif()

# Build Benchmarks & Verification Suites
option(HALO_BUILD_BENCHMARKS "Build benchmark and verification executables" ON)
option(HALO_BUILD_EXAMPLES "Build example executables" ON)

if (HALO_BUILD_BENCHMARKS)
  add_executable(halo_benchmark tests/halo_benchmark.cpp)
  target_link_libraries(halo_benchmark PRIVATE halo_aegis_core)

  add_executable(halo_dynamic_flight_benchmark tests/halo_dynamic_flight_benchmark.cpp)
  target_link_libraries(halo_dynamic_flight_benchmark PRIVATE halo_aegis_core)

  add_executable(halo_game_universal_benchmark tests/halo_game_universal_benchmark.cpp)
  target_link_libraries(halo_game_universal_benchmark PRIVATE halo_aegis_core)

  add_executable(halo_universal_spatial_benchmark tests/halo_universal_spatial_benchmark.cpp)
  target_link_libraries(halo_universal_spatial_benchmark PRIVATE halo_aegis_core)

  add_executable(halo_embedded_test tests/halo_embedded_test.cpp)
  target_link_libraries(halo_embedded_test PRIVATE halo_aegis_core)
endif()

if (HALO_BUILD_EXAMPLES)
  add_executable(halo_example examples/main.cpp)
  target_link_libraries(halo_example PRIVATE halo_aegis_core)
endif()
