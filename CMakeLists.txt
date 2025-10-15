cmake_minimum_required(VERSION 3.15)
project(EraserStatic)
set(CMAKE_CXX_STANDARD 17)

# Your sources
add_executable(eraser_static
  eraser_static/src/main.cpp
  eraser_static/src/parser.cpp
)

# Your public headers (so #include "eraser_static/parser.h" works)
target_include_directories(eraser_static PRIVATE
  ${CMAKE_SOURCE_DIR}/eraser_static/include
  ${CMAKE_SOURCE_DIR}/eraser_static/graph_nodes/include
)

# ---- libclang (Ubuntu LLVM 18) ----
# On Ubuntu WSL with llvm-18 & libclang-18-dev the paths are:
#   Headers:  /usr/lib/llvm-18/include/clang-c/Index.h
#   Library:  /usr/lib/llvm-18/lib/libclang.so

target_include_directories(eraser_static PRIVATE /usr/lib/llvm-18/include)
target_link_libraries(eraser_static PRIVATE /usr/lib/llvm-18/lib/libclang.so)

# Make sure the runtime loader can find libclang when you run the binary
set_target_properties(eraser_static PROPERTIES
  BUILD_RPATH "${LIBCLANG_ROOT}/lib"
)

# Warnings
if (MSVC)
  target_compile_options(eraser_static PRIVATE /W4 /EHsc)
else()
  target_compile_options(eraser_static PRIVATE -Wall -Wextra -Wpedantic)
endif()