# Docker Build environment

This directory contains a setup to build all 3 platforms
(Windows, Linux, MacOS) via Docker environments in a
reproducible way. This allows to verify all three environments
compile and link fine. The resulting binaries are supposed
to actually work, though native build environments
(like Visual Studio or XCode) might produce smaller binaries.

## Just Build

If you "just want to build" then

1. Install [Docker Desktop](https://www.docker.com/products/docker-desktop)
   and make sure it is running
2. `cd docker`
3. `make`

During the very first invocaton this can take several minutes
as the two(!) involved Docker images are build for the first ime.
Subsequent builds will run in a matter of seconds.

Find results in

- `build-lin`
- `build-mac`
- `build-win`

## Setup

### Linux and MacOS based on Ubunto 18.04

For Linux and MacOS builds an environment based on Ubuntu 18.04
is created, defined in `Dockerfile_Bionix` which installs
Linux build and MacOS cross compile tools.

The resulting image is tagged `bionic-lin-mac-compile-env`.

### Windows based on Ubuntu 20.04 with Mingw64

Windows is cross-compiled on an Ubuntu 20.04-based setup
using Mingw 8.0. Creating this image takes especially long
as several tools including Mingw intself are built from sources.

`Dockerfile_Mingw` is based on the image
[`mmozeiko/mingw-w64`](https://hub.docker.com/r/mmozeiko/mingw-w64),
the only change is that the Mingw64/gcc compilers are configured
to use Win32 threads instead of pthreads library.

When finished it is tagged `mingw-compile-env`.
