cmake_minimum_required(VERSION 3.21)
project(render)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(SOURCE
	main.cpp
	gui.cpp
)

set(SOURCE_BENCHMARK
	benchmark.cpp
	gui.cpp
)

set(SOURCE_PPG
	ppg.cpp
)

find_package(OpenGL REQUIRED)
find_package(OpenMP REQUIRED)

file (GLOB SOURCE_IMGUI "libs/imgui/*.cpp")
file (GLOB SOURCE_TINYEXR "libs/tinyexr/*.c")
file (GLOB SOURCE_TINYPARSER "libs/tinyparser/*.cpp")
include_directories(libs/imgui)
include_directories(libs/tinyexr)
include_directories(libs/tinyparser)

add_subdirectory(libs/glm)
add_subdirectory(libs/glfw)

include_directories(include)
add_executable(render ${SOURCE} ${SOURCE_IMGUI} ${SOURCE_TINYEXR} ${SOURCE_TINYPARSER})
add_executable(benchmark ${SOURCE_BENCHMARK} ${SOURCE_IMGUI} ${SOURCE_TINYEXR} ${SOURCE_TINYPARSER})
add_executable(ppg  ${SOURCE_PPG} ${SOURCE_TINYEXR} ${SOURCE_TINYPARSER})

target_link_libraries(render     OpenMP::OpenMP_CXX glm glfw ${OPENGL_LIBRARIES})
target_link_libraries(benchmark  OpenMP::OpenMP_CXX glm glfw ${OPENGL_LIBRARIES})
target_link_libraries(ppg        OpenMP::OpenMP_CXX glm glfw ${OPENGL_LIBRARIES})

# specify the C++ standard
# -Wno-strict-overflow disables imgui.h strange warnings
set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -O2 -g -Wno-strict-overflow")
