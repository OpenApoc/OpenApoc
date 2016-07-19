#pragma once

#include "framework/serialization/providers/zipdataprovider.h"
#include "framework/logger.h"
#include "library/sp.h"
#include "library/strings.h"
#include <boost/filesystem.hpp>
#include <iostream>

#undef MINIZ_HEADER_FILE_ONLY
#include "dependencies/miniz/miniz.c"

namespace fs = boost::filesystem;

namespace OpenApoc
{
ZipDataProvider::ZipDataProvider() { memset(&archive, 0, sizeof(archive)); }
ZipDataProvider::~ZipDataProvider()
{
	if (writing)
	{
		mz_zip_writer_end(&archive);
	}
	else
	{
		mz_zip_reader_end(&archive);
	}
}

bool ZipDataProvider::openArchive(const UString &path, bool write)
{
	this->zipPath = path;
	writing = write;
	if (write)
	{
		if (!mz_zip_writer_init_file(&archive, path.c_str(), 0))
		{
			LogWarning("Failed to init zip file \"%s\" for writing", path.c_str());
			return false;
		}
	}
	else
	{
		if (!mz_zip_reader_init_file(&archive, path.c_str(), 0))
		{
			LogWarning("Failed to init zip file \"%s\" for reading", path.c_str());
			return false;
		}

		unsigned fileCount = mz_zip_reader_get_num_files(&archive);
		for (unsigned idx = 0; idx < fileCount; idx++)
		{
			unsigned filenameLength = mz_zip_reader_get_filename(&archive, idx, nullptr, 0);
			up<char[]> data(new char[(unsigned int)filenameLength]);
			mz_zip_reader_get_filename(&archive, idx, data.get(), filenameLength);
			std::string filename(data.get());
			fileLookup[filename] = idx;
		}
	}

	return true;
}
bool ZipDataProvider::readDocument(const UString &filename, UString &result)
{

	auto it = fileLookup.find(filename.str());
	if (it == fileLookup.end())
	{
		LogWarning("File \"%s\" not found in zip in zip \"%s\"", filename.c_str(), zipPath.c_str());
		return false;
	}
	unsigned int fileId = it->second;
	mz_zip_archive_file_stat stat;
	memset(&stat, 0, sizeof(stat));
	if (!mz_zip_reader_file_stat(&archive, fileId, &stat))
	{
		LogWarning("Failed to stat file \"%s\" in zip \"%s\"", filename.c_str(), zipPath.c_str());
		return false;
	}
	if (stat.m_uncomp_size == 0)
	{
		LogInfo("Skipping %s - possibly a directory?", filename.c_str());
		return false;
	}

	LogInfo("Reading %lu bytes for file \"%s\" in zip \"%s\"", (unsigned long)stat.m_uncomp_size,
	        filename.c_str(), zipPath.c_str());

	up<char[]> data(new char[(unsigned int)stat.m_uncomp_size]);
	if (!mz_zip_reader_extract_to_mem(&archive, fileId, data.get(), (size_t)stat.m_uncomp_size, 0))
	{
		LogWarning("Failed to extract file \"%s\" in zip \"%s\"", filename.c_str(),
		           zipPath.c_str());
		return false;
	}

	result = std::string(data.get(), (unsigned int)stat.m_uncomp_size);
	return true;
}
bool ZipDataProvider::saveDocument(const UString &path, UString contents)
{
	if (!mz_zip_writer_add_mem(&archive, path.c_str(), contents.c_str(), contents.length(),
	                           MZ_DEFAULT_COMPRESSION))
	{
		LogWarning("Failed to insert \"%s\" into zip file \"%s\"", path.c_str(),
		           this->zipPath.c_str());
		return false;
	}
	return true;
}
bool ZipDataProvider::finalizeSave()
{
	if (writing)
	{
		if (!mz_zip_writer_finalize_archive(&archive))
		{
			LogWarning("Failed to finalize archive \"%s\"", zipPath.c_str());
			return false;
		}
	}
	return true;
}
}