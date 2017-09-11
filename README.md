# OpenApocalypse [![Linux Build Status](https://travis-ci.org/OpenApoc/OpenApoc.svg?branch=master)](https://travis-ci.org/OpenApoc/OpenApoc)

The OpenApocalypse project aims to build a open-source
clone of the original X-COM: Apocalypse strategy game.

OpenApoc is in early development - it is not yet 'playable' as a game.
All help is appreciated, but there's little point trying to 'play' it as of yet.

[See our Trello TO-DO list for more info.](https://trello.com/b/lX5Y3DwR/openapoc)

## Requirements

* This assumes that you have the file 'cd.iso' - a copy of the original X-Com Apocalypse CD (This can be acquired from [Steam](http://store.steampowered.com/app/7660/) - this is _required_ to run)
 * If you have the real cd, you can copy all the included files to a directory called "cd.iso"

## Building

OpenApocalypse is built leveraging a number of libraries - to provide needed functionality (and save us the time of implementing it ourselves badly)

* [SDL2](http://www.libsdl.org)
* [Boost](http://boost.org) - specifially the 'locale' library, used for localisation, 'program-options' for settings management, and 'filesystem'.
* [Libunwind](http://www.nongnu.org/libunwind/download.html) - debug backtracing on linux - not needed on windows.

The following libraries are also used, but are shipped as submodules in the repository and directly included in the build, so you don't need to install these dependencies to build or use openapoc.

* [GLM](http://glm.g-truc.net) - Math library.
* [libsmacker](http://libsmacker.sourceforge.net/) - Decoder for .smk video files.
* [lodepng](http://lodev.org/lodepng/) - Reading/writing PNG image files.
* [miniz](https://github.com/richgel999/miniz) - Zlib-comptible compression library.
* [physfs](https://icculus.org/physfs/) - Library for reading data from .iso files or directory trees (Note: We use a patched version, available on [GitHub](https://github.com/JonnyH/physfs-hg-import/tree/fix-iso) - required to read the .iso files we use).
* [pugixml](http://http://pugixml.org/) - XML library used for reading/writing the game data files.
* [tinyformat](https://github.com/c42f/tinyformat) - A c++ typesafe string formatting library.

### Building on Windows

* Checkout OpenApoc from GitHub.
* If you are using the GitHub client for Windows, the submodules should already be setup at first checkout. If not using the github client, or if the submodules have been updated, run the following commands in the 'git shell' from the root of the OpenApoc repository. This should reset the submodule checkouts to the latest versions (NOTE: This will overwrite any changes to code in the dependencies/ directory).

```cmd
git submodule init
git submodule update -f
```

* All the other dependencies (Boost, SDL2) are provided automatically by nuget packages, and Visual Studio should automatically download and install these at the first build.
* Copy the original XCom:Apocalypse .iso file into the "data/" directory. This could also be a directory containing all the extracted files from the CD, and it should be named the same (IE the directory should be data/cd.iso/). This is used during the build to extract some data tables.
* Open openapoc.sln in Visual Studio.
* Build (Release/Debug x86/x64 should all work).
* When running from the Visual Studio UI, the working directory is set to the root of the project, so the data folder should already be in the right place. If you want to run outside of Visual Studio, you need to copy the whole 'data' folder (including the cd.iso file) into the folder openapoc.exe resides in.

### Building on Linux

(Tested on Ubuntu 16.04)

* Install the following packages:

```sh
sudo apt-get install libsdl2-dev cmake build-essential git libunwind8-dev libboost-locale-dev libboost-filesystem-dev libboost-system-dev libboost-program-options-dev
```

* Checkout OpenApoc from GitHub.
* Fetch the dependencies from git with the following terminal command (run from the just-created OpenApoc folder).

```sh
git submodule init
git submodule update
```

* Copy the cd.iso file to the 'data' directory under the repository root (Note - despite dosbox having good linux support, the steam version of X-Com Apocalypse refuses to install in steam for linux - you may need to snatch the cd.iso file off a windows steam install).

```sh
cp /path/to/cd.iso data/
```

* Create a subdirectory ('build' in this example) in the OpenApoc checkout directory, and from that use cmake to configure OpenApoc.

```sh
cd /path/to/OpenApoc
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
```

* This cmake command will fail if we're missing a dependency, or your system is for some other reason unable to build - if you have any issues please contact us (see above for links).
* Build the project with the following command.

```sh
make -j4
```

* This should create a directory 'bin' under the build directory, with the 'OpenApoc' executable file. OpenApoc by default expects the data folder to be in the current working directory, so running the executable from the root of the git checkout should work.

```sh
./build/bin/OpenApoc
```

## Contact us

If you're interested, please visit our [website](http://openapoc.org).
We also have [forums](http://openapoc.org/forums/) - please pop by and introduce yourself!
We also have an IRC channel on [Freenode](http://freenode.net) - [#openapoc](irc://irc.freenode.net/#openapoc).
