# On Windows
`git clone --recurse https://github.com/IntriguingTiles/HLFixes`

Open `HLFixes.sln` and build it. It should build with Visual Studio 2017 and up. I use VS2022 with MSVC v141_xp.

# On Linux
`git clone --recurse https://github.com/IntriguingTiles/HLFixes`  
`cd Linux && mkdir build && cd build`

If you're going to make a build that is intented to run on machines other than your own, you should build within the [Steam Runtime SDK](https://gitlab.steamos.cloud/steamrt/scout/sdk/-/blob/steamrt/scout/README.md).

## With Steam Runtime (schroot)
`schroot --chroot steamrt_scout_i386 -- cmake -DCMAKE_CXX_COMPILER=g++-9 -DCMAKE_C_COMPILER=gcc-9 ..`  
`schroot --chroot steamrt_scout_i386 -- make`

## Without Steam Runtime
`cmake ..`  
`make`