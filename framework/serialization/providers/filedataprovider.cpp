#include "framework/serialization/providers/filedataprovider.h"
#include "framework/logger.h"
#include "library/strings.h"

// Disable automatic #pragma linking for boost - only enabled in msvc and that should provide boost
// symbols as part of the module that uses it
#define BOOST_ALL_NO_LIB
#include <boost/filesystem.hpp>
#include <fstream>
#include <sstream>

namespace fs = boost::filesystem;

namespace OpenApoc
{
bool FileDataProvider::openArchive(const UString &path, bool write)
{
	archivePath = path;
	if (!write && !fs::exists(path.str()))
	{
		LogWarning("Attempt to open not existing directory \"%s\"", path.cStr());
		return false;
	}
	return true;
}
bool FileDataProvider::readDocument(const UString &path, UString &result)
{
	std::string documentPath = (static_cast<fs::path>(archivePath.str()) / path.str()).string();
	std::ifstream in(documentPath, std::ios::binary);
	std::ostringstream oss;
	oss << in.rdbuf();
	result = oss.str();
	return !in.bad();
}

bool FileDataProvider::saveDocument(const UString &path, const UString &contents)
{
	fs::path documentPath = (static_cast<fs::path>(archivePath.str()) / path.str());
	fs::path directoryPath = documentPath.parent_path();
	if (!fs::exists(directoryPath))
	{
		if (!fs::create_directories(directoryPath))
		{
			LogWarning("Failed to create directory \"%s\"", directoryPath.c_str());
			return false;
		}
	}
	std::ofstream out(documentPath.string(), std::ios::binary | std::ios::trunc);
	out << contents;
	return !out.bad();
}
bool FileDataProvider::finalizeSave() { return true; }
}
