#pragma once

#include "library/strings.h"

namespace OpenApoc
{

class Organisation
{
  public:
	UString ID;
	UString name;
	int balance;
	int income;

	Organisation(const UString &ID = "", const UString &name = "", int balance = 0, int income = 0);
	bool isHostileTo(const Organisation &other) const;
};

}; // namespace OpenApoc
