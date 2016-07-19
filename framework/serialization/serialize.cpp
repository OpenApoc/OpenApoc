#define PUGIXML_NO_XPATH
#define PUGIXML_HEADER_ONLY
#include "dependencies/pugixml/src/pugixml.hpp"

#include "framework/logger.h"
#include "library/sp.h"
#include "library/strings.h"

#include "framework/serialization/providers/filedataprovider.h"
#include "framework/serialization/providers/providerwithchecksum.h"
#include "framework/serialization/providers/zipdataprovider.h"
#include "framework/serialization/serialize.h"

// Disable automatic #pragma linking for boost - only enabled in msvc and that should provide boost
// symbols as part of the module that uses it
#define BOOST_ALL_NO_LIB
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
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

using namespace pugi;

class XMLSerializationArchive : public SerializationArchive,
                                public std::enable_shared_from_this<XMLSerializationArchive>
{
  private:
	sp<SerializationDataProvider> dataProvider;
	std::map<UString, xml_document> docRoots;
	friend class SerializationArchive;

  public:
	sp<SerializationNode> newRoot(const UString &prefix, const UString &name) override;
	sp<SerializationNode> getRoot(const UString &prefix, const UString &name) override;
	bool write(const UString &path, bool pack) override;
	XMLSerializationArchive(){};
	XMLSerializationArchive(const sp<SerializationDataProvider> dataProvider)
	    : dataProvider(dataProvider){};
	~XMLSerializationArchive() override = default;
};

class XMLSerializationNode : public SerializationNode
{
  private:
	sp<SerializationDataProvider> dataProvider;
	sp<XMLSerializationArchive> archive;
	xml_node node;
	sp<XMLSerializationNode> parent;
	UString prefix;

  public:
	XMLSerializationNode(sp<XMLSerializationArchive> archive, xml_node node,
	                     sp<XMLSerializationNode> parent)
	    : archive(archive), node(node), parent(parent)
	{
	}

	XMLSerializationNode(sp<XMLSerializationArchive> archive, xml_node node, const UString &prefix)
	    : archive(archive), node(node), prefix(prefix)
	{
	}

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

sp<SerializationDataProvider> getProvider(bool pack)
{
	if (!pack)
	{
		// directory loader
		return std::static_pointer_cast<SerializationDataProvider>(
		    mksp<ProviderWithChecksum>(mksp<FileDataProvider>()));
	}
	else
	{
		// zip loader
		return std::static_pointer_cast<SerializationDataProvider>(
		    mksp<ProviderWithChecksum>(mksp<ZipDataProvider>()));
	}
}

sp<SerializationArchive> SerializationArchive::readArchive(const UString &name)
{
	sp<SerializationDataProvider> dataProvider =
	    getProvider(!boost::filesystem::is_directory(name.str()));
	if (!dataProvider->openArchive(name, false))
	{
		LogWarning("Failed to open archive at \"%s\"", name.c_str());
		return nullptr;
	}
	LogInfo("Opened archive \"%s\"", name.c_str());

	return mksp<XMLSerializationArchive>(dataProvider);
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
	if (dataProvider == nullptr)
	{
		LogWarning("Reading from not opened archive: %s!", path.c_str());
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
				LogError("Failed to parse \"%s\" : \"%s\" at \"%llu\"", path.c_str(),
				         parse_result.description(), (unsigned long long)parse_result.offset);
				return nullptr;
			}
			it = this->docRoots.find(path);
			LogInfo("Parsed \"%s\"", path.c_str());
		}
	}
	if (it == this->docRoots.end())
	{
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
	// warning! data provider must be freed when this method ends,
	// so code calling this method may override archive
	auto dataProvider = getProvider(pack);
	if (!dataProvider->openArchive(path, true))
	{
		LogWarning("Failed to open archive at \"%s\"", path.c_str());
		return false;
	}

	for (auto &root : this->docRoots)
	{
		std::stringstream ss;
		root.second.save(ss, "  ");
		if (!dataProvider->saveDocument(root.first, ss.str()))
		{
			return false;
		}
	}

	return dataProvider->finalizeSave();
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
