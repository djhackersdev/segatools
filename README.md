# Segatools

Version: `v005`

Loaders and hardware emulators for SEGA games that run on the Nu and ALLS platforms.

## List of supported games

* Chunithm
  * [Chunithm (Plus)](doc/chunihook.md)
  * [Chunithm Air (Plus)](doc/chunihook.md)
  * [Chunithm Star (Plus)](doc/chunihook.md)
  * [Chunithm Amazon (Plus)](doc/chunihook.md)
  * [Chunithm Crystal (Plus)](doc/chunihook.md)
* Initial D
  * Initial D Zero

## End-users

For setup and configuration guides, refer to the dedicated documents available for each game, see
[the links in the previous section](#list-of-supported-games).

## Developers

### Building

The root `Makefile` contains various targets that allow you to build the project easily.

#### Local build

For a local build, you need to install Meson and a recent build of MinGW-w64. Then you can start the
build process:

```shell
make build
```

Build output will be located in `build/build32` and `build/build64` folders.

#### Cleanup local build

```shell
make clean
```

#### Create distribution package (zip file)

```shell
make dist
```

The output will be located in `build/zip`.

#### Build and create distribution package using docker

You can also build using docker which avoids having to setup a full development environment if you
are just interested in building binaries of the latest changes. Naturally, this requires you to
have the docker daemon installed.

```shell
make build-docker
```

Once completed successfully, the build output is located in the `build/docker/zip` sub-folder.
