#pragma once

#include "library/sp.h"
#include "library/strings.h"
#include <stdexcept>
#include <vector>

namespace OpenApoc
{
class SerializationNode : public std::enable_shared_from_this<SerializationNode>
{
  public:
	sp<SerializationNode> virtual addNode(const UString &name, const UString &value = "") = 0;
	sp<SerializationNode> virtual addSection(const UString &name) = 0;

	// Opt versions may return nullptr (they're 'optional'), Req gets throw an exception if
	// missing
	sp<SerializationNode> virtual getNodeReq(const UString &name);
	sp<SerializationNode> virtual getNodeOpt(const UString &name) = 0;
	sp<SerializationNode> virtual getNextSiblingReq(const UString &name);
	sp<SerializationNode> virtual getNextSiblingOpt(const UString &name) = 0;
	sp<SerializationNode> virtual getSectionReq(const UString &name);
	sp<SerializationNode> virtual getSectionOpt(const UString &name) = 0;

	sp<SerializationNode> getNode(const UString &name) { return this->getNodeOpt(name); }
	sp<SerializationNode> getNextSibling(const UString &name)
	{
		return this->getNextSiblingOpt(name);
	}
	sp<SerializationNode> getSection(const UString &name) { return this->getSectionOpt(name); }

	virtual UString getName() = 0;
	virtual void setName(const UString &str) = 0;
	virtual UString getValue() = 0;
	virtual void setValue(const UString &str) = 0;

	virtual unsigned int getValueUInt() = 0;
	virtual void setValueUInt(unsigned int i) = 0;

	virtual unsigned char getValueUChar() = 0;
	virtual void setValueUChar(unsigned char c) = 0;

	virtual int getValueInt() = 0;
	virtual void setValueInt(int i) = 0;

	virtual unsigned long long getValueUInt64() = 0;
	virtual void setValueUInt64(unsigned long long i) = 0;

	virtual long long getValueInt64() = 0;
	virtual void setValueInt64(long long i) = 0;

	virtual float getValueFloat() = 0;
	virtual void setValueFloat(float f) = 0;

	virtual bool getValueBool() = 0;
	virtual void setValueBool(bool b) = 0;

	// std::vector<bool> is handled specially
	virtual std::vector<bool> getValueBoolVector() = 0;
	virtual void setValueBoolVector(const std::vector<bool> &v) = 0;

	virtual UString getFullPath() = 0;
	virtual const UString &getPrefix() const = 0;

	virtual ~SerializationNode() = default;
};

class SerializationArchive
{
  public:
	static sp<SerializationArchive> createArchive();
	static sp<SerializationArchive> readArchive(const UString &path);

	sp<SerializationNode> virtual newRoot(const UString &prefix, const UString &name) = 0;
	sp<SerializationNode> virtual getRoot(const UString &prefix, const UString &name) = 0;
	bool virtual write(const UString &path, bool pack = true) = 0;
	virtual ~SerializationArchive() = default;
};

class SerializationException : public std::runtime_error
{
  public:
	sp<SerializationNode> node;
	SerializationException(const UString &description, sp<SerializationNode> node)
	    : std::runtime_error(description.cStr()), node(node)
	{
	}
};

} // namespace OpenApoc
