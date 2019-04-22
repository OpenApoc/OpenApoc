#There a few bash scripts to make your OpenApoc life a bit easier.
#Since I am using Linux only I have no idea how to make them Windows and/or POSIX compitable
#I would have to do some few research on my own to check for that
#Drop all the bash scripts INSIDE the ~/data folder.

TODO:
minor:
Add a few more user variables
Improve the process of other bash scripts so that it calls less commands or so
major:
Figure out a way to force reload a mod on a save game
POSIX compability

#Filenames:
generate_crc.sh (v1.0)
Purpose: Generates OpenApoc friendly checksum.xml file which uses crc
It loops through all files inside the mod folder ($MODNAME) which are in .xml and then generates
a crc alphabetic numerals, due to odd formating I had to seperate the filter to 2 outputs as a file,
then finally it runs a awk script that glues them together and produces a OpenApoc friendly checksum.xml file. 
The produced results are tested and according to OpenApoc logs it is read with success. This one took most of my time but,
I'm glad I made it because it will make my life and possible other easier.

build_mod.sh (v1.0)
Purpose: generates a simple compressed zip file from a folder defined by $MODNAME and $MODNAME_ZIP from bashalias.txt
A relative simple script, all it does is calling upon generate_crc.sh then waiting for its result and then it just zips up
all the file inside the $MODNAME folder then it places it outside the mod folder which is called $MODNAME_ZIP 
then its done with it job. It completely replaces the zip file too.

pretty_print_xml.sh (v1.0)
dependcies: xml_pp
Purpose: pretty prints OpenApoc hideous looking XML file
You need to place the xml file defined by $INPUT_FOLDER (default AB_input) then xml_pp 

#Getting the Mod to load
Well once you figured out some of the few gritty details on your own I will give you another hint.
At "~/OpenApoc/game/ui/general" there is a file called difficultymenu.cpp, scroll down a bit and you should
see a particular 5 lines of code that looks like this:

		if (!state->loadGame(fw().getDataDir() + "/gamestate_common"))
		{
			LogError("Failed to load common gamestate");
			return;
		}
		if (!state->loadGame(fw().getDataDir() + "/" + path))

Just copy it AFTER the other 2 and rename gamestate_common to whatever your zip modname is called, also
you might want to rename the string part of LogError so that it makes a bit more sense related to your mod. 
Of course after doing some dirty changes you need to recompile it don't forget it, once you have all set up OpenApoc
should now load your mod at least on a new game start. There is also a lua command to load the mod which is 
"OpenApoc.GameState.loadGame('path')" but I haven't used that one myself so use it on your own risk. There is also
3 more files you might want to check out which are:
/build/tools/extractors/CMakeFiles/extract-data.dir/build.make
/build/tools/extractors/CMakeFiles/extract-data.dir/cmake_clean.cmake
/tools/extractors/CMakeLists.txt
But I haven't figured out their purpose yet.


