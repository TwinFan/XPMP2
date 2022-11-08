# build-lin

This is a custom GitHub action to build a library on Linux based on a prepared CMake setup.

## Parameters

Parameter|Requied|Default|Description
---------|-------|-------|------------
`libName`|yes    |       |Library's name, used as file name
`flags`  |no     |       |Flags to be passed to CMake

## What it does

- Installs Ninja and OpenGL libs
- Creates build folder `build-lin`
- There, runs `cmake`, then `ninja` to build

## Outputs

Output|Description
------|-----------
`lib-file-name`|path to the produced library
