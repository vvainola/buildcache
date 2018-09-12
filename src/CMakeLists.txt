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

# We build everything against the C++11 standard.
set(CMAKE_CXX_STANDARD 11)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MINGW)
  # For ease of deployment, statically link the standard libraries when using MinGW.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
endif()

# Thrid party libraries used by BuildCache.
add_subdirectory(third_party)

# For the project internal files, all includes are relative to the root folder.
include_directories(.)

# BuildCache internal libraries.
add_subdirectory(base)
add_subdirectory(config)
add_subdirectory(sys)
add_subdirectory(wrappers)

# The BuildCache application.
add_executable(buildcache
  main.cpp
  )
target_link_libraries(buildcache base config sys wrappers)

install(
  TARGETS buildcache
  RUNTIME DESTINATION bin)
