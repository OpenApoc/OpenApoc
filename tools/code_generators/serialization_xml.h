#pragma once
// Disable automatic #pragma linking for boost - only enabled in msvc and that should provide boost
// symbols as part of the module that uses it
#include "dependencies/pugixml/src/pugixml.hpp"
#include <fstream>
#include <iostream>
#include <list>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION >= 106600
#include <boost/uuid/detail/sha1.hpp>
#else
#include <boost/uuid/sha1.hpp>
#endif

enum class NodeType
{
	Normal,
	Section,
	SectionMap,
};

class SerializeNode
{
  public:
	std::string name;
	NodeType type = NodeType::Normal;
	SerializeNode(std::string name) : name(name) {}
};

class SerializeObject
{
  public:
	std::string name;
	std::list<std::pair<std::string, SerializeNode>> members;
	SerializeObject(std::string name) : name(name) {}
	bool external = false;
	bool full = false;
};

class SerializeEnum
{
  public:
	std::string name;
	bool external = false;
	std::list<std::string> values;
};

class StateDefinition
{
  public:
	std::vector<SerializeObject> objects;
	std::vector<SerializeEnum> enums;
	std::string hashString;
};

bool readXml(std::istream &in, StateDefinition &state);
