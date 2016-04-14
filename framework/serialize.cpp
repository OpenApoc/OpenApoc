#define PUGIXML_NO_XPATH
#define PUGIXML_HEADER_ONLY
#include "dependencies/pugixml/src/pugixml.hpp"

#include "framework/logger.h"
#include "library/sp.h"
#include "library/strings.h"

#include "framework/serialize.h"

//#define MINIZ_HEADER_FILE_ONLY
#include "dependencies/miniz/miniz.c"

// Disable automatic #pragma linking for boost - only enabled in msvc and that should provide boost
// symbols as part of the module that uses it
#define BOOST_ALL_NO_LIB
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/uuid/sha1.hpp>
#include <map>

namespace OpenApoc
{

sp<SerializationNode> SerializationNode::getNodeReq(const UString &name)
{
	auto node = this->getNodeOpt(name);
	if (!node)
	{
		throw SerializationException("Missing node \"" + name + "\"", shared_from_this());
	}
	return node;
}

sp<SerializationNode> SerializationNode::getSectionReq(const UString &name)
{
	auto node = this->getSectionOpt(name);
	if (!node)
	{
		throw SerializationException("Missing section \"" + name + "\"", shared_from_this());
	}
	return node;
}

sp<SerializationNode> SerializationNode::getNextSiblingReq(const UString &name)
{
	auto node = this->getNextSiblingOpt(name);
	if (!node)
	{
		throw SerializationException("Missing sibling of \"" + name + "\"", shared_from_this());
	}
	return node;
}

static UString calculate_checksum(const std::string &str)
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
			hashString += UString::format("%02x", byteHex).str();
			v <<= 8;
		}
	}

	return hashString;
}

static std::string writeManifest(const std::map<UString, std::string> &contents)
{
	pugi::xml_document manifestDoc;
	auto decl = manifestDoc.prepend_child(pugi::node_declaration);
	decl.append_attribute("version") = "1.0";
	decl.append_attribute("encoding") = "UTF-8";
	auto root = manifestDoc.root().append_child();
	root.set_name("openapoc_manifest");
	for (auto &p : contents)
	{
		auto node = root.append_child();
		node.set_name("gamestate_xml");
		node.text().set(p.first.c_str());
		auto sha1sum = calculate_checksum(p.second);
		node.append_attribute("SHA1") = sha1sum.c_str();
	}
	std::stringstream ss;
	manifestDoc.save(ss, "  ");
	return ss.str();
}

static std::map<UString, UString> readManifest(const std::string &manifestData)
{
	std::map<UString, UString> shaMap;
	std::stringstream ss(manifestData);
	pugi::xml_document manifestDoc;
	auto parse_result = manifestDoc.load(ss);
	if (!parse_result)
	{
		LogError("Failed to parse openapoc_manifest.xml : \"%s\" at \"%llu\"",
		         parse_result.description(), (unsigned long long)parse_result.offset);
		return {};
	}
	auto rootNode = manifestDoc.child("openapoc_manifest");
	if (!rootNode)
	{
		LogError("openapoc_manifest.xml has invalid root node");
		return {};
	}
	auto gamestateNode = rootNode.child("gamestate_xml");
	while (gamestateNode)
	{
		UString fileName = gamestateNode.text().get();
		auto shaAttribute = gamestateNode.attribute("SHA1");
		if (!shaAttribute)
		{
			LogWarning("openapoc_manifest.xml gamestate_xml node \"%s\" has no SHA1 attribute",
			           fileName.c_str());
			shaMap[fileName] = "";
		}
		else
		{
			shaMap[fileName] = shaAttribute.value();
		}
		LogInfo("Manifest \"%s\" sha1=\"%s\"", fileName.c_str(), shaAttribute.value());
		gamestateNode = gamestateNode.next_sibling("gamestate_xml");
	}
	return shaMap;
}

