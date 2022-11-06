# build-mac

This is a custom GitHub action to build a framwork for MacOS based on a prepared CMake setup.

## Inputs

Parameter|Requied|Default|Description
---------|-------|-------|------------
`libName`|yes    |       |Library's name, used as file name
`flags`  |no     |       |Flags to be passed to CMake

## What it does

- Installs Ninja
- Creates build folder `build-mac`
- There, runs `cmake`, then `ninja` to build
- Zips the framework, including the `-y` parameter to preserve symlinks

## Outputs

Output|Description
------|-----------
`lib-file-name`|path to the produced zip archive of the framework
