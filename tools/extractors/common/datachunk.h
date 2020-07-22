#pragma once

#include "framework/logger.h"

#include <cstdlib>
#include <fstream>
#include <vector>

namespace OpenApoc
{

template <typename T> class DataChunk
{
  public:
	std::vector<T> readData;
	DataChunk(std::istream &file, off_t start_offset, off_t end_offset)
	{
		int count = (end_offset - start_offset) / sizeof(T);
		LogInfo("Reading %d units of %zu bytes from offset 0x%08lx", count, sizeof(T),
		        start_offset);
		file.seekg(start_offset, file.beg);
		for (int i = 0; i < count; i++)
		{
			LogAssert(file);
			T data;
			file.read((char *)&data, sizeof(data));
			readData.push_back(data);
		}
	}

	T get(unsigned int offset) const
	{
		LogAssert(offset < readData.size());
		return readData[offset];
	}

	size_t count() const { return readData.size(); }
};
} // namespace OpenApoc
