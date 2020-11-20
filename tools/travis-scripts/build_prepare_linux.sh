#! /bin/bash

set -euo pipefail

echo "Setting ccache variables"
ccache -M 1G
ccache -s

echo "Installing dependencies"
sudo apt-get install libsdl2-dev cmake build-essential git libunwind8-dev libboost-locale-dev libboost-filesystem-dev libboost-program-options-dev qtbase5-dev libvorbis-dev


echo "Fetching minimal cd.iso for build"
wget http://s2.jonnyh.net/pub/cd_minimal.iso.xz -O data/cd.iso.xz
xz -d data/cd.iso.xz

exit 0
