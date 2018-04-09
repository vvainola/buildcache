# BuildCache

This is a simple compiler accelerator that caches and reuses build results to
avoid unnecessary re-compilations, and thereby speeding up the build process.

It is similar in spirit to [ccache](https://ccache.samba.org/).

## Building

Use [CMake](https://cmake.org/) and your favorite C++ compiler to build the BuildCache program:

```bash
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ../src
$ cmake --build .
```

## Usage

To use BuildCache for your builds, simply prefix the build command with
`buildcache`. For instance:

```bash
$ buildcache g++ -c -O2 hello.cpp -o hello.o
```

A convenient solution for bigger CMake-based projects is to use the
`RULE_LAUNCH_COMPILE` property to use BuildCache for all compilation commands,
like so:

```cmake
find_program(buildcache_program buildcache)
if(buildcache_program)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${buildcache_program}")
endif()
```

## Using with icecream

[icecream](https://github.com/icecc/icecream) (or ICECC) is a tool for
distributed compilation. To use icecream you can set the environment variable
`BUILDCACHE_PREFIX` to the icecc executable, e.g:

```bash
$ BUILDCACHE_PREFIX=/usr/bin/icecc buildcache g++ -c -O2 hello.cpp -o hello.o
```

## Supported compilers and languages

Currently the following compilers and languages are supported:

| Compiler | Languages | Supported |
| -------- | --------- | --------- |
| GCC      | C, C++    | Yes       |
| Clang    | C, C++    | Yes       |
| GHS      | C, C++    | Yes       |

New backends are relatively easy to add.

## Debugging

To get debug output from a BuildCache run, set the environment variable
`BUILDCACHE_DEBUG` to the desired debug level:

| BUILDCACHE_DEBUG | Level | Comment           |
| ---------------- | ----- | ----------------- |
| 1                | DEBUG | Maximum printouts |
| 2                | INFO  |                   |
| 3                | ERROR |                   |
| 4                | FATAL |                   |

For instance:

```bash
$ BUILDCACHE_DEBUG=2 buildcache g++ -c -O2 hello.cpp -o hello.o
```

## Status

**NOTE:** BuildCache is still in early development and should not be considered
ready for production projects yet!

There are many missing features, such as cache housekeeping (the cache
currently grows indefinitely and needs to be cleared manually).

