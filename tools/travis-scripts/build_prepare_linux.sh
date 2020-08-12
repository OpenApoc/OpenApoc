#! /bin/bash

set -euo pipefail

echo "Setting ccache variables"
ccache -M 1G
ccache -s

echo "Installing dependencies"
sudo apt-get install libunwind8-dev libsdl2-dev libboost-locale-dev libboost-filesystem-dev libboost-program-options-dev libegl1-mesa-dev libgles2-mesa-dev qtbase5-dev libvorbis-dev -y

echo "Fetching minimal cd.iso for build"
wget http://s2.jonnyh.net/pub/cd_minimal.iso.xz -O data/cd.iso.xz
xz -d data/cd.iso.xz

exit 0
