# SSH Drop

Minimal SSH server that securely delivers a secret to authenticated clients.

## Features

- **Public key authentication:** clients authenticate via an `authorized_keys` file
- **Three secret sources:** inline value, file on disk, or environment variable
- **Concurrent connections:** thread-per-connection model, no queueing
- **Auth timeout:** configurable timeout for the authentication phase (default 30 s)
- **Startup validation:** port range, key files, and secret source are checked before binding
- **Configurable logging:** four levels (`debug`, `info`, `warn`, `error`) with optional file output
- **Systemd service:** ships with a hardened unit file and a daily-restart timer

## Build

### Dependencies

- [CMake](https://cmake.org/download/) (>= 3.18)
- [Ninja](https://ninja-build.org/)

### 1. Configure with Ninja

```bash
cmake -B build -G "Ninja Multi-Config" .
```

### 2. Build

```bash
cmake --build build --config Release
```

The [Ninja Multi-Config](https://cmake.org/cmake/help/v3.18/generator/Ninja%20Multi-Config.html)
generator lets you use other build types ([
`CMAKE_BUILD_TYPE`](https://cmake.org/cmake/help/v3.18/variable/CMAKE_BUILD_TYPE.html))
like `Debug` or `RelWithDebInfo`, apart from `Release`.

The resulting binaries are in `build/<build-type>/`.

## Configuration

ssh-drop reads its config from `config/ssh-drop.conf` by default, or from a path passed as the first argument:

```bash
./ssh-drop /etc/ssh-drop/ssh-drop.conf
```

All keys are optional — defaults are shown below:

| Key               | Default               | Description                                             |
|-------------------|-----------------------|---------------------------------------------------------|
| `port`            | `7022`                | TCP port to listen on (1–65535)                         |
| `host_key`        | `key/id_ed25519`      | Path to the server's private host key                   |
| `authorized_keys` | `key/authorized_keys` | Path to the authorized public keys file                 |
| `auth_timeout`    | `30`                  | Seconds before an unauthenticated connection is dropped |
| `log_level`       | `info`                | Minimum log level: `debug`, `info`, `warn`, `error`     |
| `log_file`        | *(empty — stderr)*    | Path to a log file; omit for stderr                     |

### Secret source

Exactly one of the following must be set:

| Key           | Description                                        |
|---------------|----------------------------------------------------|
| `secret`      | Inline secret value                                |
| `secret_file` | Path to a file whose contents are the secret       |
| `secret_env`  | Name of an environment variable holding the secret |

If no secret key is present in the config file, `secret_file` defaults to `secret/secret`.

Example config:

```ini
port = 7022
host_key = /etc/ssh-drop/id_ed25519
authorized_keys = /etc/ssh-drop/authorized_keys
auth_timeout = 30
log_level = info
log_file = /var/log/ssh-drop.log
secret_file = /etc/ssh-drop/secret
```

## Usage

### 1. Generate a host key

```bash
ssh-keygen -t ed25519 -f key/id_ed25519 -N ""
```

### 2. Add client public keys

Append each client's public key to the authorized keys file:

```bash
cat ~/.ssh/id_ed25519.pub >> key/authorized_keys
```

### 3. Set a secret source

Pick one approach — for example, write a secret to a file:

```bash
echo "my-secret-value" > secret/secret
```

Or set it inline in the config:

```ini
secret = my-secret-value
```

### 4. Run the server

```bash
./ssh-drop
```

### 5. Connect from a client

```bash
ssh localhost -p 7022
```

On successful authentication the secret is printed and the connection closes.

## Deployment

### Install files

```bash
sudo useradd -r -s /usr/sbin/nologin ssh-drop

sudo install -Dm755 build/Release/ssh-drop    /usr/local/bin/ssh-drop
sudo install -Dm640 config/ssh-drop.conf      /etc/ssh-drop/ssh-drop.conf
sudo install -Dm600 key/id_ed25519            /etc/ssh-drop/id_ed25519
sudo install -Dm644 key/authorized_keys       /etc/ssh-drop/authorized_keys
sudo install -Dm640 secret/secret             /etc/ssh-drop/secret

sudo chown -R ssh-drop:ssh-drop /etc/ssh-drop
```

### Install systemd units

```bash
sudo install -Dm644 config/ssh-drop.service         /etc/systemd/system/ssh-drop.service
sudo install -Dm644 config/ssh-drop-restart.service  /etc/systemd/system/ssh-drop-restart.service
sudo install -Dm644 config/ssh-drop-restart.timer    /etc/systemd/system/ssh-drop-restart.timer
```

### Enable and start

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now ssh-drop.service
sudo systemctl enable --now ssh-drop-restart.timer
```

### Check status and logs

```bash
sudo systemctl status ssh-drop
sudo journalctl -u ssh-drop -f
```

## License

Licensed under the [Apache License 2.0](LICENSE).

##

[![forthebadge](https://forthebadge.com/images/badges/made-with-c-plus-plus.svg)](https://forthebadge.com)
[![forthebadge](https://forthebadge.com/images/badges/powered-by-coffee.svg)](https://forthebadge.com)
