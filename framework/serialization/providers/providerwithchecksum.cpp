#define SHA1_CHECKSUM
#include "framework/serialization/providers/providerwithchecksum.h"
#include "framework/configfile.h"
#include "framework/logger.h"
#include "framework/options.h"
#include "library/strings.h"
#include "library/strings_format.h"
#include <sstream>

#include "dependencies/pugixml/src/pugixml.hpp"

#include <boost/crc.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION >= 106600
#include <boost/uuid/detail/sha1.hpp>
#else
#include <boost/uuid/sha1.hpp>
#endif
#include <boost/uuid/uuid.hpp>

namespace OpenApoc
{

static UString calculateSHA1Checksum(const std::string &str)
{
	UString hashString;

	boost::uuids::detail::sha1 sha;
	sha.process_bytes(str.c_str(), str.size());
	unsigned int hash[5];
	sha.get_digest(hash);
	for (int i = 0; i < 5; i++)
	{
		unsigned int v = hash[i];
		for (int j = 0; j < 4; j++)
		{
			// FIXME: Probably need to do the reverse for big endian?
			unsigned int byteHex = v & 0xff000000;
			byteHex >>= 24;
			hashString += format("%02x", byteHex);
			v <<= 8;
		}
	}

	return hashString;
}
static UString calculateCRCChecksum(const std::string &str)
{
	UString hashString;

	boost::crc_32_type crc;
	crc.process_bytes(str.c_str(), str.size());
	auto hash = crc.checksum();
	hashString = format("%08x", hash);
	return hashString;
}

static UString calculateChecksum(const UString &type, const std::string &str)
{
	if (type == "CRC")
	{
		return calculateCRCChecksum(str);
	}
	else if (type == "SHA1")
	{
		return calculateSHA1Checksum(str);
	}
	else
	{
		LogWarning("Unknown checksum type \"%s\"", type);
		return "";
	}
}

std::string ProviderWithChecksum::serializeManifest()
{
	pugi::xml_document manifestDoc;
	auto decl = manifestDoc.prepend_child(pugi::node_declaration);
	decl.append_attribute("version") = "1.0";
	decl.append_attribute("encoding") = "UTF-8";
	auto root = manifestDoc.root().append_child();
	root.set_name("checksums");
	for (auto &p : checksums)
	{
		auto node = root.append_child();
		node.set_name("file");
		node.text().set(p.first.c_str());
		for (auto &csum : p.second)
		{
			auto checksumNode = node.append_child();
			checksumNode.set_name(csum.first.c_str());
			checksumNode.text().set(csum.second.c_str());
		}
	}
	std::stringstream ss;
	manifestDoc.save(ss, "  ");
	return ss.str();
}

bool ProviderWithChecksum::parseManifest(const std::string &manifestData)
{
	std::stringstream ss(manifestData);
	pugi::xml_document manifestDoc;
	auto parse_result = manifestDoc.load(ss);
	if (!parse_result)
	{
		LogWarning("Failed to parse checksum.xml : \"%s\" at \"%llu\"", parse_result.description(),
		           (unsigned long long)parse_result.offset);
		return false;
	}
	auto rootNode = manifestDoc.child("checksums");
	if (!rootNode)
	{
		LogWarning("checksum.xml has invalid root node");
		return false;
	}
	auto fileNode = rootNode.child("file");
	while (fileNode)
	{
		UString fileName = fileNode.text().get();

		if (this->checksums.find(fileName) != this->checksums.end())
		{
			LogWarning("Multiple manifest entries for path \"%s\"", fileName);
		}

		this->checksums[fileName] = {};

		auto checksumNode = fileNode.first_child();
		while (checksumNode)
		{
			if (checksumNode.type() == pugi::xml_node_type::node_element)
			{
				UString checksumType = checksumNode.name();
				this->checksums[fileName][checksumType] = checksumNode.text().get();
			}
			checksumNode = checksumNode.next_sibling();
		}
		fileNode = fileNode.next_sibling("file");
	}

	return true;
}
bool ProviderWithChecksum::openArchive(const UString &path, bool write)
{

	if (!inner->openArchive(path, write))
	{
		return false;
	}

	if (!write)
	{
		UString result;
		if (!inner->readDocument("checksum.xml", result))
		{
			LogInfo("Missing manifest file in \"%s\"", path);
			return true;
		}
		parseManifest(result);
	}
	return true;
}
bool ProviderWithChecksum::readDocument(const UString &path, UString &result)
{
	if (inner->readDocument(path, result))
	{
		for (auto &csum : checksums[path])
		{
			auto expectedCSum = csum.second;
			auto calculatedCSum = calculateChecksum(csum.first, result);
			if (expectedCSum != calculatedCSum)
			{
				LogWarning("File \"%s\" has incorrect \"%s\" checksum \"%s\", expected \"%s\"",
				           path, csum.first, calculatedCSum, expectedCSum);
			}
			else
			{
				LogDebug("File \"%s\" matches \"%s\" checksum \"%s\"", path, csum.first,
				         calculatedCSum);
			}
		}
		return true;
	}

	return false;
}
bool ProviderWithChecksum::saveDocument(const UString &path, const UString &contents)
{

	if (inner->saveDocument(path, contents))
	{
		if (this->checksums.find(path) != this->checksums.end())
		{
			LogWarning("Multiple document entries for path \"%s\"", path);
		}
		this->checksums[path] = {};
		if (Options::useCRCChecksum.get())
			this->checksums[path]["CRC"] = calculateChecksum("CRC", contents);
		if (Options::useSHA1Checksum.get())
			this->checksums[path]["SHA1"] = calculateChecksum("SHA1", contents);
		return true;
	}
	return false;
}
bool ProviderWithChecksum::finalizeSave()
{
	UString manifest = serializeManifest();
	inner->saveDocument("checksum.xml", manifest);
	return inner->finalizeSave();
}
} // namespace OpenApoc
