Building XPMP2 and the Sample Plugin
==

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

Using the projects works similarly. For the main XPMP2 projects start
in the XPMP2 root folder. For the XPMP2-Sample stand-alone project
start in the `XPMP2-Sample` folder.

Then, building differs per platform:

Mac OS
---

My development environment is Mac OS, so expect the XCode environment to be
maintained best. Open `XPMP2.xcodeproj` resp.
`XPMP2-Sample/XPMP2-Sample.xcodeproj` in XCode.

In the XCode Build Settings you may want to change the definition
of the User-Defined build macro `XPLANE11_ROOT`: Put in the path to your
X-Plane 11 installation's root path, then the XPMP2 binaries will be
installed right there in the appropriate `Resources/plugins` sub-folder
structure.

Windows
---

Windows cannot build in the Docker environment as mingw's libraries
aren't up to C++ 17 standards.

- Install [Visual Studio](https://visualstudio.microsoft.com/vs/community/)
- Open the root folder to build the XPMP2 library targets,
  or the `XPMP2-Sample` folder to only build the sample plugin
  ("File > Open > Folder", [see here for VS's "Open Folder" functionality](https://docs.microsoft.com/en-us/cpp/build/open-folder-projects-cpp?view=vs-2019))
- In both cases will VS use the CMake setup to configure building the binaries.
- Build from within Visual Studio.

Results are in `build/x64`.

The `XPMP2.lib` library is also copied into `XPMP2-Sample/lib` to be available in a subsequent build
of the sample plugin.

Linux and Mac OS via Docker
---

A docker environment based on Ubuntu 18.04 is provided,
which can build both Linux and Mac OS.

- Install [Docker Desktop](https://www.docker.com/products/docker-desktop) and start it.
- `cd` to the project's `docker` folder, and enter `make` for the three
  XPMP2 library targets, or
- `cd` to `XPMP2-Sample/docker`, and enter `make` for the sample plugin only.
- Three `make` targets are available:
  - `lin` builds Linux only,
  - `mac` builds Mac OS only,
  - `bash` starts a Linux bash within the docker environment.

In the first run only, it will create the necessary Docker image based on Ubuntu 18.04,
which includes downloading lots of base images and packages and even
some compile activities for the Mac OS build toolchain. This takes a couple
of minutes but is required once only. If that part fails your best guess
is to just try again once or twice.

The actual library and sample builds take only a few seconds.
Results are written to `build-lin` and `build-mac`.

The resulting library/framework are also copied into `XPMP2-Sample/lib`.
Then, also the sample plugin must be build using the docker environment.