static bool writeDir(const UString &name, const std::map<UString, std::string> &contents)
{
	auto manifestData = writeManifest(contents);
	auto manifestPathStr = name + "/openapoc_manifest.xml";
	boost::filesystem::path manifestPath(manifestPathStr.str());
	boost::filesystem::ofstream outManifest(manifestPath, std::ios::out | std::ios::binary);
	if (!outManifest)
	{
		LogError("Failed to open manifest file \"%s\" for writing", manifestPath.string().c_str());
		return false;
	}

	outManifest.write(manifestData.c_str(), manifestData.size());
	if (!outManifest)
	{
		LogError("Failed to write manifest file \"%s\"", manifestPath.string().c_str());
		return false;
	}

	for (auto p : contents)
	{
		auto fullPathStr = name + "/" + p.first;
		boost::filesystem::path fullPath(fullPathStr.c_str());
		LogInfo("Writing to \"%s\"", fullPathStr.c_str());
		auto dirPath = fullPath;
		dirPath.remove_filename();
		LogInfo("Directory: \"%s\"", dirPath.string().c_str());
		if (boost::filesystem::exists(dirPath) && !boost::filesystem::is_directory(dirPath))
		{
			LogError("Trying to write to a directory \"%s\" but is already exists and is not a "
			         "directory",
			         dirPath.string().c_str());
			return false;
		}
		if (!boost::filesystem::exists(dirPath))
		{
			if (!boost::filesystem::create_directories(dirPath))
			{
				LogError("Failed to create directory \"%s\"", dirPath.string().c_str());
				return false;
			}
		}
		boost::filesystem::ofstream outStream(fullPath, std::ios::out | std::ios::binary);
		if (!outStream)
		{
			LogError("Failed to open \"%s\" for writing", fullPath.string().c_str());
			return false;
		}
		outStream.write(p.second.c_str(), p.second.size());
		if (!outStream)
		{
			LogError("Failed to write \"%s\"", fullPath.string().c_str());
			return false;
		}

		auto sha1sum = calculate_checksum(p.second);

		LogInfo("file \"%s\" SHA1 \"%s\"", fullPath.string().c_str(), sha1sum.c_str());
	}
	return true;
}

static bool readDir(const UString &name, std::map<UString, std::string> &contents)
{
	boost::filesystem::path basePath = name.str();
	if (!boost::filesystem::is_directory(basePath))
	{
		LogInfo("Path \"%s\" not a directory, not reading DirArchive", name.c_str());
		return false;
	}
	auto manifestPath = basePath;
	manifestPath /= "openapoc_manifest.xml";
	boost::filesystem::ifstream manifestInStream(manifestPath, std::ios::in | std::ios::binary);
	if (!manifestInStream)
	{
		LogError("Failed to open manifest \"%s\"", manifestPath.string().c_str());
		return false;
	}
	auto manifestSize = boost::filesystem::file_size(manifestPath);

	LogInfo("Reading %llu bytes from manifest \"%s\"", (unsigned long long)manifestSize,
	        manifestPath.string().c_str());

	up<char[]> manifestData(new char[manifestSize]);
	manifestInStream.read(manifestData.get(), manifestSize);

	if (!manifestInStream)
	{
		LogError("Failed to read %llu bytes from manifest \"%s\"", (unsigned long long)manifestSize,
		         manifestPath.string().c_str());
		return false;
	}

	auto manifestContents = readManifest(std::string(manifestData.get(), manifestSize));

	if (manifestContents.empty())
	{
		LogError("Manfest \"%s\" contains no entries", manifestPath.string().c_str());
		return false;
	}

	for (auto &p : manifestContents)
	{
		auto fullPath = basePath;
		fullPath /= p.first.str();
		boost::filesystem::ifstream inStream(fullPath, std::ios::in | std::ios::binary);
		if (!inStream)
		{
			LogError("Failed to open file \"%s\" for reading", fullPath.string().c_str());
			return false;
		}
		auto size = boost::filesystem::file_size(fullPath);
		LogInfo("Reading %llu bytes from \"%s\"", (unsigned long long)size,
		        fullPath.string().c_str());
		up<char[]> data(new char[size]);

		inStream.read(data.get(), size);

		if (!inStream)
		{
			LogError("Failed to read %llu bytes from \"%s\"", (unsigned long long)size,
			         fullPath.string().c_str());
			return false;
		}

		std::string fileContents(data.get(), size);

		auto sha1Sum = calculate_checksum(fileContents);
		auto expectedSha1Sum = p.second;
		if (expectedSha1Sum != "")
		{
			if (sha1Sum != expectedSha1Sum)
			{
				LogError("File \"%s\" has incorrect checksum \"%s\", expected \"%s\"",
				         fullPath.string().c_str(), sha1Sum.c_str(), expectedSha1Sum.c_str());
				// Don't return false, as we can allow the user to continue?
			}
		}
		else
		{
			LogWarning("Skipping missing checksum for file \"%s\"", fullPath.string().c_str());
		}
		contents[p.first] = std::move(fileContents);
	}

	return true;
}

