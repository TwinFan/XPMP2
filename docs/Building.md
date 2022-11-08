# Building XPMP2 and the Sample Plugin

The [XPMP2 Library](https://github.com/TwinFan/XPMP2) and
the [XPMP2-Sample plugin]/https://github.com/TwinFan/XPMP2-Sample)
are now two separate GitHub repositories.

It is recommended that you include `XPMP2` as a submodule in your project.

The `XPMP2-Sample` repo is configured as a _Public Template_,
so you can immediately [base your own project on it](https://docs.github.com/en/repositories/creating-and-managing-repositories/creating-a-repository-from-a-template).
Please note that `XPMP2-Sample` includes `XPMP2` as a submodule,
so you need to checkout those, too, e.g. on the command line by doing
```
git clone --recurse-submodules https://github.com/TwinFan/XPMP2-Sample
```

## Build options

Both repositories share many similarities when it comes to building.
There are four options to build from sources:

Options            | Windows            | MacOS (universal)   | Linux
-------------------|--------------------|---------------------|-------------------
**IDE**            | Visual Studio 2022 | XCode 12            | -
**Docker**         | Mingw64            | clang, SDK 12       | Focal and Bionic
**CMake**          | VS 2022 / `NMAKE`  | XCode 12 / `ninja`  | Focal and Bionic / `ninja`
**Github Actions** | Visual Studio 2022 | XCode 12            | Focal

## Using an IDE

### Windows

- Install [Visual Studio](https://visualstudio.microsoft.com/vs/community/)
- Open the root folder of the repo using "File > Open > Folder",
  [see here for VS's "Open Folder" functionality](https://docs.microsoft.com/en-us/cpp/build/open-folder-projects-cpp?view=vs-2019))
- VS use the CMake setup to configure building the binaries.
- Build from within Visual Studio.

Results are in `build-win`.

### MacOS

My development environment is Mac OS, so expect the XCode environment to be
maintained best. Open `XPMP2.xcodeproj` resp. `XPMP2-Sample.xcodeproj` in XCode.

In the `XPMP2-Sample` XCode Build Settings you may want to change the definition
of the User-Defined build macro `XPLANE11_ROOT`: Put in the path to your
X-Plane 11 installation's root path, then the XPMP2 binaries will be
installed right there in the appropriate `Resources/plugins` sub-folder
structure.

## Using Docker

Docker environments based on Ubuntu 20.04 and 18.04 are used,
which can build all 3 platforms, Linux even in two flavors.

- Install [Docker Desktop](https://www.docker.com/products/docker-desktop) and start it.
- `cd` to the project's `docker` folder, and enter `make` for all
  XPMP2 library targets on all platforms.

The following `make` targets are available:

- `lin` builds Linux on Ubuntu 20.04
- `lin-bionic` builds Linux on Ubuntu 18.04
- `mac-arm` builds MacOS for Apple Silicon using `clang` as cross compiler
- `mac-x86` builds MacOS for x86 using `clang` as cross compiler
- `mac` builds `mac-arm` and `mac-x86` and then combines the results into a univeral binary
- `win` builds Windows using a Mingw64 cross compiler setup
- `bash_focal` starts a bash prompt in the Ubuntu 20.04 docker container
- `bash_bionic` starts a bash prompt in the Ubuntu 18.04 docker container
- `doc` builds the documentation based on Doxygen, will probably only work on a Mac with Doxygen provided in `Applications/Doxygen.app`
- `clean` removes all `build-<platform>` folders

Find results in the respective `build-<platform>` folder, the `XPMP2` library right there,
the Sample plugins in their proper `<platform>_x64` subfolder.

For more details and background information on the provided Docker environments
see the `docker/README.md`.

## Using CMake

Given a proper local setup with a suitable compiler, CMake, and Ninja installed,
you can just locally build the sources from the `CMakeList.txt` file,
e.g. like this:

```
mkdir build
cd build
cmake -G Ninja ..
ninja
```

This is precicely how the Mac and Linux builds are done in Github Actions.

## Using Gitub Actions

The GitHub workflow `.github/workflows/build.yml` builds the plugin in GitHubs CD/CI environment.
`build.yml` calls a number of custom composite actions available in `.github/actions`,
each coming with its own `README.md`.

The workflow builds Linux, MacOS, and Windows plugin binaries and provides them as artifacts,
so you can download the result from the _Actions_ tab on GitHub.

### Apple: Sign and Notiarize

The Apple build of the `XPMP2-Sample` plugin can be signed and notarized,
provided that the following _Repository Secrets_ are defined in the repository's settings
(Settings > Secrets > Actions):
- `MACOS_CERTIFICATE`: Base64-encoded .p12 certificate as
  [explained here](https://localazy.com/blog/how-to-automatically-sign-macos-apps-using-github-actions#lets-get-started)
- `MACOS_CERT_PWD`: The password for the above .p12 certificate file export
- `NOTARIZATION_USERNAME`: Apple ID for notarization service (parameter `--apple-id` to `notarytool`)
- `NOTARIZATION_TEAM`: Team ID for notarization service (parameter `--team-id` to `notarytool`)
- `NOTARIZATION_PASSWORD`: [App-specific password](https://support.apple.com/en-gb/HT204397) for notarization service (parameter `--password` to `notarytool`)

## Including XPMP2 directly in GitHub project and CMake builds

You can avoid separate builds and instead include `XPMP2` directly into your project.
Recommended steps are:

1. Include XPMP2 as a Github Submodule into your Github project, using something like
    ```
    git submodule add --name XPMP2 'https://github.com/TwinFan/XPMP2' lib/XPMP2
    ```
    To update your local version with changes from the XPMP2 repository run something like
    ```
    git submodule update --remote
    ```
2. In your `CMakeLists.txt` file include XPMP2 using something like the following in appropriate places:
    1. If needing FMOD sound support first define
        ```
        set (INCLUDE_FMOD_SOUND 1)              
        add_compile_definitions(INCLUDE_FMOD_SOUND=1)
        include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib/XPMP2/lib/fmod/logo")
        target_sources(${CMAKE_PROJECT_NAME} lib/XPMP2/lib/fmod/logo/FMOD_Logo.cpp)
        ```
    2. Including building XPMP2 as follows:
        ```
        include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib/XPMP2/inc")
        add_subdirectory(lib/XPMP2)
        add_dependencies(${CMAKE_PROJECT_NAME} XPMP2)
        target_link_libraries(${CMAKE_PROJECT_NAME} XPMP2)
        ```
