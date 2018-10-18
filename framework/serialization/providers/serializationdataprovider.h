#pragma once

#include "library/strings.h"

namespace OpenApoc
{
// abstract interface for loading files
class SerializationDataProvider
{
	SerializationDataProvider(SerializationDataProvider const &) = delete;
	SerializationDataProvider &operator=(SerializationDataProvider const &) = delete;

  public:
	// opens archive with given path
	virtual bool openArchive(const UString &path, bool write) = 0;
	virtual bool readDocument(const UString &path, UString &result) = 0;
	virtual bool saveDocument(const UString &path, const UString &contents) = 0;
	// should be called after all reads are finished
	virtual bool finalizeSave() = 0;

	SerializationDataProvider() = default;
	virtual ~SerializationDataProvider() = default;
};
} // namespace OpenApoc
