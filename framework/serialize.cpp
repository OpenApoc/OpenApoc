#define PUGIXML_NO_XPATH
#define PUGIXML_HEADER_ONLY
#include "dependencies/pugixml/src/pugixml.hpp"

#include "framework/logger.h"
#include "library/sp.h"
#include "library/strings.h"

#include "framework/serialize.h"

//#define MINIZ_HEADER_FILE_ONLY
#include "dependencies/miniz/miniz.c"

#include <map>
#include <iostream>

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

static bool writePack(const UString &name, const std::map<UString, std::string> &contents)
{
	mz_zip_archive archive = {0};

	auto path = name + ".zip";

	bool ret = false;

	int idx = 0;

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

	int idx = 0;

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
	bool write(const UString &path) override;
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
	if (!readPack(name, contents))
	{
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
			LogError("Failed to parse \"%s\" : \"%s\" at \"%zu\"", pair.first.c_str(),
			         parse_result.description(), parse_result.offset);
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

bool XMLSerializationArchive::write(const UString &path)
{
	std::map<UString, std::string> contents;

	for (auto &root : this->docRoots)
	{
		std::stringstream ss;
		root.second.save(ss);
		contents[root.first] = ss.str();
	}

	return writePack(path, contents);
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
	auto attribute = includeNode->node.append_attribute("href");
	attribute.set_value(path.c_str());
	auto nsAttribute = includeNode->node.append_attribute("xmlns:xi");
	nsAttribute.set_value("http://www.w3.org/2001/XInclude");
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
