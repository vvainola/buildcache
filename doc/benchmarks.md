# Benchmarks

## LLVM Compilation

For this benchmark, LLVM *(2019-10-22, Git commit: 2c4ca6832fa6b306ee6a7010bfb80a3f2596f824)* was built from source as follows:

```sh
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
time ninja
```

The system used for the benchmark was:

* **CPU**: AMD Ryzen 7 1800x (8-core x86-64, 3.6 GHz).
* **Disk**: 1TB NVMe (960 EVO)
* **RAM**: 32GiB DDR4 @ 3000 MHz
* **OS**: Linux (Ubuntu 18.04)
* **Compiler**: GCC 7.5.0
* **BuildCache**: 0.19.0

### No cache

| Time | Speed |
|---|---|
| 12m17.8s | 1.0x |


### Local cache, no compression

|  | Time | Speed |
|---|---|---|
| Cold cache | 12m56.9s | 0.95x |
| Warm cache | 1m3.5s | 11.6x |

Cache size: **196.8 MiB**


### Local cache, LZ4 compression

|  | Time | Speed |
|---|---|---|
| Cold cache | 12m56.3s | 0.95x |
| Warm cache | 1m5.8s | 11.2x |

Cache size: **75.3 MiB**


### Local cache, ZSTD compression

|  | Time | Speed |
|---|---|---|
| Cold cache | 12m56.6s | 0.95x |
| Warm cache | 1m2.6s | 11.8x |

Cache size: **49.7 MiB**

