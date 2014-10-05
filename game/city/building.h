#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class Organisation;
class Framework;

class Building
{
	public:
		Building(Organisation &owner, std::string name, Rect<int> bounds);
		Organisation &owner;
		std::string name;
		Rect<int> bounds;
		static std::vector<std::string> defaultNames;
};

std::list<Building> loadBuildingsFromBld(Framework &fw, std::string fileName, std::vector<Organisation> &orgList, std::vector<std::string> nameList);

}; //namespace OpenApoc
