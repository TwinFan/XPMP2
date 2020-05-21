Building XPMP2 and the Sample Plugin
==

Mac OS
---

My development environment is Mac OS, so expect the XCode environment to be
maintained best. Open `XPMP2.xcodeproj` in XCode: There is one for the XPMP2
library in the root folder as well as another one for the sample plugin in `XPMP2-Sample`.

**XPMP2 Library:** The resulting XPMP2 framework is also copied into
`XPMP2-Sample/lib` so it is accessible when building the sample plugin.

**Sample Plugin:** In the sample plugin's settings you will need to change
the value of the user-defined build macro `XPLANE11_ROOT` to point to
_your_ X-Plane folder. Then the plugin is installed there right after build.

Windows
---

Windows cannot build in the Docker environment as mingw's libraries
aren't up to C++ 17 standards.

- Install [Visual Studio](https://visualstudio.microsoft.com/vs/community/)
- Open the root folder to build the library
  ("File > Open > Folder", [see here for VS's "Open Folder" functionality](https://docs.microsoft.com/en-us/cpp/build/open-folder-projects-cpp?view=vs-2019))
- Open the folder `XPMP2-Sample` to build the sample plugin
- In both cases will VS use the CMake setup to configure building the binaries.
- Build from within Visual Studio.

Results are in `build/x64`.

The `XPMP2.lib` library is also copied into `XPMP2-Sample/lib` to be available in a subsequent build
of the sample plugin.

Linux and Mac OS via Docker
---

A docker environment based on Ubuntu 18.04 is provided,
which can build both Linux and MacOS.

- Install [Docker Desktop](https://www.docker.com/products/docker-desktop) and start it.
- `cd` to the project's `docker` folder, and enter `make` for the library
- `cd` to `XPMP2-Sample/docker`, and enter `make` for the plugin.
- Three `make` targets are available:
  - `lin` builds Linux only
  - `mac` builds Mac OS only
  - `bash` starts a Linux bash within the docker environment

In the first run only, it will create the necessary Docker image based on Ubuntu 18.04,
which includes downloading lots of base images and packages and even
some compile activities for the Mac OS build toolchain. This takes a couple
of minutes but is required once only. If that part fails your best guess
is to just try again once or twice.

The actual library and sample builds take only a few seconds.
Results are written to `build-lin` and `build-mac`.

The resulting library/framework are also copied into `XPMP2-Sample/lib`.
Then, also the sample plugin must be build using the docker environment.
