//--------------------------------------------------------------------------------------------------
// Copyright (c) 2021 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the
// authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote
//     the original software. If you use this software in a product, an acknowledgment in the
//     product documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//     being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Purpose:  This is the Doxygen main page. The file is not actually used by the BuildCache source.
//--------------------------------------------------------------------------------------------------

#ifndef BUILDCACHE_DOC_MAINPAGE_HPP_
#define BUILDCACHE_DOC_MAINPAGE_HPP_

/// @mainpage BuildCache internals documentation
///
/// @section intro_sec Introduction
///
/// This documentation describes the BuildCache internals. It is mainly useful for BuildCache
/// contributors.
///
/// @section wrapper_sec Creating a new program wrapper
///
/// In order to add support for a new compiler or program, create a new class that inherits from the
/// @ref bcache::program_wrapper_t class.
///
/// @section structur_sec Code structure
///
/// The BuildCache source code is divided into the following parts:
///   - base - A portable set of support classes and functions.
///   - sys - System specific support functions.
///   - cache - The implementation of the cache logic (wrapper independent).
///   - wrappers - The wrappers for different compilers and programs.
///   - config - Program configuration logic.
///   - third_party - Third party libraries.
///
/// @section namespaces_sec Namespaces
///
/// The following namespaces are used in BuildCache:
///   - @ref bcache
///   - @ref bcache::comp
///   - @ref bcache::config
///   - @ref bcache::debug
///   - @ref bcache::file
///   - @ref bcache::perf
///   - @ref bcache::serialize
///   - @ref bcache::sys
///   - @ref bcache::time

/// @namespace bcache
/// @brief The top level namespace.
///
/// All BuildCache functionality lives in the @c bcache namespace or one of its decendant
/// namespaces.

/// @namespace bcache::config
/// @brief BuildCachce configuration options.
///
/// Configuration options are gathered during program startup, and they can be queried via the
/// functions in the @c config namespace (which acts as a sigleton).

/// @namespace bcache::comp
/// @brief Compression functions.

/// @namespace bcache::debug
/// @brief Debug logging functions.

/// @namespace bcache::file
/// @brief File utility functions and classes.

/// @namespace bcache::perf
/// @brief Performance profiling functions.

/// @namespace bcache::serialize
/// @brief Data (de)serialization functions.

/// @namespace bcache::sys
/// @brief System utility functions.

/// @namespace bcache::time
/// @brief Portable file time types and functions.

#endif  // BUILDCACHE_DOC_MAINPAGE_HPP_