static bool writePack(const UString &name, const std::map<UString, std::string> &contents)
{
	mz_zip_archive archive = {0};

	auto path = name + ".zip";

	bool ret = false;

	if (!mz_zip_writer_init_file(&archive, path.c_str(), 0))
	{
		LogError("Failed to init zip file \"%s\" for writing", path.c_str());
		return false;
	}

	for (auto &p : contents)
	{
		if (!mz_zip_writer_add_mem(&archive, p.first.c_str(), p.second.c_str(), p.second.length(),
		                           MZ_DEFAULT_COMPRESSION))
		{
			LogError("Failed to insert \"%s\" into zip file \"%s\"", p.first.c_str(), path.c_str());
			goto err;
		}
	}

	if (!mz_zip_writer_finalize_archive(&archive))
	{
		LogError("Failed to finalize archive \"%s\"", path.c_str());
		goto err;
	}

	ret = true;

err:
	mz_zip_writer_end(&archive);
	return ret;
}

static bool readPack(const UString &name, std::map<UString, std::string> &contents)
{
	mz_zip_archive archive = {0};

	auto path = name + ".zip";

	bool ret = false;

	if (!mz_zip_reader_init_file(&archive, path.c_str(), 0))
	{
		LogError("Failed to init zip file \"%s\" for reading", path.c_str());
		return false;
	}

	unsigned fileCount = mz_zip_reader_get_num_files(&archive);

	for (unsigned idx = 0; idx < fileCount; idx++)
	{
		unsigned filenameLength = mz_zip_reader_get_filename(&archive, idx, nullptr, 0);

		up<char[]> data(new char[filenameLength]);
		mz_zip_reader_get_filename(&archive, idx, data.get(), filenameLength);
		std::string filename(data.get());

		mz_zip_archive_file_stat stat = {0};

		if (!mz_zip_reader_file_stat(&archive, idx, &stat))
		{
			LogError("Failed to stat file \"%s\" in zip \"%s\"", filename.c_str(), path.c_str());
			goto err;
		}
		if (stat.m_uncomp_size == 0)
		{
			LogInfo("Skipping %s - possibly a directory?", filename.c_str());
			continue;
		}

		LogInfo("Reading %lu bytes for file \"%s\" in zip \"%s\"",
		        (unsigned long)stat.m_uncomp_size, filename.c_str(), path.c_str());

		data.reset(new char[stat.m_uncomp_size]);

		if (!mz_zip_reader_extract_to_mem(&archive, idx, data.get(), stat.m_uncomp_size, 0))
		{
			LogError("Failed to extract file \"%s\" in zip \"%s\"", filename.c_str(), path.c_str());
			goto err;
		}

		std::string filecontents(data.get(), stat.m_uncomp_size);

		contents[filename] = std::move(filecontents);
	}

	ret = true;

err:
	mz_zip_reader_end(&archive);
	return ret;
}

using namespace pugi;

class XMLSerializationArchive : public SerializationArchive,
                                public std::enable_shared_from_this<XMLSerializationArchive>
{
  private:
	std::map<UString, xml_document> docRoots;
	friend class SerializationArchive;

  public:
	sp<SerializationNode> newRoot(const UString &prefix, const UString &name) override;
	sp<SerializationNode> getRoot(const UString &prefix, const UString &name) override;
	bool write(const UString &path, bool pack) override;
	~XMLSerializationArchive() override = default;
};

class XMLSerializationNode : public SerializationNode
{
  private:
	sp<XMLSerializationArchive> archive;
	xml_node node;
	sp<XMLSerializationNode> parent;
	UString prefix;

  public:
	XMLSerializationNode(sp<XMLSerializationArchive> archive, xml_node node,
	                     sp<XMLSerializationNode> parent)
	    : archive(archive), node(node), parent(parent){};

