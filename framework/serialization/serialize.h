#pragma once

#include "library/sp.h"
#include "library/strings.h"
#include <stdexcept>
#include <vector>

namespace OpenApoc
{
class SerializationNode
{
  public:
	virtual SerializationNode *addNode(const char *name, const UString &value = "") = 0;
	virtual SerializationNode *addSection(const char *name) = 0;

	// Opt versions may return nullptr (they're 'optional'), Req gets throw an exception if
	// missing
	virtual SerializationNode *getNodeReq(const char *name);
	virtual SerializationNode *getNodeOpt(const char *name) = 0;
	virtual SerializationNode *getNextSiblingReq(const char *name);
	virtual SerializationNode *getNextSiblingOpt(const char *name) = 0;
	virtual SerializationNode *getSectionReq(const char *name);
	virtual SerializationNode *getSectionOpt(const char *name) = 0;

	SerializationNode *getNode(const char *name) { return this->getNodeOpt(name); }
	SerializationNode *getNextSibling(const char *name) { return this->getNextSiblingOpt(name); }
	SerializationNode *getSection(const char *name) { return this->getSectionOpt(name); }

	virtual UString getName() = 0;
	virtual void setName(const UString &str) = 0;
	virtual UString getAttribute(const UString &attribute) = 0;
	virtual void setAttribute(const UString &attribute, const UString &value) = 0;
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
	static up<SerializationArchive> createArchive();
	static up<SerializationArchive> readArchive(const UString &path);

	virtual SerializationNode *newRoot(const UString &prefix, const char *name) = 0;
	virtual SerializationNode *getRoot(const UString &prefix, const char *name) = 0;
	virtual bool write(const UString &path, bool pack = true, bool pretty = false) = 0;
	virtual ~SerializationArchive() = default;
};

class SerializationException : public std::runtime_error
{
  public:
	SerializationException(const UString &description, SerializationNode *node)
	    : std::runtime_error(UString(description + " " + node->getFullPath()).c_str())
	{
	}
};

} // namespace OpenApoc
