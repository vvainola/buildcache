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

# Disable all warnings for third party code.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
endif()

add_library(cjson
  cjson/cJSON.c
  )
target_include_directories(cjson INTERFACE .)

add_library(md4
  md4/md4.c
  )
target_include_directories(md4 INTERFACE .)

add_library(lz4
  lz4/lz4.c
  )
target_include_directories(lz4 INTERFACE .)

set(LUA_DIR lua-5.3.4)
add_library(lua
  ${LUA_DIR}/src/lapi.c
  ${LUA_DIR}/src/lcode.c
  ${LUA_DIR}/src/lctype.c
  ${LUA_DIR}/src/ldebug.c
  ${LUA_DIR}/src/ldo.c
  ${LUA_DIR}/src/ldump.c
  ${LUA_DIR}/src/lfunc.c
  ${LUA_DIR}/src/lgc.c
  ${LUA_DIR}/src/llex.c
  ${LUA_DIR}/src/lmem.c
  ${LUA_DIR}/src/lobject.c
  ${LUA_DIR}/src/lopcodes.c
  ${LUA_DIR}/src/lparser.c
  ${LUA_DIR}/src/lstate.c
  ${LUA_DIR}/src/lstring.c
  ${LUA_DIR}/src/ltable.c
  ${LUA_DIR}/src/ltm.c
  ${LUA_DIR}/src/lundump.c
  ${LUA_DIR}/src/lvm.c
  ${LUA_DIR}/src/lzio.c
  ${LUA_DIR}/src/lauxlib.c
  ${LUA_DIR}/src/lbaselib.c
  ${LUA_DIR}/src/lbitlib.c
  ${LUA_DIR}/src/lcorolib.c
  ${LUA_DIR}/src/ldblib.c
  ${LUA_DIR}/src/liolib.c
  ${LUA_DIR}/src/lmathlib.c
  ${LUA_DIR}/src/loslib.c
  ${LUA_DIR}/src/lstrlib.c
  ${LUA_DIR}/src/ltablib.c
  ${LUA_DIR}/src/lutf8lib.c
  ${LUA_DIR}/src/loadlib.c
  ${LUA_DIR}/src/linit.c
  )
target_include_directories(lua PUBLIC ${LUA_DIR}/src)

if(APPLE)
  target_compile_definitions(lua PRIVATE LUA_USE_MACOSX)
  target_link_libraries(lua ${CMAKE_DL_LIBS})
elseif(UNIX AND NOT (WIN32 OR MINGW))
  target_compile_definitions(lua PRIVATE LUA_USE_LINUX)
  target_link_libraries(lua ${CMAKE_DL_LIBS})
endif()
