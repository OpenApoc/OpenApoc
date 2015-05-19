#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class Organisation;
class Framework;

class Building
{
	public:
		Building(Organisation &owner, UString name, Rect<int> bounds);
		Organisation &owner;
		UString name;
		Rect<int> bounds;
		static std::vector<UString> defaultNames;
};

std::vector<Building> loadBuildingsFromBld(Framework &fw, UString fileName, std::vector<Organisation> &orgList, std::vector<UString> nameList);

}; //namespace OpenApoc
