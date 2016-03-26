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
	std::string get(int offset);
	int count() { return readStrings.size(); }
};
} // namespace OpenApoc
