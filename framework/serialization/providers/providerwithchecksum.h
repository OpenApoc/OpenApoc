#pragma once

#include "framework/serialization/providers/serializationdataprovider.h"
#include "library/sp.h"
#include "library/strings.h"
#include <map>

namespace OpenApoc
{
// decorator for provider adding checksum.xml
class ProviderWithChecksum : public SerializationDataProvider
{
  private:
	// checksums is a map of {"file/path", { {"CHECKSUM1_TYPE", "CHECKSUM1_VALUE"},
	// {"CHECKSUM2_TYPE", "CHECKSUM2_VALUE"}}}
	std::map<UString, std::map<UString, UString>> checksums;
	sp<SerializationDataProvider> inner;
	std::string serializeManifest();
	bool parseManifest(const std::string &manifestData);

  public:
	ProviderWithChecksum(sp<SerializationDataProvider> inner) : inner(inner){};
	ProviderWithChecksum &operator=(ProviderWithChecksum const &) = delete;
	bool openArchive(const UString &path, bool write) override;
	bool readDocument(const UString &path, UString &result) override;
	bool saveDocument(const UString &path, const UString &contents) override;
	bool finalizeSave() override;
};
} // namespace OpenApoc
