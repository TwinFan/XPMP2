# Building XPMP2 and the Sample Plugin

## Two Projects: XPMP2 Full Build and Sample Plugin only

There are two separate projects provided in the XPMP2 repository:

- One based in the XPMP2 root folder. This project builds all 3 targets:
  - The XPMP2 static library, which is also copied into
    `XPMP2-Sample/lib` so that is thereafter accessible by the stand-alone
    XPMP2-Sample project;
  - The XPMP2 Remote Client, which builds the `XPMP2-Remote.xpl` binary;
  - The XPMP2 Sample plugin, which builds the `XPMP2-Sample.xpl` binary,
    same as with the following stand-alone project.
- a stand-alone project only for the XPMP2-Sample plugin. This is provided
  separately so it can serve as basis for your own plugin implementations.

Using either project works similarly when it comes to building.
For the main XPMP2 projects start in the XPMP2 root folder.
For the XPMP2-Sample stand-alone project start in the `XPMP2-Sample` folder.

Then, there are two options to build from sources:

Options       | Windows            | MacOS            | Linux
--------------|--------------------|------------------|---------------------
**IDE**       | Visual Studio 2019 | XCode 12         | -
**Docker**    | Mingw64            | clang, SDK 11.1  | Focal and Bionic

## Using an IDE

### Windows

- Install [Visual Studio](https://visualstudio.microsoft.com/vs/community/)
- Open the root folder to build the XPMP2 library targets,
  or the `XPMP2-Sample` folder to only build the sample plugin
  ("File > Open > Folder", [see here for VS's "Open Folder" functionality](https://docs.microsoft.com/en-us/cpp/build/open-folder-projects-cpp?view=vs-2019))
- In both cases will VS use the CMake setup to configure building the binaries.
- Build from within Visual Studio.

Results are in `build/x64`.

The `XPMP2.lib` library is also copied into `XPMP2-Sample/lib` to be available in a subsequent build
of the sample plugin.

### MacOS

My development environment is Mac OS, so expect the XCode environment to be
maintained best. Open `XPMP2.xcodeproj` resp.
`XPMP2-Sample/XPMP2-Sample.xcodeproj` in XCode.

In the XCode Build Settings you may want to change the definition
of the User-Defined build macro `XPLANE11_ROOT`: Put in the path to your
X-Plane 11 installation's root path, then the XPMP2 binaries will be
installed right there in the appropriate `Resources/plugins` sub-folder
structure.

## Using Docker

A docker environment based on Ubuntu 20.04 and 18.04 is provided,
which can build all 3 platforms, Linux even in two flavors.

- Install [Docker Desktop](https://www.docker.com/products/docker-desktop) and start it.
- `cd` to the project's `docker` folder, and enter `make` for all
  XPMP2 library targets on all platforms, or
- `cd` to `XPMP2-Sample/docker`, and enter `make` for the sample plugin only
  on all platforms.

6 individual `make` targets are available:

- `lin` builds Linux on Ubuntu 20.04
- `lin-bionic` builds Linux on Ubuntu 18.04
- `mac` builds MacOS using `clang` as cross compiler
- `win` builds Windows using a Mingw64 cross compiler setup
- `bash_focal` starts a bash prompt in the Ubuntu 20.04 docker container
- `bash_bionic` starts a bash prompt in the Ubuntu 18.04 docker container

Find results in the respective `build-<platform>` folder, the `XPMP2` library right there,
the Sample and Remote plugins in their proper `<platform>_x64` subfolder.

The resulting library/framework are also copied into `XPMP2-Sample/lib/<platform>` so it is accessible if building the Sample plugin from the Sample project only.

For more details and background information on the provided Docker environments
see the `docker/README.md`.
