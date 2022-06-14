cmake_minimum_required(VERSION 3.15)
project(hasher)

set(CMAKE_CXX_STANDARD 20)

# required for clang completion in editors - must be set after creating project
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3")

add_subdirectory(3rdParty)

add_executable(hasher
  src/hasher.cpp
)

target_include_directories(hasher
  PUBLIC
  3rdParty/velocypack/include
  3rdParty/xxhash
)

target_link_libraries(hasher
  velocypack
  ${CMAKE_THREAD_LIBS_INIT}
)

