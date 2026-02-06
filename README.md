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

## Usage

### 1. Generate a host key

The `key/` directory ships with a `dummy` placeholder. Before running the server, generate a real key:

```bash
ssh-keygen -t ed25519 -f key/id_ed25519 -N ""
```

### 2. Run the server

From the build output directory:

```bash
./ssh-drop
```

The server listens on port `9393` and waits for a single connection.

### 3. Connect from a client

In another terminal:

```bash
ssh paco@localhost -p 9393
```

Password: `jones`. On success the secret is printed and the server exits.

### Notes

- The server handles one connection then exits — restart it for each test.
- All credentials are hardcoded (expected for the current stage).
- The host key must exist at `key/id_ed25519` relative to the executable's working directory (the build copies `key/` automatically).

##

[![forthebadge](https://forthebadge.com/images/badges/made-with-c-plus-plus.svg)](https://forthebadge.com)
[![forthebadge](https://forthebadge.com/images/badges/powered-by-coffee.svg)](https://forthebadge.com)
