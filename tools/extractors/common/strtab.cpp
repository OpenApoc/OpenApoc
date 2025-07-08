#include "tools/extractors/common/strtab.h"
#include "framework/logger.h"
#include <iostream>
#include <map>
#include <sstream>

namespace OpenApoc
{

StrTab::StrTab(std::vector<std::string> strings) : readStrings(strings) {}

StrTab::StrTab(std::istream &file, off_t start_offset, off_t end_offset, bool makeUnique)
{
	LogAssert(end_offset > start_offset);
	file.seekg(start_offset, file.beg);
	LogAssert(file);
	char c = '\0';
	std::map<std::string, int> unique_id;
	while (file && file.tellg() <= end_offset)
	{
		std::string s;
		file.get(c);
		while (c && file && file.tellg() <= end_offset)
		{
			s += c;
			file.get(c);
		}
		if (makeUnique)
		{
			if (unique_id.find(s) != unique_id.end())
			{
				std::stringstream ss;
				ss << s << " " << ++unique_id[s];
				s = ss.str();
				LogWarning("Munged string to make unique: \"{}\"", s);
			}
			else
				unique_id.emplace(s, 0);
		}
		readStrings.push_back(s);
	}
	if (c)
		LogError("Table didn't end with NULL");
}

std::string StrTab::get(int offset) const
{
	if (offset >= (int)readStrings.size())
	{
		LogError("Trying to read string table entry {} - table size {}", offset,
		         readStrings.size());
	}
	return readStrings[offset];
}
} // namespace OpenApoc
