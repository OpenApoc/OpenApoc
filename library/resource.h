#pragma once

#include "library/strings.h"

namespace OpenApoc
{

// ResObjects are constant objects that can be described by a string (Images, Samples, whatever)
class ResObject
{
  public:
	UString path;
	bool operator==(const ResObject &other) const { return this->path == other.path; }
	bool operator!=(const ResObject &other) const { return !(*this == other); }
};

} // namespace OpenApoc
