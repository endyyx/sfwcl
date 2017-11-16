# sfwcl

This is source of sfwcl of crymp.net, please use Microsoft Visual C++ to compile.

Unmodified version of zlib library from https://github.com/madler/zlib is used here.

This software is licenced under GPLv3 licence. So if you ever decide to make some improvements, I would kindly ask you to commit to existing repo (this one) after creating your fork and making changes instead of creating own one and splitting community once again by creating another solution.

TODOs:
 - fully working 64 bit version
 - asynchronous network communication for verifying servers, logging in, so UX is more fluid
 - ingame map downloader without admin permissions

Other useful tools:
 - Wireshark
 - ollydbg2.0.1 or x64dbg
 - Crysis 1.2 Mod SDK

### Build Instructions

#### Preparation:
- Install CMake
- Install 7-zip (optional, used for creating .pak file)
- Download source code from this GitHub repository and unpack it somewhere
- Create empty build directory
- Create another one if you want to build 64-bit version too (only 64-bit DLL is needed)

#### Building (choose the way):
- Using Visual Studio:
    - open CMake, select directory with source code, select build directory
    - `Configure`
    - select your version of Visual Studio in the list of generators
    - select build features
    - `Generate`
    - `Open Project`
    - select *Release* build in Visual Studio
    - build solution
    - repeat all above steps for 64-bit version using Win64 Visual Studio generator

- Using Windows SDK only:
    - open Windows SDK command prompt
    - run `cmake-gui` in the command prompt
    - select directory with source code and build directory in CMake
    - `Configure`
    - select **NMake Makefiles** generator
    - select build features
    - `Generate`
    - move to the build directory in WinSDK command prompt
    - run `nmake` in the command prompt
    - repeat all above steps for 64-bit version using x64 Windows SDK command prompt

#### Installing:
- copy files to your Crysis directory like this:
~~~~
Crysis/
├── Bin32/
├── Bin64/
├── Game/
├── ...
├── Mods/
│   └── sfwcl/
│       ├── Bin32/ (32-bit binaries)
│       │   ├── sfwcl.dll
│       │   └── mapdl.dll
│       ├── Bin64/ (64-bit binaries)
│       │   ├── sfwcl.dll
│       │   └── mapdl.dll
│       └── Game/
│           └── sfwcl.pak
└── SfwClFiles/ (use either 32-bit or 64-bit binaries here)
    ├── MapDownloader.exe
    ├── SafeWritingClient.exe (used for joining server from web page)
    └── sfwcl_precheck.exe
~~~~
- create `Crysis.exe` shortcut and add `-mod sfwcl` parameter
