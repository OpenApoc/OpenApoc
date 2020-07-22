#pragma once
#include <fstream>
#include <string>
#include <vector>

namespace OpenApoc
{

class StrTab
{
  public:
	std::vector<std::string> readStrings;
	StrTab(std::istream &file, off_t start_offset, off_t end_offset, bool makeUnique = false);
	StrTab(std::vector<std::string> strings);
	std::string get(int offset) const;
	size_t count() const { return readStrings.size(); }
};
} // namespace OpenApoc
