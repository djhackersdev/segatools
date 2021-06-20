# Development

This document is intended for developers interested in contributing to segatools. Please read this document before
you start developing/contributing.

## Goals

We want you to understand what this project is about and its goals. The following list serves as a guidance for all
developers to identify valuable contributions for this project. As the project evolves, these goals might do as well.

* Allow running Sega arcade (rhythm) games on arbitrary hardware
  * Emulate required software and hardware features
  * Provide means to cope with incompatibility issues resulting from using a different software platform (e.g. version of Windows).
* Provide an API for custom interfaces and configuring fundamental application features

## Development environment

The following tooling is required in order to build this project.

### Tooling

#### Linux / MacOSX

* git
* make
* mingw-w64
* docker (optional)

On MacOSX, you can use homebrew or macports to install these packages.

#### Windows

TODO

### IDE

Ultimately, you are free to use whatever you feel comfortable with for development. The following is our preferred
development environment which we run on a Linux distribution of our choice:

* Visual Studio Code with the following extensions
  * C/C++
  * C++ Intellisense

### Further tools for testing and debugging

* Debugger: Can be part of your reverse engineering IDE of your choice or stand-along like
[OllyDbg](http://www.ollydbg.de/)
* [apitrace](https://apitrace.github.io/): Trace render calls to graphics APIs like D3D and OpenGL.
This tool allows you to record and re-play render calls of an application with frame-by-frame
debugging. Very useful to analyze the render pipeline or debug graphicial glitches

## Building

The root `Makefile` contains various targets that allow you to build the project easily.

### Local build

For a local build, you need to install Meson and a recent build of MinGW-w64. Then you can start the
build process:

```shell
make build
```

Build output will be located in `build/build32` and `build/build64` folders.

### Cleanup local build

```shell
make clean
```

### Create distribution package (zip file)

```shell
make dist
```

The output will be located in `build/zip`.

### Build and create distribution package using docker

You can also build using docker which avoids having to setup a full development environment if you
are just interested in building binaries of the latest changes. Naturally, this requires you to
have the docker daemon installed.

```shell
make build-docker
```

Once completed successfully, the build output is located in the `build/docker/zip` sub-folder.

### Building with Docker Desktop on Windows

* [Install WSL2](https://docs.microsoft.com/en-us/windows/wsl/install-win10)
* [Install Docker Desktop](https://docs.docker.com/docker-for-windows/install/)
* Run Docker Desktop to start the Docker Engine
* Open a command prompt (`cmd.exe`) and `cd` to your `segatools` folder
* Run `docker-build.bat`
* Once completed successfully, build output is located in the `build/docker/zip` sub-folder.

### Building with Docker on Windows using WSL2

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
* Once completed successfully, build output is located in the `build/docker/zip` sub-folder