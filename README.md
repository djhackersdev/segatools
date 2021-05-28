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

#### Building with docker on Windows using WSL2

In order to use docker for building on Windows, follow these steps:

* [Install WSL2](https://docs.microsoft.com/en-us/windows/wsl/install-win10)
* Regarding Linux distribution, we recommend using Ubuntu 20.04
* Run the "Ubuntu 20.04 LTS" App which opens a Linux shell
* Install `make` and `docker` by running
  * `sudo apt-get update`
  * `sudo apt-get install make docker.io`
* Add the current user to the docker group that you don't have to run docker commands with root:
`sudo usermod -a -G docker $USER`
* Run the docker daemon in the background: `sudo dockerd > /dev/null 2>&1 &`
* Navigate to your segatools folder. If it is located on the `C:` drive, WSL automatically provides
a mountpoint for that under `/mnt/c`, e.g. `cd /mnt/c/segatools` (if the folder `segatools` is
located under `C:\segatools` on Windows).
* Build segatools: `make build-docker`
