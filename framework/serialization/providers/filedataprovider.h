#pragma once

#include "framework/serialization/providers/serializationdataprovider.h"
#include "library/strings.h"

namespace OpenApoc
{
// loads data from files in catalogue
class FileDataProvider : public SerializationDataProvider
{
  private:
	UString archivePath;

  public:
	FileDataProvider &operator=(FileDataProvider const &) = delete;
	bool openArchive(const UString &path, bool write) override;
	bool readDocument(const UString &path, UString &result) override;
	bool saveDocument(const UString &path, const UString &contents) override;
	bool finalizeSave() override;
};
} // namespace OpenApoc
