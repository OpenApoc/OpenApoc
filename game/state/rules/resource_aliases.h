#pragma once

#include "library/strings.h"
#include <map>

namespace OpenApoc
{

class ResourceAliases
{
  public:
	std::map<UString, UString> sample;

	void addSample(UString match, UString replacement);
};

} // namespace OpenApoc
