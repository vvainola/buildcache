# BuildCache [![Build Status](https://travis-ci.org/mbitsnbites/buildcache.svg?branch=master)](https://travis-ci.org/mbitsnbites/buildcache)

This is a simple compiler accelerator that caches and reuses build results to
avoid unnecessary re-compilations, and thereby speeding up the build process.

It is similar in spirit to [ccache](https://ccache.samba.org/),
[sccache](https://github.com/mozilla/sccache) and
[clcache](https://github.com/frerich/clcache).

## Features

* Works on different operating systems:
  * Linux
  * macOS
  * Windows
  * Though untested, it probably works on most BSD:s
* A modular compiler support system:
  * Built-in support for [popular compilers](#supported-compilers-and-languages).
  * Extensible via custom [Lua](https://www.lua.org/) scripts.
  * In addition to caching compilation results, BuildCache can be used for
    caching almost any reproducible program artifacts (e.g. test results,
    [rendered images](https://en.wikipedia.org/wiki/Rendering_(computer_graphics)),
    etc).
* A fast local file system cache.
* Can optionally use a remote, shared server as a second level cache.
* Optional compression with [LZ4](https://github.com/lz4/lz4) or
  [ZSTD](https://github.com/facebook/zstd) (almost negligable overhead).


### Supported compilers and languages

Currently the following compilers and languages are supported:

| Compiler | Languages | Support |
| --- | --- | --- |
| GCC | C, C++ | Built-in |
| Clang | C, C++ | Built-in |
| Microsoft Visual C++ | C, C++ | Built-in |
| Green Hills Software | C, C++ | Built-in |
| Texas Instruments TMS320C6000™ | C, C++ | Built-in |
| scan-build static analyzer | C, C++ | Built-in |

New backends are relatively easy to add, both as built-in wrappers in C++ and as
[Lua wrappers](doc/lua.md).

## Status

BuildCache has been used daily in production environments for years with near
zero issues (any problem that has emereged has of course been fixed), which
gives it a good track record.

With that said, BuildCache is still considered to be under development.

## Building

Use [CMake](https://cmake.org/) and your favorite C++ compiler to build the
BuildCache program:

```bash
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ../src
$ cmake --build .
```

Note: For S3 support on non-macOS/Windows systems you need OpenSSL (e.g. install
`libssl-dev` on Ubuntu before running CMake).

## Usage

See [Using BuildCache](doc/usage.md).

## Configuration

See [Configruation](doc/configuration.md).

## Using custom Lua plugins

See [Using custom Lua plugins](doc/lua.md).

