# COMMANDS:
# cmake --build .
# cmake

cmake_minimum_required(VERSION 3.15)
project(EraserStatic)
set(CMAKE_CXX_STANDARD 20)

# Your sources
add_executable(eraser_static
  eraser_static/src/main.cpp
  eraser_static/src/parser.cpp
  eraser_static/src/graph_visualizer.cpp
  eraser_static/src/call_graph.cpp
  eraser_static/src/lockset.cpp
  eraser_static/src/file_includes.cpp
  eraser_static/src/parser_helpers.cpp
  eraser_static/src/construction_environment.cpp
  eraser_static/graph_nodes/src/basic_node.cpp
  eraser_static/graph_nodes/src/break_node.cpp
  eraser_static/graph_nodes/src/continue_node.cpp
  eraser_static/graph_nodes/src/continue_return_node.cpp
  eraser_static/graph_nodes/src/endif_node.cpp
  eraser_static/graph_nodes/src/endwhile_node.cpp
  eraser_static/graph_nodes/src/eraser_ignore_on_node.cpp
  eraser_static/graph_nodes/src/eraser_ignore_off_node.cpp
  eraser_static/graph_nodes/src/function_call_node.cpp
  eraser_static/graph_nodes/src/graph_node.cpp
  eraser_static/graph_nodes/src/if_node.cpp
  eraser_static/graph_nodes/src/lock_node.cpp
  eraser_static/graph_nodes/src/read_node.cpp
  eraser_static/graph_nodes/src/return_node.cpp
  eraser_static/graph_nodes/src/start_node.cpp
  eraser_static/graph_nodes/src/startwhile_node.cpp
  eraser_static/graph_nodes/src/thread_create_node.cpp
  eraser_static/graph_nodes/src/thread_join_node.cpp
  eraser_static/graph_nodes/src/unlock_node.cpp
  eraser_static/graph_nodes/src/while_node.cpp
  eraser_static/graph_nodes/src/write_node.cpp
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
target_compile_options(eraser_static PRIVATE
$<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic -g>
)
endif()