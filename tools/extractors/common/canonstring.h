#pragma once
#include <algorithm>
#include <string>
#include <vector>

#include "library/strings.h"

static std::vector<int> allowed_canon_chars = {
    '-',
};

static inline int canon_char(int c)
{
	if (isalnum(c))
		return toupper(c);
	for (auto &allowed : allowed_canon_chars)
	{
		if (c == allowed)
			return c;
	}
	return '_';
}

static inline OpenApoc::UString canon_string(OpenApoc::UString s)
{
	std::transform(s.begin(), s.end(), s.begin(), canon_char);

	return s;
}
