#pragma once
#include "library/strings.h"

namespace OpenApoc
{
class RulesLoader;
class OrganisationDef
{
  private:
	OrganisationDef(){};
	UString name;
	friend class RulesLoader;

  public:
	const UString &getName() const { return this->name; }
};
};
