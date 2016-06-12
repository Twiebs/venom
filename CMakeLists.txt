cmake_minimum_required(VERSION 3.5)
project(venom)

set(CMAKE_CXX_COMPILER  clang++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(SOURCE_FILES src/venom_platform.cpp)
add_executable(venom ${SOURCE_FILES})
target_link_libraries(venom dl assimp X11)



