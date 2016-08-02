#pragma once

#include "framework/serialization/providers/serializationdataprovider.h"
#include "library/strings.h"

#define MINIZ_HEADER_FILE_ONLY
#include "dependencies/miniz/miniz.c"
#include <map>

namespace OpenApoc
{
// loads data to/from zip files using miniz library
class ZipDataProvider : public SerializationDataProvider
{
  private:
	UString archivePath;
	mz_zip_archive archive;
	UString zipPath;
	bool writing;
	std::map<UString, unsigned int> fileLookup;

  public:
	ZipDataProvider();
	~ZipDataProvider() override;
	ZipDataProvider &operator=(ZipDataProvider const &) = delete;
	bool openArchive(const UString &path, bool write) override;
	bool readDocument(const UString &path, UString &result) override;
	bool saveDocument(const UString &path, UString contents) override;
	bool finalizeSave() override;
};
}