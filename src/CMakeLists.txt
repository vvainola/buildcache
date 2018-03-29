cmake_minimum_required(VERSION 3.5)
project(BuildCache)

set(CMAKE_CXX_STANDARD 11)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(third_party)

add_executable(buildcache
  arg_list.hpp
  hasher.hpp
  main.cpp
  )
target_link_libraries(buildcache md4)

