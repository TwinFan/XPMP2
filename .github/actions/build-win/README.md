# build-win

This is a custom GitHub action to build a library on and for Windows based on a prepared CMake setup.

## Inputs

Parameter|Requied|Default|Description
---------|-------|-------|------------
`libName`|yes    |       |Library's name, used as file name
`flags`  |no     |       |Flags to be passed to CMake

## What it does

- Runs a separate command file, `build-win.cmd`, which in tun
- Creates build folder `build-win`
- There, runs `CMAKE`, then `NMAKE` to build

## Outputs

Output|Description
------|-----------
`lib-file-name`|path to the produced lib file
