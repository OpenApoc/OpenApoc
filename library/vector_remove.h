#pragma once

#include <algorithm>
#include <vector>

namespace OpenApoc
{

// Removes a single item matching 'item' from vector - note this may re-order the vector so don't
// use if order is important!
template <typename T> bool removeOneItemFromVector(std::vector<T> &vec, const T &item)
{
	auto it = std::find(vec.begin(), vec.end(), item);
	if (it == vec.end())
		return false;

	auto lastIt = vec.end();
	lastIt--;
	if (it != lastIt)
		std::swap(it, lastIt);

	vec.erase(lastIt, vec.end());
	return true;
}

} // namespace OpenApoc
