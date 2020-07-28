#include "framework/serialization/serialize.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "framework/filesystem.h"
#include "framework/logger.h"
#include "framework/serialization/providers/filedataprovider.h"
#include "framework/serialization/providers/providerwithchecksum.h"
#include "framework/serialization/providers/zipdataprovider.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/strings_format.h"
#include <deque>
#include <map>
#include <sstream>

namespace OpenApoc
{

SerializationNode *SerializationNode::getNodeReq(const char *name)
{
	auto node = this->getNodeOpt(name);
	if (!node)
	{
		throw SerializationException(format("Missing node \"%s\"", name), this);
	}
	return node;
}

SerializationNode *SerializationNode::getSectionReq(const char *name)
{
	auto node = this->getSectionOpt(name);
	if (!node)
	{
		throw SerializationException(format("Missing section \"%s\"", name), this);
	}
	return node;
}

SerializationNode *SerializationNode::getNextSiblingReq(const char *name)
{
	auto node = this->getNextSiblingOpt(name);
	if (!node)
	{
		throw SerializationException(format("Missing sibling of \"%s\"", name), this);
	}
	return node;
}

using namespace pugi;
class XMLSerializationArchive;

class XMLSerializationNode final : public SerializationNode
{
  private:
	XMLSerializationArchive *archive;
	xml_node node;
	XMLSerializationNode *parent;
	const UString *prefix;

  public:
	XMLSerializationNode(XMLSerializationArchive *archive, xml_node node,
	                     XMLSerializationNode *parent)
	    : archive(archive), node(node), parent(parent), prefix(nullptr)
	{
	}

	XMLSerializationNode(XMLSerializationArchive *archive, xml_node node, const UString *prefix)
	    : archive(archive), node(node), parent(nullptr), prefix(prefix)
	{
	}

	SerializationNode *addNode(const char *name, const UString &value = "") override;
	SerializationNode *addSection(const char *name) override;

	SerializationNode *getNodeOpt(const char *name) override;
	SerializationNode *getNextSiblingOpt(const char *name) override;
	SerializationNode *getSectionOpt(const char *name) override;

	UString getName() override;
	void setName(const UString &str) override;
	UString getAttribute(const UString &attribute) override;
	void setAttribute(const UString &attribute, const UString &value) override;
	UString getValue() override;
	void setValue(const UString &str) override;

	unsigned int getValueUInt() override;
	void setValueUInt(unsigned int i) override;

	unsigned char getValueUChar() override;
	void setValueUChar(unsigned char i) override;

	int getValueInt() override;
	void setValueInt(int i) override;

	unsigned long long getValueUInt64() override;
	void setValueUInt64(unsigned long long i) override;

	long long getValueInt64() override;
	void setValueInt64(long long i) override;

	float getValueFloat() override;
	void setValueFloat(float f) override;

	bool getValueBool() override;
	void setValueBool(bool b) override;

	std::vector<bool> getValueBoolVector() override;
	void setValueBoolVector(const std::vector<bool> &vec) override;

	UString getFullPath() override;
	const UString &getPrefix() const override
	{
		static const UString emptyString = "";
		if (this->parent)
			return this->parent->getPrefix();
		else if (this->prefix)
			return *this->prefix;
		else
			return emptyString;
	}

	~XMLSerializationNode() override = default;
};

class XMLSerializationArchive final : public SerializationArchive
{
  private:
	up<SerializationDataProvider> dataProvider;
	std::map<UString, xml_document> docRoots;
	std::deque<XMLSerializationNode> nodes;
	friend class SerializationArchive;
	friend class XMLSerializationNode;
	std::deque<UString> prefixes;

