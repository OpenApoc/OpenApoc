# OpenApocalypse
 
The OpenApocalypse project aims to build a open-source
clone of the original X-COM: Apocalypse strategy game.

# BIG CAVEAT
OpenApoc is in early development - it is not yet 'playable' as a game.
All help is appreciated, but there's little point trying to 'play' is as yes

# Current build status:
Linux: [![Linux Build Status](https://travis-ci.org/JonnyH/OpenApoc.svg?branch=master)](https://travis-ci.org/JonnyH/OpenApoc)
Windows: [![Windows Build Status](https://ci.appveyor.com/api/projects/status/07ndsvrbyct924a1)](https://ci.appveyor.com/project/JonnyH/openapoc)
 
# Contact us
If you're interested, please visit us at http://openapoc.org
We have forums at http://openapoc.pmprog.co.uk - please pop by and introduce yourself!
We also have an IRC channel on http://freenode.net - #openapoc
 
## Building
OpenApocalypse is built leveraging a number of libraries - to provide needed functionality (and save us the time of implementing it ourselves badly)
 
- [SDL2](http://www.libsdl.org)
- [TinyXML2](http://www.grinninglizard.com/tinyxml2/) (Version 2)
- [Physfs] (http://icculus.org/physfs/) - though we have patched it to fix some ISO loading bugs on [Github] (https://github.com/JonnyH/physfs-hg-import)- so use that version if you want to use the .iso file as a data source directly
- [GLM] (http://glm.g-truc.net/)
- [Libunwind] (http://www.nongnu.org/libunwind/download.html) - debug backtracing on linux
- [Boost] (http://boost.org) - specifially the 'locale' library, used for localisation

Requirements:
- This assumes that you have the file 'cd.iso' - a copy of the original X-Com Apocalypse CD (This can be got from steam for a pittance http://store.steampowered.com/app/7660/ - this is _required_ to run)

Building on Windows:
(Tested with Visual Studio 2013 community edition)
- Checkout OpenApoc from github
All the required dependencies (SDL2, tinyxml2, physfs, ICU) are packaged as submodules. These submodules are fetched automatically if using the github for windows app, so if you are please skip the next step
- From a Git command line, run the following to fetch the dependency packages
```
git submodule init
git submodule update
```
- Open openapoc.sln in Visual Studio
- Build (Release/Debug x86/x64 should all work)
- Before running, copy the 'cd.iso' file into the 'data' directory in the root of the git project.
-- When running from the Visual Studio UI, the working directory is set to the root of the project, so the data folder should already be in the right place. If you want to run outside of Visual Studio, you need to copy the whole 'data' folder (including the cd.iso file) into the folder openapoc.exe resides in

Building on Linux
(tested on ubuntu 14.04.3 - other distributions will probably need different packages to install - see the dependency list above)
- Install the following packages: libsdl2-dev glm libtinyxml2-dev cmake build-essential git libboost-locale-dev libboost-filesystem-dev libboost-system-dev
```
sudo apt-get install libsdl2-dev libtinyxml2-dev cmake build-essential git libunwind8-dev libboost-locale-dev libboost-filesystem-dev libboost-system-dev
```
- Checkout OpenApoc from github
- Fetch the dependencies from git with the following terminal command (run from the just-created OpenApoc folder)
```
git submodule init
git submodule update
```
-  Build our patched Physfs
-- You can do this by typing the following command in a terminal from the dependencies/physfs directory:
```
cd /path/to/OpenApoc/dependencies/physfs
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .
make
```
-- Install the patched physfs with the following command in the terminal (also in the physfs directory - providing your password if prompted). By default this will install physfs system wide under /usr/local.
```
sudo make install
```
- Configure and install the dependencies version of GLM (libglm-dev in ubuntu 14.04 is old and doesn't seem to work)
```
cd /path/to/OpenApoc/dependencies/glm
cmake .
make
sudo make install
```
- Create a subdirectory ('build' in this example) in the OpenApoc checkout directory, and from that use cmake to configure OpenApoc:
```
cd /path/to/OpenApoc
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
```
- This cmake command will fail if we're missing a dependency, or your system is for some other reason unable to build - if you have any issues please contact us (see above for links)
- Build the project with the following command
```
make -j4
```
- This should create a directory 'bin' under the build directory, with the 'OpenApoc' executable file, and the 'data' directory already in place.
- Copy the cd.iso file to the 'data' directory under build/bin (Note - despite dosbox having good linux support, the steam version of X-Com Apocalypse refuses to install in steam for linux - you may need to snatch the cd.iso file off a windows steam install)
```
cp /path/to/cd.iso data/
```
- Change to the build/bin directory
```
cd bin
```
- Run openapoc
```
./OpenApoc
```