	XMLSerializationNode(sp<XMLSerializationArchive> archive, xml_node node, const UString &prefix)
	    : archive(archive), node(node), parent(nullptr), prefix(prefix){};

	sp<SerializationNode> addNode(const UString &name, const UString &value = "") override;
	sp<SerializationNode> addSection(const UString &name) override;

	sp<SerializationNode> getNodeOpt(const UString &name) override;
	sp<SerializationNode> getNextSiblingOpt(const UString &name) override;
	sp<SerializationNode> getSectionOpt(const UString &name) override;

	UString getName() override;
	void setName(const UString &str) override;
	UString getValue() override;
	void setValue(const UString &str) override;

	unsigned int getValueUInt() override;
	void setValueUInt(unsigned int i) override;

	unsigned char getValueUChar() override;
	void setValueUChar(unsigned char i) override;

	int getValueInt() override;
	void setValueInt(int i) override;

	float getValueFloat() override;
	void setValueFloat(float f) override;

	bool getValueBool() override;
	void setValueBool(bool b) override;

	std::vector<bool> getValueBoolVector() override;
	void setValueBoolVector(const std::vector<bool> &vec) override;

	UString getFullPath() override;
	const UString &getPrefix() const override
	{
		if (this->parent)
			return this->parent->getPrefix();
		else
			return this->prefix;
	}

	~XMLSerializationNode() override = default;
};

sp<SerializationArchive> SerializationArchive::createArchive()
{
	return std::make_shared<XMLSerializationArchive>();
}

sp<SerializationArchive> SerializationArchive::readArchive(const UString &name)
{
	std::map<UString, std::string> contents;
	if (readDir(name, contents))
	{
		LogInfo("Reading directory \"%s\"", name.c_str());
	}
	else if (readPack(name, contents))
	{
		LogInfo("Reading pack \"%s\"", name.c_str());
	}
	else
	{
		LogInfo("Failed to find archive at \"%s\"", name.c_str());
		return nullptr;
	}

	auto archive = std::make_shared<XMLSerializationArchive>();
	for (auto &pair : contents)
	{
		// FIXME: Make this actually read from the root and load the xinclude tags properly?
		auto &doc = archive->docRoots[pair.first];
		auto parse_result = doc.load_string(pair.second.c_str());
		if (!parse_result)
		{
			LogError("Failed to parse \"%s\" : \"%s\" at \"%llu\"", pair.first.c_str(),
			         parse_result.description(), (unsigned long long)parse_result.offset);
			return nullptr;
		}
		LogInfo("Parsed \"%s\"", pair.first.c_str());
	}
	return archive;
}

sp<SerializationNode> XMLSerializationArchive::newRoot(const UString &prefix, const UString &name)
{
	auto path = prefix + name + ".xml";
	auto root = this->docRoots[path].root().append_child();
	auto decl = this->docRoots[path].prepend_child(pugi::node_declaration);
	decl.append_attribute("version") = "1.0";
	decl.append_attribute("encoding") = "UTF-8";
	root.set_name(name.c_str());
	return std::make_shared<XMLSerializationNode>(shared_from_this(), root, prefix + name + "/");
}

sp<SerializationNode> XMLSerializationArchive::getRoot(const UString &prefix, const UString &name)
{
	auto path = prefix + name + ".xml";
	auto it = this->docRoots.find(path);
	if (it == this->docRoots.end())
	{
		LogInfo("No root file named \"%s\"", path.c_str());
		return nullptr;
	}
	auto root = it->second.child(name.c_str());
	if (!root)
	{
		LogWarning("Failed to find root with name \"%s\" in \"%s\"", name.c_str(), path.c_str());
		return nullptr;
	}
	return std::make_shared<XMLSerializationNode>(shared_from_this(), root, prefix + name + "/");
}

bool XMLSerializationArchive::write(const UString &path, bool pack)
{
	std::map<UString, std::string> contents;

	for (auto &root : this->docRoots)
	{
		std::stringstream ss;
		root.second.save(ss, "  ");
		contents[root.first] = ss.str();
	}

	if (pack)
		return writePack(path, contents);
	else
		return writeDir(path, contents);
}

