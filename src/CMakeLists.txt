#---------------------------------------------------------------------------------------------------
# Copyright (c) 2018 Marcus Geelnard
#
# This software is provided 'as-is', without any express or implied warranty. In no event will the
# authors be held liable for any damages arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose, including commercial
# applications, and to alter it and redistribute it freely, subject to the following restrictions:
#
#  1. The origin of this software must not be misrepresented; you must not claim that you wrote
#     the original software. If you use this software in a product, an acknowledgment in the
#     product documentation would be appreciated but is not required.
#
#  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
#     being the original software.
#
#  3. This notice may not be removed or altered from any source distribution.
#---------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.5)
project(BuildCache)

set(CMAKE_CXX_STANDARD 11)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MINGW)
  # For ease of deployment, statically link the standard libraries.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
endif()

add_subdirectory(third_party)

add_executable(buildcache
  cache.cpp
  cache.hpp
  configuration.cpp
  configuration.hpp
  debug_utils.cpp
  debug_utils.hpp
  file_utils.cpp
  file_utils.hpp
  gcc_wrapper.cpp
  gcc_wrapper.hpp
  ghs_wrapper.cpp
  ghs_wrapper.hpp
  hasher.cpp
  hasher.hpp
  lua_wrapper.cpp
  lua_wrapper.hpp
  main.cpp
  msvc_wrapper.cpp
  msvc_wrapper.hpp
  perf_utils.cpp
  perf_utils.hpp
  program_wrapper.cpp
  program_wrapper.hpp
  serializer_utils.cpp
  serializer_utils.hpp
  string_list.hpp
  sys_utils.cpp
  sys_utils.hpp
  unicode_utils.cpp
  unicode_utils.hpp
  )
target_link_libraries(buildcache md4 lua)

if(WIN32 OR MINGW)
  target_link_libraries(buildcache userenv)
endif()

install(
  TARGETS buildcache
  RUNTIME DESTINATION bin)
