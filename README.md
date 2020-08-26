# OpenApocalypse [![Tweet](https://img.shields.io/twitter/url/http/shields.io.svg?style=social)](https://twitter.com/intent/tweet?text=Are%20you%20a%20fan%20of%20X-Com%20Apocalypse?%20OpenApoc%20is%20a%20clone%20of%20this%20great%20game%20-%20contribute!%20https://github.com/OpenApoc/OpenApoc&hashtags=games,openapoc,xcom)

> OpenApoc is an open-source re-implementation of the original [X-COM: Apocalypse](https://www.ufopaedia.org/index.php/Apocalypse), that requires the original files to run, licensed under the GPL3 and written in C++ / SDL2. It was originally founded by PmProg in July 2014, and has since grown in [community](https://www.ufopaedia.org/index.php/Credits_(OpenApoc)).

[![Linux Build Status](https://img.shields.io/travis/OpenApoc/OpenApoc/master.svg?label=LinuxTravis)](https://travis-ci.com/github/OpenApoc/OpenApoc)
[![Windows Build Status](https://img.shields.io/appveyor/build/OpenApoc/openapoc/master.svg?label=WindowsAppveyor)](https://ci.appveyor.com/project/openapoc/openapoc/branch/master)
[![Openapoc issues](https://img.shields.io/github/issues/openapoc/openapoc.svg)](https://github.com/openapoc/openapoc/issues)
[![See our Trello TO-DO list for more info](https://img.shields.io/badge/See%20our-Trello%20TO--DO%20list-blue.svg)](https://trello.com/b/lX5Y3DwR/openapoc)
[![Translate OpenApoc to your language](https://img.shields.io/badge/Translate-Openapoc-blue.svg)](https://www.transifex.com/x-com-apocalypse/apocalypse/)
[![OpenApoc GPL3 license](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://github.com/OpenApoc/OpenApoc/blob/master/LICENSE)\
[![Openapoc forum](https://img.shields.io/badge/Visit%20our-forum-orange.svg)](http://openapoc.org)
[![Openapoc IRC chat](https://img.shields.io/badge/IRC-devs%20chat-brightgreen.svg)](http://webchat.freenode.net/?channels=openapoc)
[![Openapoc Discord](https://img.shields.io/discord/142798944970211328.svg?label=discord)](https://discord.gg/d6DAHEb)
[![Openapoc Facebook](https://img.shields.io/badge/Join%20our-Facebook-blue.svg)](https://fb.com/openapoc)
[![Openapoc Vkontakte](https://img.shields.io/badge/Join%20our-Vkontakte-blue.svg)](https://vk.com/openapoc)

<p align="center"><img src="https://i.imgur.com/XxudxVj.jpg"/></p>

## Table of Contents

* [Key Features](#key-features)
* [What's left to finish?](#whats-left)
* [Building](#building)
  * [Building on Windows](#building-on-windows)
  * [Building on Linux](#building-on-linux)
* [How to setup OpenApoc](#how-to-setup-openapoc)
* [Contact us](#contact-us)

## Key Features

* Unlimited modding capabilities, which was not possible in the original
* Port the game to any platform you like (windows, linux, android etc)
* Support for modern screen resolutions
* Added a full debug system ([hot keys](https://github.com/OpenApoc/OpenApoc/blob/master/README_HOTKEYS.txt), etc.)
* Added 'more options' menu (which many improvements)
* Added skirmish module (fast fight)
* We can bring, in mods, Julian Gollop cut ideas into the game
* The new engine have ample opportunities for expansion and changes
  * High FPS, smooth sound during the game without bugs from original
  * No limitations which were in vanilla

## What's left?

* In order to have a playable game, we need:
  * Aliens moving from building to building
  * Funding for X-Com and orgs
  * Agent Training
  * Bribes

That will be a game which has all of its main mechanics implemented. We still have a long way to go, but that's a point at which you can really play Apocalypse in OpenApoc and do everything you need.

* And then, to reach a mostly complete state, we also require:
  * Proper portal location in alien dimension
  * Proper portal movement
  * Alien Takeover screen
  * Score screen and graphs
  * Proper collapse algorithm for alien tubes
  * Victory & Defeat screens
  * UFO growth halting when relevant building is destroyed
  * Organisations properly buying their vehicles
  * Organisations making treaties, raiding each other and x-com, sending illegal flyers
  * Proper Music (not just 3 looped tracks)
  * Overspawn
* Then, to be fully vanilla feature complete, we also need:
  * Better battle AI (behavior, taking cover, more intelligent attack patterns)
  * Better city AI (craft retreat when damaged, other stuff)
  * Proper ground vehicles (occupying lanes, blocking, overtaking etc.)
  * Proper location screen (assigning agents to vehicles)
  * Some fixes to Ufopaedia display
  * Proper transferring agents
  * UI Tooltips
  * Controls for editing text (naming soldiers and other property)
  * Agent medals and statistics
  * Agent name generator (more than 10 names and surnames)
  * Colored text support
* And then, to be a full 1.0 OpenApoc release, we also need:
  * Different file format than xml for storing save files (at least), so that save/loading takes reasonable amount of time
  * Different handling of game data (separation of "rules" and "gamestate", so that for example you can modify a research project in a mod, add that mod to your playthrough midgame, and not have to lose all progress made on that project, or later remove the project mod, and have the changes reversed but research state persist)
  * Close all issues
  * Maybe something else that didn't come up to mind immediately

## Building

OpenApocalypse is built leveraging a number of libraries - to provide needed functionality (and save us the time of implementing it ourselves badly). 
Note: The following libraries will be fetched and built with vcpkg in a later step, ensuring you get the correct version

* [SDL2](https://www.libsdl.org)
* [Boost](https://boost.org) - We specifially use the 'locale' library, used for localisation, 'program-options' for settings management, and 'filesystem'.
* [Qt](https://www.qt.io/) - needed for the launcher, can be disabled with 'BUILD_LAUNCHER'.
* [Libunwind](https://nongnu.org/libunwind/download.html) - debug backtracing on linux - not needed on windows.
* [LibVorbis](https://xiph.org/vorbis/) - Ogg vorbis music decoder library

The following libraries are also used, but are shipped as submodules in the repository and directly included in the build, so you don't need to install these dependencies to build or use openapoc.

* [GLM](https://glm.g-truc.net) - Math library.
* [libsmacker](https://sourceforge.net/projects/libsmacker/) - Decoder for .smk video files.
* [lodepng](https://github.com/lvandeve/lodepng) - Reading/writing PNG image files.
* [Lua](https://www.lua.org/) - Scripting language.
* [miniz](https://github.com/richgel999/miniz) - Zlib-comptible compression library.
* [physfs](https://icculus.org/physfs/) - Library for reading data from .iso files or directory trees (Note: We use a patched version, available on [GitHub](https://github.com/JonnyH/physfs-hg-import/tree/fix-iso) - required to read the .iso files we use).
* [pugixml](https://pugixml.org) - XML library used for reading/writing the game data files.
* [fmtlib](https://github.com/fmtlib/fmt) - A c++ string formatting library - proposed for c++20 standard.

### Building on Windows

* Install [Visual Studio](https://visualstudio.microsoft.com/vs/) (2017 or later) with "Desktop Development with C++" workload.
* Install a Git client eg. [Github Desktop](https://desktop.github.com/).
* Checkout OpenApoc from GitHub.
* If you are using the GitHub Desktop client, the submodules should already be setup at first checkout. If not, or if the submodules have been updated, run the following commands in the 'git shell' from the root of the OpenApoc repository. This should reset the submodule checkouts to the latest versions (NOTE: This will overwrite any changes to code in the dependencies/ directory).

```cmd
git submodule update --init --recursive
```

* All the other dependencies (Boost, SDL2, Qt) need to be supplied separately. Install [Vcpkg](https://github.com/Microsoft/vcpkg) and run the following command:

  * For x64 builds:

```cmd
vcpkg --triplet x64-windows install sdl2 boost-locale boost-program-options boost-uuid boost-crc qt5-base libvorbis
```

  * For x86 builds:

```cmd
vcpkg --triplet x86-windows install sdl2 boost-locale boost-program-options boost-uuid boost-crc qt5-base libvorbis
```

  * For list of all supported by Vcpkg architectures: `vcpkg help triplet`

* Copy the original XCom:Apocalypse .iso file into the "data/" directory. This could also be a directory containing all the extracted files from the CD, and it should be named the same (IE the directory should be data/cd.iso/). This is used during the build to extract some data tables.
* If you do not have Visual Studio (community edition works fine) already, install it and make sure to enable c++ support (Check "Desktop development with C++" in the VS setup)
* Open the OpenApoc directory in Visual Studio (if you don't have an Open Folder option, generate a project with [CMake](https://cmake.org/)).
* Set your configuration to x64-Release or x86-Release (must match your Vcpkg dependencies). Release is recommended as Debug is very slow.

* Visual Studio should automatically detect and configure CMake appropriately. If you didn't integrate Vcpkg, you will need to manually add it to your CMake Settings file:
  * Visual Studio 2017:

```json
"variables": [
    {
        "name": "CMAKE_TOOLCHAIN_FILE",
        "value": "<path to vcpkg>\\scripts\\buildsystems\\vcpkg.cmake"
    }
]
```
  * Visual Studio 2019: Build > CMake Settings > Toolchain file > `<path to vcpkg>\\scripts\\buildsystems\\vcpkg.cmake`

* Build OpenApoc. If you get errors, clear your cache from the CMake menu and generate again.
* When running from the Visual Studio UI, the working directory is set to the root of the project, so the data folder should already be in the right place. If you want to run outside of Visual Studio, you need to copy the whole 'data' folder (including the cd.iso file) into the folder openapoc.exe resides in.

### Building on Linux

(Tested on Ubuntu 16.04, Mageia 6 and Fedora 31)

* On Ubuntu, install the following packages:

```sh
sudo apt-get install libsdl2-dev cmake build-essential git libunwind8-dev libboost-locale-dev libboost-filesystem-dev libboost-program-options-dev qtbase5-dev libvorbis-dev
```

* On Mageia, install the following packages as root:

```sh
urpmi "cmake(sdl2)" libstdc++-static-devel boost-devel boost unwind-devel task-c++-devel cmake git qtbase5-devel libvorbis-devel
```

* On Fedora or other RedHat distro, install the folowing packages as root:

```sh
yum groupinstall "Development Tools" "Development Libraries"
yum install git SDL2-devel cmake libunwind-devel qt5-qtbase-devel libvorbis-devel
```

* Checkout OpenApoc from GitHub.
* Fetch the dependencies from git with the following terminal command (run from the just-created OpenApoc folder).

```sh
git submodule update --init --recursive
```

* Copy the cd.iso file to the 'data' directory under the repository root (Note - despite dosbox having good linux support, the steam version of X-Com Apocalypse will only install if Steam Play is enabled).

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

## How to setup OpenApoc

* Download OpenApoc: [![Windows Build Status](https://img.shields.io/appveyor/build/OpenApoc/openapoc/master.svg?label=WindowsAppveyor)](https://ci.appveyor.com/project/openapoc/openapoc/branch/master)
  * If you see a green latest build then you can get it, if it's not then go to HISTORY at the top and click another build from *master* branch that's green (look for a title beginning with "Merge pull request #")
  * Click Platform x64 (or Win32 if you need 32bit binaries)
  * Click ARTIFACTS
  * Download the first option (without "debug" in it)
  * Unzip downloaded file which will create a new folder with everything from us inside

* Put original X-Com Apocalypse CD into data folder inside OpenApoc folder:
  * if you have disc, copy contents to 'cd.iso' subfolder in data.
  * if you have [Steam](https://store.steampowered.com/app/7660/) version, copy 'cd.iso' file to data.
  * if you have [GOG](https://www.gog.com/game/xcom_apocalypse) version, copy 'xcom.bin' and 'xcom.cue' to data, and rename 'xcom.cue' to 'cd.iso'.
  * pirated versions will not work!

* Run and enjoy!
  * If you find bug report it [here](https://github.com/openapoc/openapoc/issues) (upload also openapoc_log.txt from game folder)

## Contact us

If you're interested, please visit our [website](http://openapoc.org).
We have [forums](http://openapoc.org/forums/) - please pop by and introduce yourself!
We have an IRC channel on [Freenode](http://webchat.freenode.net/?channels=openapoc) - [#openapoc](irc://irc.freenode.net/#openapoc).
We have a [Facebook](https://www.facebook.com/openapoc) page.
