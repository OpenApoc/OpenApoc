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
	std::map<UString, std::string> contents;
	sp<SerializationDataProvider> inner;
	std::string serializeManifest();
	bool parseManifest(const std::string &manifestData);

  public:
	ProviderWithChecksum(sp<SerializationDataProvider> inner) : inner(inner){};
	ProviderWithChecksum &operator=(ProviderWithChecksum const &) = delete;
	bool openArchive(const UString &path, bool write) override;
	bool readDocument(const UString &path, UString &result) override;
	bool saveDocument(const UString &path, UString contents) override;
	bool finalizeSave() override;
};
}