  public:
	SerializationNode *newRoot(const UString &prefix, const char *name) override;
	SerializationNode *getRoot(const UString &prefix, const char *name) override;
	bool write(const UString &path, bool pack, bool pretty) override;
	XMLSerializationArchive() : dataProvider(nullptr), docRoots(){};
	XMLSerializationArchive(up<SerializationDataProvider> dataProvider)
	    : dataProvider(std::move(dataProvider)){};
	~XMLSerializationArchive() override = default;
};

up<SerializationArchive> SerializationArchive::createArchive()
{
	return mkup<XMLSerializationArchive>();
}

up<SerializationDataProvider> getProvider(bool pack)
{
	if (!pack)
	{
		// directory loader
		return mkup<ProviderWithChecksum>(mkup<FileDataProvider>());
	}
	else
	{
		// zip loader
		return mkup<ProviderWithChecksum>(mkup<ZipDataProvider>());
	}
}

up<SerializationArchive> SerializationArchive::readArchive(const UString &path)
{
	up<SerializationDataProvider> dataProvider = getProvider(!fs::is_directory(path));
	if (!dataProvider->openArchive(path, false))
	{
		LogWarning("Failed to open archive at \"%s\"", path);
		return nullptr;
	}
	LogInfo("Opened archive \"%s\"", path);

	return mkup<XMLSerializationArchive>(std::move(dataProvider));
}

SerializationNode *XMLSerializationArchive::newRoot(const UString &prefix, const char *name)
{
	auto path = prefix + name + ".xml";
	auto root = this->docRoots[path].root().append_child();
	auto decl = this->docRoots[path].prepend_child(pugi::node_declaration);
	decl.append_attribute("version") = "1.0";
	decl.append_attribute("encoding") = "UTF-8";
	root.set_name(name);
	const UString *newPrefix = &this->prefixes.emplace_back(prefix + name + "/");
	return &this->nodes.emplace_back(this, root, newPrefix);
}

SerializationNode *XMLSerializationArchive::getRoot(const UString &prefix, const char *name)
{
	auto path = prefix + name + ".xml";
	if (dataProvider == nullptr)
	{
		LogWarning("Reading from not opened archive: %s!", path);
		return nullptr;
	}

	auto it = this->docRoots.find(path);
	if (it == this->docRoots.end())
	{
		UString content;
		if (dataProvider->readDocument(path, content))
		{
			// FIXME: Make this actually read from the root and load the xinclude tags properly?
			auto &doc = this->docRoots[path];
			auto parse_result = doc.load_string(content.c_str());
			if (!parse_result)
			{
				LogInfo("Failed to parse \"%s\" : \"%s\" at \"%llu\"", path,
				        parse_result.description(), (unsigned long long)parse_result.offset);
				return nullptr;
			}
			it = this->docRoots.find(path);
			LogInfo("Parsed \"%s\"", path);
		}
	}
	if (it == this->docRoots.end())
	{
		return nullptr;
	}

	auto root = it->second.child(name);
	if (!root)
	{
		LogWarning("Failed to find root with name \"%s\" in \"%s\"", name, path);
		return nullptr;
	}
	const UString *newPrefix = &this->prefixes.emplace_back(prefix + name + "/");
	return &this->nodes.emplace_back(this, root, newPrefix);
}

bool XMLSerializationArchive::write(const UString &path, bool pack, bool pretty)
{
	// warning! data provider must be freed when this method ends,
	// so code calling this method may override archive
	auto dataProvider = getProvider(pack);
	if (!dataProvider->openArchive(path, true))
	{
		LogWarning("Failed to open archive at \"%s\"", path);
		return false;
	}

	for (auto &root : this->docRoots)
	{
		std::stringstream ss;
		unsigned int flags = pugi::format_default;
		const char *indent = "  ";
		if (pretty == false)
		{
			flags = pugi::format_raw;
			indent = "";
		}
		root.second.save(ss, indent, flags);
		if (!dataProvider->saveDocument(root.first, ss.str()))
		{
			return false;
		}
	}

	return dataProvider->finalizeSave();
}

SerializationNode *XMLSerializationNode::addNode(const char *name, const UString &value)
{
	auto newNode = this->node.append_child();
	newNode.set_name(name);
	newNode.text().set(value.c_str());
	return &this->archive->nodes.emplace_back(this->archive, newNode, this);
}

SerializationNode *XMLSerializationNode::getNodeOpt(const char *name)
{
	auto newNode = this->node.child(name);
	if (!newNode)
	{
		return nullptr;
	}
	return &this->archive->nodes.emplace_back(this->archive, newNode, this);
}

SerializationNode *XMLSerializationNode::getNextSiblingOpt(const char *name)
{
	auto newNode = this->node.next_sibling(name);
	if (!newNode)
	{
		return nullptr;
	}
	return &this->archive->nodes.emplace_back(this->archive, newNode, this);
}

SerializationNode *XMLSerializationNode::addSection(const char *name)
{
	auto includeNode = static_cast<XMLSerializationNode *>(this->addNode("xi:include"));
	auto path = UString(name) + ".xml";
	auto nsAttribute = includeNode->node.append_attribute("xmlns:xi");
	nsAttribute.set_value("http://www.w3.org/2001/XInclude");
	auto attribute = includeNode->node.append_attribute("href");
	attribute.set_value(path.c_str());
	return this->archive->newRoot(this->getPrefix(), name);
}

SerializationNode *XMLSerializationNode::getSectionOpt(const char *name)
{
	return archive->getRoot(this->getPrefix(), name);
}

UString XMLSerializationNode::getName() { return node.name(); }

void XMLSerializationNode::setName(const UString &str) { node.set_name(str.c_str()); }

UString XMLSerializationNode::getValue() { return node.text().get(); }

void XMLSerializationNode::setValue(const UString &str) { node.text().set(str.c_str()); }

UString XMLSerializationNode::getAttribute(const UString &attribute)
{
	return node.attribute(attribute.c_str()).value();
}

void XMLSerializationNode::setAttribute(const UString &attribute, const UString &value)
{
	node.attribute(attribute.c_str()).set_value(value.c_str());
}

unsigned int XMLSerializationNode::getValueUInt() { return node.text().as_uint(); }

void XMLSerializationNode::setValueUInt(unsigned int i) { node.text().set(i); }

unsigned char XMLSerializationNode::getValueUChar()
{
	auto uint = node.text().as_uint();
	if (uint > std::numeric_limits<unsigned char>::max())
	{
		throw SerializationException(format("Value %u is out of range of unsigned char type", uint),
		                             this);
	}
	return static_cast<unsigned char>(uint);
}

void XMLSerializationNode::setValueUChar(unsigned char c) { node.text().set((unsigned int)c); }

int XMLSerializationNode::getValueInt() { return node.text().as_int(); }

void XMLSerializationNode::setValueInt(int i) { node.text().set(i); }

unsigned long long XMLSerializationNode::getValueUInt64() { return node.text().as_ullong(); }

void XMLSerializationNode::setValueUInt64(unsigned long long i) { node.text().set(i); }

long long XMLSerializationNode::getValueInt64() { return node.text().as_llong(); }

void XMLSerializationNode::setValueInt64(long long i) { node.text().set(i); }

float XMLSerializationNode::getValueFloat() { return node.text().as_float(); }

void XMLSerializationNode::setValueFloat(float f) { node.text().set(f); }

bool XMLSerializationNode::getValueBool() { return node.text().as_bool(); }

void XMLSerializationNode::setValueBool(bool b) { node.text().set(b); }

std::vector<bool> XMLSerializationNode::getValueBoolVector()
{
	std::vector<bool> vec;
	auto string = this->getValue();

	vec.resize(string.length());
	for (size_t i = 0; i < string.length(); i++)
	{
		auto c = string[i];
		if (c == '1')
			vec[i] = true;
		else if (c == '0')
			vec[i] = false;
		else
			throw SerializationException(format("Unknown char '%c' in bool vector", c), this);
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
