# BuildCache [![Build Status](https://travis-ci.com/mbitsnbites/buildcache.svg?branch=master)](https://travis-ci.com/mbitsnbites/buildcache)

This is a simple compiler accelerator that caches and reuses build results to
avoid unnecessary re-compilations, and thereby speeding up the build process.

It is similar in spirit to [ccache](https://ccache.samba.org/),
[sccache](https://github.com/mozilla/sccache) and
[clcache](https://github.com/frerich/clcache).

## Download

Pre-built binaries of BuildCache can be downloaded [here](https://github.com/mbitsnbites/buildcache/releases/latest).

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
| [GCC](https://gcc.gnu.org/) | C, C++ | Built-in |
| [Clang](https://clang.llvm.org/) | C, C++ | Built-in |
| [Microsoft Visual C++](https://visualstudio.microsoft.com/vs/features/cplusplus/) | C, C++ | Built-in |
| [Green Hills Optimizing Compilers](https://www.ghs.com/products/compiler.html) | C, C++ | Built-in |
| [TI TMS320C6000 Optimizing Compiler](http://www.ti.com/tool/C6000-CGT) | C, C++ | Built-in |
| [TI ARM Optimizing C/C++ Compiler](http://www.ti.com/tool/ARM-CGT) | C, C++ | Built-in |
| TI ARP32 Optimizing C/C++ Compiler | C, C++ | Built-in |
| [scan-build static analyzer](https://clang-analyzer.llvm.org/scan-build.html) | C, C++ | Built-in |

New backends are relatively easy to add, both as built-in wrappers in C++ and as
[Lua wrappers](doc/lua.md).

## Status

BuildCache has been used daily in production environments for years with near
zero issues (any problem that has emereged has of course been fixed), which
gives it a good track record.

With that said, BuildCache is still considered to be under development and
things like configuration parameters and cache formats may change between 0.x
versions.

Once BuildCache has reached version 1.0, all releases will be fully backwards
compatible within a major version (e.g. 1.x).

## Documentation

* [Building BuildCache](doc/building.md)
* [Using BuildCache](doc/usage.md)
* [Configuration](doc/configuration.md)
* [Using custom Lua plugins](doc/lua.md)
* [Contributing to BuildCache](doc/contributing.md)
* [Benchmarks](doc/benchmarks.md)

