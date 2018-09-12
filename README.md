# BuildCache

This is a simple compiler accelerator that caches and reuses build results to
avoid unnecessary re-compilations, and thereby speeding up the build process.

It is similar in spirit to [ccache](https://ccache.samba.org/).

## Building

Use [CMake](https://cmake.org/) and your favorite C++ compiler to build the
BuildCache program:

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

Note: At the time of writing there is a
[bug](https://github.com/icecc/icecream/issues/390) in ICECC that may disable
distributed compilation when ICECC is invoked via BuildCache.

## Supported compilers and languages

Currently the following compilers and languages are supported:

| Compiler | Languages | Supported |
| -------- | --------- | --------- |
| GCC      | C, C++    | Yes       |
| Clang    | C, C++    | Yes       |
| MSVC     | C, C++    | Yes       |
| GHS      | C, C++    | Yes       |

New backends are relatively easy to add, both in C++ and in Lua (see below).

## Using custom Lua plugins

It is possible to extend the capabilities of BuildCache with
[Lua](https://www.lua.org/). See [lua-examples](lua-examples/) for some examples
of Lua wrappers.

BuildCache first searches for Lua scripts in the paths given in the environment
variable `BUILDCACHE_LUA_PATH` (colon separated on POSIX systems, and semicolon
separated on Windows), and then continues searching in `$BUILDCACHE_DIR/lua`.
If no matching script file was found, BuildCache falls back to the built in
compiler wrappers (as listed above).

**Note:** To use Lua standard libraries (`coroutine`, `debug`, `io`, `math`,
`os`, `package`, `string`, `table` or `utf8`), you must first load them by
calling `require_std(name)`. For convenience it is possible to load all standard
libraries with `require_std("*")`, but beware that it is slower than to load
only the libraries that are actually used.

All program arguments are available in the global `ARGS` array (an array of
strings).

The following methods can be implemented (see
[program_wrapper.hpp](src/wrappers/program_wrapper.hpp) for a more detailed
documentation):

| Function | Returns | Default |
| --- | --- | --- |
| can_handle_command () | Can the wrapper handle this program? | true |
| resolve_args () | (nothing) | - |
| get_capabilities () | A list of supported capabilities | An empty table |
| preprocess_source () | The preprocessed source code (e.g. for C/C++) | An empty string |
| get_relevant_arguments () | Arguments that can affect the build output | All arguments |
| get_relevant_env_vars () | Environment variables that can affect the build output | An empty table |
| get_program_id () | A unique program identification | The MD4 hash of the program binary |
| get_build_files () | A table of build result files | An empty table |

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

## Configuration options

BuildCache can be configured via environment variables and a per-cache JSON
configuration file. The optinal configuration file is located in the cache
root directory, and is called `config.json` (e.g.
`$HOME/.buildcache/config.json`).

The following options control the behavior of BuildCache:

| Env | JSON | Description | Default |
| --- | --- | --- | --- |
| `BUILDCACHE_DIR` | - | The cache root directory | `$HOME/.buildcache` |
| `BUILDCACHE_PREFIX` | `prefix` | Prefix command for cache misses | None |
| `BUILDCACHE_LUA_PATH` | `lua_paths` | Extra path(s) to Lua wrappers | None |
| `BUILDCACHE_DEBUG` | `debug` | Debug level | None |
| `BUILDCACHE_MAX_CACHE_SIZE` | `max_cache_size` | Cache size limit in bytes | 5368709120 |
| `BUILDCACHE_HARD_LINKS` | `hard_links` | Allow the use of hard links when caching | true |
| `BUILDCACHE_PERF` | `perf` | Enable performance logging | false |
| `BUILDCACHE_DISABLE` | `disable` | Disable caching (bypass BuildCache) | false |

An example configuration file:

```json
{
  "max_cache_size": 10000000000,
  "prefix": "icecc",
  "debug": 3,
  "lua_paths": [
    "/home/myname/buildcache-lua",
    "/opt/buildcache-lua"
  ]
}
```

To see the configuration options that are in effect, run:

```bash
$ buildcache -s
```

## Status

**NOTE:** BuildCache is still in early development and should not be considered
ready for production projects yet!

