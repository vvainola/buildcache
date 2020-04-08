# Building BuildCache

## Building from source

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

