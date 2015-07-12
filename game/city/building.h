#pragma once

#include "framework/includes.h"

#include "game/rules/buildingdef.h"

namespace OpenApoc {

class Organisation;
class Framework;

class Building
{
	public:
		Building(BuildingDef &def, Organisation &owner);
		BuildingDef &def;
		Organisation &owner;
};

}; //namespace OpenApoc
