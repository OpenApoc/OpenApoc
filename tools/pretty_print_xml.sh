#!/usr/bin/env bash
#Version 1.0
#Find xml files and make them nicer formatted

#enable aliases and load up the storage
shopt -s expand_aliases
. ./bashalias.txt
#Constants, you can modify them 
SEARCHFOR="*.xml"
SEARCHFOR1="*.bak"
FORMAT=".xml"
SUFFIX="_PP.xml"
STYLE="record"

#Just in case you wondering if it works at all
if [ "$DEBUG_TXT" = "1" ]; then \
> WORKING.txt
fi

#TODO check if output and input folder exist
cd $INPUT_FOLDER

#Check if any xml files is available
if find $SEARCHFOR 2> /dev/null; then \
    #Pretty print the xml files and blackup 'em
    xml_pp -s $STYLE -v -i.bak *.xml
    #Move the files outside of its location to input
    move $SEARCHFOR ../$OUTPUT_FOLDER 
    #due to the behavior of xml_pp the backups should be temporary and revert back its name
    rename 's/\.bak$//' $SEARCHFOR1

else
    echo "Nothing to Pretty Print a XML file. (Empty input folder)"
fi

#Its done
if [ "$DEBUG_TXT" = "1" ]; then \
sleep 0.75; rm ../WORKING.txt
fi

#OLD, BROKEN CRUDE VERSION
#it needs to be piped to xml_pp
#find -name "*.xml" | xml_pp -s record    

#Task 0: Find XML files and store the results temporarily
#search_file $SEARCHFOR > arg0.txt
#Task 1: Add suffixes to the results with _PP
#open arg0.txt|string_match "s/$FORMAT/$SUFFIX/g" > arg1.txt 
#Task 2: Prettyprint it
#xml_pp -s $STYLE -v -i.bak *.xml
#Task 3: Put the results outside of its current folder to output folder
