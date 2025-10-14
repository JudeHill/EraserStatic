cmake_minimum_required(VERSION 3.24)
project(EraserStatic VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# If LLVM is installed with CMake config files:
find_package(LLVM QUIET CONFIG)
if(LLVM_FOUND)
  message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
  include_directories(${LLVM_INCLUDE_DIRS})
  add_definitions(${LLVM_DEFINITIONS})
else()
  message(WARNING "LLVM not found. AST/CFG frontend will be stubbed for now.")
endif()

add_library(eraser_static
  src/facts.cpp src/transfer.cpp src/analysis.cpp src/barriers.cpp src/reporting.cpp)
target_include_directories(eraser_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(eraser_static PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(eraser_static PRIVATE /W4 /permissive-)
else()
  target_compile_options(eraser_static PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_subdirectory(tools/eraser_cli)
enable_testing()
add_subdirectory(tests)
