#pragma once

#include "framework/serialization/providers/filedataprovider.h"
#include "framework/logger.h"
#include "library/strings.h"
#include <boost/filesystem.hpp>
#include <fstream>

namespace fs = boost::filesystem;

namespace OpenApoc
{
bool FileDataProvider::openArchive(const UString &path, bool write)
{
	archivePath = path;
	if (!write && !fs::exists(path.str()))
	{
		LogWarning("Attempt to open not existing directory \"%s\"", path.c_str());
		return false;
	}
	return true;
}
bool FileDataProvider::readDocument(const UString &path, UString &result)
{
	std::string documentPath = ((fs::path)archivePath.str() / path.str()).string();
	std::ifstream in(documentPath, std::ios::binary);
	result = static_cast<std::ostringstream &>(std::ostringstream{} << in.rdbuf()).str();
	return !in.bad();
}
bool FileDataProvider::saveDocument(const UString &path, UString contents)
{
	fs::path documentPath = ((fs::path)archivePath.str() / path.str());
	fs::path directoryPath = documentPath.parent_path();
	if (!boost::filesystem::exists(directoryPath))
	{
		if (!boost::filesystem::create_directories(directoryPath))
		{
			LogWarning("Failed to create directory \"%s\"", directoryPath.c_str());
			return false;
		}
	}
	std::ofstream out(documentPath.string(), std::ios::binary | std::ios::trunc);
	out << contents.str();
	return !out.bad();
}
bool FileDataProvider::finalizeSave() { return true; }
}