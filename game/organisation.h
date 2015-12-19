#pragma once

#include "library/strings.h"
#include "library/sp.h"

#include <list>

namespace OpenApoc
{

class Vehicle;

class Organisation
{
  public:
	UString ID;
	UString name;
	int balance;
	int income;

	std::list<wp<Vehicle>> vehicles;

	Organisation(const UString &ID = "", const UString &name = "", int balance = 0, int income = 0);
	bool isHostileTo(const Organisation &other) const;
};

}; // namespace OpenApoc
