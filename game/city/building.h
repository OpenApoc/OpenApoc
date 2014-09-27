#pragma once

#include "organisation.h"
#include "../../library/rect.h"
#include <list>

class Building
{
	public:
		Building(Organisation &owner, std::string name, Rect<int> bounds);
		Organisation &owner;
		std::string name;
		Rect<int> bounds;
		static std::vector<std::string> defaultNames;
};

std::list<Building> loadBuildingsFromBld(std::string fileName, std::vector<Organisation> &orgList, std::vector<std::string> nameList);
