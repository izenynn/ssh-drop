# SSH Drop

Minimal SSH server that securely delivers a secret to authenticated clients.

> **Warning:** This software has not been independently tested, audited, or reviewed for security. Use it at your own risk and do not rely on it in security-critical environments.

## Features

- **Flexible authentication:** public key, password, or both (multi-factor)
- **Optional username check:** restrict connections to a specific SSH username
- **Three secret sources:** inline value, file on disk, or environment variable
- **Concurrent connections:** thread-per-connection model, no queueing
- **Auth timeout:** configurable timeout for the authentication phase (default 30 s)
- **Startup validation:** port range, key files, and secret source are checked before binding
- **Configurable logging:** four levels (`debug`, `info`, `warn`, `error`) with optional file output
- **Systemd service:** ships with a hardened unit file and a daily-restart timer

## Build

### Dependencies

- C++20 compatible compiler
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

A config file is **mandatory**. ssh-drop reads it from `config/ssh-drop.conf` by default, or from a path passed as the first argument:

```bash
./ssh-drop /etc/ssh-drop/ssh-drop.conf
```

### Required fields

These must always be present in the config file:

| Key           | Description                                             |
|---------------|---------------------------------------------------------|
| `port`        | TCP port to listen on (1–65535)                         |
| `host_key`    | Path to the server's private host key                   |
| `auth_method` | Authentication mode: `publickey`, `password`, or `both` |

A secret source is also required — exactly one of:

| Key           | Description                                        |
|---------------|----------------------------------------------------|
| `secret`      | Inline secret value                                |
| `secret_file` | Path to a file whose contents are the secret       |
| `secret_env`  | Name of an environment variable holding the secret |

### Conditionally required fields

| Key               | Required when                              | Description                         |
|-------------------|--------------------------------------------|-------------------------------------|
| `authorized_keys` | `auth_method` is `publickey` or `both`     | Path to the authorized public keys file |

A password source is required when `auth_method` is `password` or `both` — exactly one of:

| Key                  | Description                                         |
|----------------------|-----------------------------------------------------|
| `auth_password`      | Inline password value                               |
| `auth_password_file` | Path to a file whose contents are the password      |
| `auth_password_env`  | Name of an environment variable holding the password |

### Optional fields

| Key            | Default            | Description                                             |
|----------------|--------------------|---------------------------------------------------------|
| `auth_timeout` | `30`               | Seconds before an unauthenticated connection is dropped |
| `log_level`    | `info`             | Minimum log level: `debug`, `info`, `warn`, `error`     |
| `log_file`     | *(empty)*          | Path to a log file (see below)                           |

When `log_file` is omitted, errors go to stderr and everything else to stdout.
When `log_file` is set, output goes to **both** the console (as above) and the file.

### Authentication

#### Public key mode

Clients authenticate with a key listed in the `authorized_keys` file (`auth_method = publickey`).

#### Password mode

Set `auth_method = password` and provide exactly one password source:

| Key                  | Description                                         |
|----------------------|-----------------------------------------------------|
| `auth_password`      | Inline password value                               |
| `auth_password_file` | Path to a file whose contents are the password      |
| `auth_password_env`  | Name of an environment variable holding the password |

#### Both mode (multi-factor)

Set `auth_method = both` to require **both** a valid public key **and** a correct password. The server enforces a strict pubkey-first order: only the public key method is advertised initially. The password prompt is only revealed after the key is verified via SSH partial authentication. This prevents attackers from brute-forcing passwords without a valid key.

#### Username check (optional, any mode)

To restrict which SSH username is accepted, set exactly one:

| Key              | Description                                       |
|------------------|---------------------------------------------------|
| `auth_user`      | Inline username value                             |
| `auth_user_file` | Path to a file whose contents are the username    |
| `auth_user_env`  | Name of an environment variable holding the username |

When set, the connecting client's SSH username must match. When omitted, any username is accepted.

### File source tips

`secret_file`, `auth_password_file`, and `auth_user_file` read the **entire** file contents, including any trailing newline. Use `echo -n` or `printf` when writing these files to avoid an unintended trailing newline:

```bash
printf 'my-secret-value' > /etc/ssh-drop/secret
```

### Example configs

Public key only:

```ini
port = 7022
host_key = /etc/ssh-drop/id_ed25519
authorized_keys = /etc/ssh-drop/authorized_keys
auth_method = publickey
secret_file = /etc/ssh-drop/secret
```

Password only:

```ini
port = 7022
host_key = /etc/ssh-drop/id_ed25519
auth_method = password
auth_password_file = /etc/ssh-drop/password
secret_file = /etc/ssh-drop/secret
```

Both (multi-factor):

```ini
port = 7022
host_key = /etc/ssh-drop/id_ed25519
authorized_keys = /etc/ssh-drop/authorized_keys
auth_method = both
auth_password_env = SSH_DROP_PASSWORD
auth_user = deploy
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

A config file must exist at `config/ssh-drop.conf`, or pass a custom path as the first argument:

```bash
./ssh-drop
./ssh-drop /etc/ssh-drop/ssh-drop.conf
```

### 5. Connect from a client

Public key mode:

```bash
ssh localhost -p 7022
```

Password mode:

```bash
ssh localhost -p 7022 -o PreferredAuthentications=password
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
sudo install -Dm644 deploy/ssh-drop.service         /etc/systemd/system/ssh-drop.service
sudo install -Dm644 deploy/ssh-drop-restart.service  /etc/systemd/system/ssh-drop-restart.service
sudo install -Dm644 deploy/ssh-drop-restart.timer    /etc/systemd/system/ssh-drop-restart.timer
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
