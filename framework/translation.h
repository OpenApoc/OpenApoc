#pragma once

#include "library/framework.h"
#include "library/strings.h"

namespace OpenApoc
{

class LocalizedString
{
  public:
	UStringView _id;
	UStringView _pleural;
	UStringView _context;
	int _n;

	LocalizedString(UStringView id) : _id(id){};
	LocalizedString(UStringView context, UStringView id) : _id(id), _context(context){};
	LocalizedString(UStringView id, UStringView pleural, int n)
	    : _id(id), _pleural(pleural), _n(n){};
	LocalizedString(UStringView context, UStringView id, UStringView pleural, int n)
	    : _id(id), _pleural(pleural), _context(context), _n(n){};
};

} // namespace OpenApoc