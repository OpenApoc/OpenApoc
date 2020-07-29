#include "framework/serialization/providers/filedataprovider.h"
#include "framework/filesystem.h"
#include "framework/logger.h"
#include "library/strings.h"
#include <fstream>
#include <sstream>

namespace OpenApoc
{
bool FileDataProvider::openArchive(const UString &path, bool write)
{
	archivePath = path;
	if (!write && !fs::exists(path))
	{
		LogWarning("Attempt to open not existing directory \"%s\"", path);
		return false;
	}
	return true;
}
bool FileDataProvider::readDocument(const UString &path, UString &result)
{
	std::string documentPath = (static_cast<fs::path>(archivePath) / path).string();
	std::ifstream in(documentPath, std::ios::binary);
	std::ostringstream oss;
	oss << in.rdbuf();
	result = oss.str();
	return !in.bad();
}

bool FileDataProvider::saveDocument(const UString &path, const UString &contents)
{
	fs::path documentPath = (static_cast<fs::path>(archivePath) / path);
	fs::path directoryPath = documentPath.parent_path();
	if (!fs::exists(directoryPath))
	{
		if (!fs::create_directories(directoryPath))
		{
			LogWarning("Failed to create directory \"%s\"", directoryPath.string());
			return false;
		}
	}
	std::ofstream out(documentPath.string(), std::ios::binary | std::ios::trunc);
	out << contents;
	return !out.bad();
}
bool FileDataProvider::finalizeSave() { return true; }
} // namespace OpenApoc
