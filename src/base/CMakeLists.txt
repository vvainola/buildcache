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

add_library(base
  compressor.cpp
  compressor.hpp
  debug_utils.cpp
  debug_utils.hpp
  file_utils.cpp
  file_utils.hpp
  hasher.cpp
  hasher.hpp
  lock_file.cpp
  lock_file.hpp
  serializer_utils.cpp
  serializer_utils.hpp
  string_list.hpp
  unicode_utils.cpp
  unicode_utils.hpp
  )
target_link_libraries(base md4 lz4)
if(WIN32 OR MINGW)
  target_link_libraries(base userenv)
endif()

buildcache_add_test(NAME lock_file_test
                    SOURCES lock_file_test.cpp
                    LIBRARIES base)

buildcache_add_test(NAME string_list_test
                    SOURCES string_list_test.cpp
                    LIBRARIES base)

# This is a special test executable. It is meant to be run as multiple concurrent instances.
add_executable(lock_file_stresstest
               lock_file_stresstest.cpp)
target_link_libraries(lock_file_stresstest base)
