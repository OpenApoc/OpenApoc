#! /bin/sh

unset LC_CTYPE

find -name CMakeCache.txt | xargs rm -v

git submodule init
git submodule update

(
	cd dependencies/physfs
	cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .
	make
	make install
)

(
	cd dependencies/glm
	cmake .
	make
	make install
)

(
	mkdir -p build
	cd build
	cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..

	make
)
