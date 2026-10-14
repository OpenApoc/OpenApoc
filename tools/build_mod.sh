#!/usr/bin/env bash
#Version 1.0
#It will create a testmod zip file for development purpose only.

#enable aliases and load up the storage
shopt -s expand_aliases
. ./bashalias.txt


if [ "$DEBUG_TXT" = "1" ]; then \
> WORKING.txt
fi

#execute a CRC generator, OpenApoc requires crcsum of files in a XML file.
./generate_crc.sh

#Now archive it up.
cd $MODNAME

zip -1r $MODNAME.zip .
mv -f $MODNAME.zip ../$MODNAME_ZIP

if [ "$DEBUG_TXT" = "1" ]; then \
sleep 1.5; rm ../WORKING.txt
fi

