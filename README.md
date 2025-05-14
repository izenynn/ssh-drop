# SSH Drop

## Info

Simple SSH server that securely "drops" a secret.

## Build

### Dependencies:

- [CMake](https://cmake.org/download/) (>= 3.18).
- [Ninja](https://ninja-build.org/).

### 1. Configure with `Ninja`:

```bash
cmake -B build -G "Ninja Multi-Config" .
```

### 2. Build:

```bash
cmake --build build --config Release
```

The [Ninja Multi-Config](https://cmake.org/cmake/help/v3.18/generator/Ninja%20Multi-Config.html)
generator lets you use other build types ([`CMAKE_BUILD_TYPE`](https://cmake.org/cmake/help/v3.18/variable/CMAKE_BUILD_TYPE.html))
like "`Debug`" or "`RelWithDebInfo`", apart from "`Release`".

The resulting binaries are in `build/<build-type>/`.

##

[![forthebadge](https://forthebadge.com/images/badges/made-with-c-plus-plus.svg)](https://forthebadge.com)
[![forthebadge](https://forthebadge.com/images/badges/powered-by-coffee.svg)](https://forthebadge.com)
