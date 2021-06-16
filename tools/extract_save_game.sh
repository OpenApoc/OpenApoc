#!/bin/bash

XML_FORMATTER="xmllint"
XML_FORMATTER_ARGS="--format"

if [ -z "$1" ]; then
    echo "Usage: extract_save_game.sh <save_game> [output_dir]"
    echo "       output_dir is current directory by default"
    exit
fi

savegame=$1
outputdir=${2:-'.'}

if [ ! -f "$savegame" ]; then
	echo "Save game \"$savegame\" not found"
	exit
fi

if [ ! -d "$outputdir" ]; then
	echo "\"$outputdir\" is not a valid directory"
	exit
fi

save_filename=$(basename -- "$savegame")
destdir="$outputdir/$save_filename.dir"


if [ -d "$destdir" ]; then
	read -p "Output dir \"$destdir\" already exists. Override? [y/N]" -n 1 -r
	echo
	if [[ ! $REPLY =~ ^[Yy]$ ]]
	then
		exit
	fi
	rm -fr "$destdir"
fi

echo "Extracting savegame to $destdir"
unzip -q "$savegame" -d "$destdir"

if [ -z `which $XML_FORMATTER` ]; then
	echo "Configured XML formatter \"$XML_FORMATTER\" not found. Skipping formatting."
	exit
fi

echo "Formatting..."
for file in `find "$destdir" -name \*.xml`; do 
	mv "$file" "$file.bk" 
	`$XML_FORMATTER $XML_FORMATTER_ARGS --format "$file.bk" > "$file"` 
	rm "$file.bk"
done

