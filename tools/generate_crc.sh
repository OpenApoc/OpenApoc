#!/usr/bin/env bash
#Version 1.0
#It will create checksum of all XML files found from the modfile

#enable aliases and load up the storage
shopt -s expand_aliases
. ./bashalias.txt

#Variables
SEARCHFOR=*.xml
TRUNCATE='./checksum.xml'
FORMAT="xml"
#Extended regex pattern
PATTERN0=^[[:alnum:]]{8} #CRC Pattern
PATTERN1=[[:blank:]]\./ #Blank lines pattern
PATTERN2=\./.* #The filenames
PATTERN3=\./ #Filename beginning
PATTERN3A=[^\./].* #Filename beginning, exclude

#REGULAR GREP REGEX pattern
#PATTERN0='(^.{0,8})' #Filter the 1st line
#PATTERN1='[[:space:]]\./' #Filter the 2nd line aka the "./" because it is not needed for XML
#PATTERN2='' #Now get the file location itself finally

#TODO: Optimize the process better, make it less error prone
#One issues it has that it appends a crc to checksum.xml from the file itself, OpenApoc
#Log doesn't seem to have trouble with that, odd.

cd $MODNAME
#Look for files that contains XML files
find -name "$SEARCHFOR" |\
#Generate a crc32 checksum
xargs crc32 > checksum
#PATTERN: (8xAlphanumeric)(tabs or 4x space)(./)(filename path)
#Filter the CRC sums
filter $KEEPFOUNDONLY "$PATTERN0" ./checksum > crcsum
filter $KEEPFOUNDONLY "$PATTERN2" ./checksum > crcsum_files
#Now get rid of the ./
filter $KEEPFOUNDONLY "$PATTERN3A" ./crcsum_files > crcsum_files2
#Cleanup
move ./crcsum_files2 ./crcsum_files
#Used this code from some site, due to unfamilirity with awk, it joins 2 files together in a different way
awk 'FNR==NR { a[FNR""] = $0; next } { print a[FNR""], $0 }' crcsum crcsum_files > crcsum_final
#Now the final part
awk 'BEGIN { print "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" }' > checksum.xml
awk 'BEGIN { print "<checksums>"}' >> checksum.xml
awk '{ print "<file>"$2"<CRC>"$1"</CRC></file> "}' crcsum_final >> checksum.xml
awk 'BEGIN { print "</checksums>"}' >> checksum.xml
#Optional Cleanup the trash again
if [ "$CLEANUP" = YES ]; then \
echo "Getting rid of trash"
rm checksum && rm crcsum && rm crcsum_files && rm crcsum_final 
fi
