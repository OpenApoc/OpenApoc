
#pragma once

#include "game/resources/gamecore.h"

namespace OpenApoc {

class UfopaediaEntry
{
	public:
		UString ID;
		UString Title;
		UString BodyInformation;
		UString DynamicDataMode;
		UString BackgroundImageFilename;

		UfopaediaEntry(tinyxml2::XMLElement* Element);
		~UfopaediaEntry();
};
}; //namespace OpenApoc