sp<SerializationNode> XMLSerializationNode::addNode(const UString &name, const UString &value)
{
	auto newNode = this->node.append_child();
	newNode.set_name(name.c_str());
	newNode.set_value(value.c_str());
	return std::make_shared<XMLSerializationNode>(
	    this->archive, newNode, std::static_pointer_cast<XMLSerializationNode>(shared_from_this()));
}

sp<SerializationNode> XMLSerializationNode::getNodeOpt(const UString &name)
{
	auto newNode = this->node.child(name.c_str());
	if (!newNode)
	{
		return nullptr;
	}
	return std::make_shared<XMLSerializationNode>(
	    this->archive, newNode, std::static_pointer_cast<XMLSerializationNode>(shared_from_this()));
}

sp<SerializationNode> XMLSerializationNode::getNextSiblingOpt(const UString &name)
{
	auto newNode = this->node.next_sibling(name.c_str());
	if (!newNode)
	{
		return nullptr;
	}
	return std::make_shared<XMLSerializationNode>(this->archive, newNode, this->parent);
}

sp<SerializationNode> XMLSerializationNode::addSection(const UString &name)
{
	auto includeNode =
	    std::static_pointer_cast<XMLSerializationNode>(this->addNode(UString{"xi:include"}));
	auto path = name + ".xml";
	auto nsAttribute = includeNode->node.append_attribute("xmlns:xi");
	nsAttribute.set_value("http://www.w3.org/2001/XInclude");
	auto attribute = includeNode->node.append_attribute("href");
	attribute.set_value(path.c_str());
	return this->archive->newRoot(this->getPrefix(), name);
}

sp<SerializationNode> XMLSerializationNode::getSectionOpt(const UString &name)
{
	return archive->getRoot(this->getPrefix(), name);
}

UString XMLSerializationNode::getName() { return node.name(); }

void XMLSerializationNode::setName(const UString &str) { node.set_name(str.c_str()); }

UString XMLSerializationNode::getValue() { return node.text().get(); }

void XMLSerializationNode::setValue(const UString &str) { node.text().set(str.c_str()); }

unsigned int XMLSerializationNode::getValueUInt() { return node.text().as_uint(); }

void XMLSerializationNode::setValueUInt(unsigned int i) { node.text().set(i); }

unsigned char XMLSerializationNode::getValueUChar()
{
	auto uint = node.text().as_uint();
	if (uint > std::numeric_limits<unsigned char>::max())
	{
		throw SerializationException(
		    UString::format("Value %u is out of range of unsigned char type", uint),
		    shared_from_this());
	}
	return static_cast<unsigned char>(uint);
}

void XMLSerializationNode::setValueUChar(unsigned char c) { node.text().set((unsigned int)c); }

int XMLSerializationNode::getValueInt() { return node.text().as_int(); }

void XMLSerializationNode::setValueInt(int i) { node.text().set(i); }

float XMLSerializationNode::getValueFloat() { return node.text().as_float(); }

void XMLSerializationNode::setValueFloat(float f) { node.text().set(f); }

bool XMLSerializationNode::getValueBool() { return node.text().as_bool(); }

void XMLSerializationNode::setValueBool(bool b) { node.text().set(b); }

std::vector<bool> XMLSerializationNode::getValueBoolVector()
{
	std::vector<bool> vec;
	auto string = this->getValue().str();

	vec.resize(string.length());
	for (size_t i = 0; i < string.length(); i++)
	{
		auto c = string[i];
		if (c == '1')
			vec[i] = true;
		else if (c == '0')
			vec[i] = false;
		else
			throw SerializationException(UString::format("Unknown char '%c' in bool vector", c),
			                             shared_from_this());
	}
	return vec;
}

void XMLSerializationNode::setValueBoolVector(const std::vector<bool> &vec)
{
	std::string str(vec.size(), ' ');

	for (size_t i = 0; i < vec.size(); i++)
	{
		auto b = vec[i];
		if (b)
			str[i] = '1';
		else
			str[i] = '0';
	}
	this->setValue(str);
}

UString XMLSerializationNode::getFullPath()
{
	UString str;
	if (this->parent)
	{
		str = this->parent->getFullPath();
	}
	else
	{
		str += node.name();
		str += ".xml:";
	}
	str += "/";
	str += node.name();
	return str;
}

} // namespace OpenApoc
