#include "dependencies/pugixml/src/pugixml.hpp"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>

// Disable automatic #pragma linking for boost - only enabled in msvc and that should provide boost
// symbols as part of the module that uses it
#define BOOST_ALL_NO_LIB
#include <boost/program_options.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION >= 106600
#include <boost/uuid/detail/sha1.hpp>
#else
#include <boost/uuid/sha1.hpp>
#endif

namespace po = boost::program_options;

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

bool readXml(std::istream &in, StateDefinition &state)
{
	pugi::xml_document doc;

	in.seekg(0, std::ios::end);
	auto fileSize = in.tellg();
	in.seekg(0, std::ios::beg);

	std::unique_ptr<char[]> fileData(new char[fileSize]);
	in.read(fileData.get(), fileSize);
	if (!in)
	{
		std::cerr << "Failed to read " << fileSize << "bytes from input file\n";
		return false;
	}

	boost::uuids::detail::sha1 sha;
	sha.reset();
	sha.process_bytes(fileData.get(), fileSize);
	unsigned int hashValue[5];
	sha.get_digest(hashValue);

	for (int i = 0; i < 5; i++)
	{
		unsigned int v = hashValue[i];
		for (int j = 0; j < 4; j++)
		{
			// FIXME: Probably need to do the reverse for big endian?
			char stringBuf[3];
			unsigned int byteHex = v & 0xff000000;
			byteHex >>= 24;
			snprintf(stringBuf, sizeof(stringBuf), "%02x", byteHex);
			state.hashString += stringBuf;
			v <<= 8;
		}
	}

	auto ret = doc.load_buffer(fileData.get(), fileSize);
	if (!ret)
	{
		std::cerr << "Failed to parse file:" << ret.description() << "\n";
		return false;
	}

	auto rootNode = doc.first_child();
	while (rootNode)
	{
		if (std::string(rootNode.name()) == "openapoc_gamestate")
		{
			auto objectNode = rootNode.first_child();
			while (objectNode)
			{
				if (std::string(objectNode.name()) == "object")
				{
					SerializeObject obj("");
					auto externalAttr = objectNode.attribute("external");
					if (!externalAttr.empty())
					{
						std::string externalValue = externalAttr.as_string();
						if (externalValue == "true")
						{
							obj.external = true;
						}
						else
						{
							std::cerr << "Unknown object external attribute \"" << externalValue
							          << "\"\n";
						}
					}
					auto fullAttr = objectNode.attribute("full");
					if (!fullAttr.empty())
					{
						std::string fullValue = fullAttr.as_string();
						if (fullValue == "true")
						{
							obj.full = true;
						}
						else
						{
							std::cerr << "Unknown object full attribute \"" << fullValue << "\"\n";
						}
					}
					auto memberNode = objectNode.first_child();
					while (memberNode)
					{
						if (std::string(memberNode.name()) == "member")
						{
							std::string memberName = memberNode.text().as_string();
							NodeType type = NodeType::Normal;
							auto typeAttr = memberNode.attribute("type");
							if (!typeAttr.empty())
							{
								std::string typeName = typeAttr.as_string();
								if (typeName == "Normal")
									type = NodeType::Normal;
								else if (typeName == "Section")
									type = NodeType::Section;
								else if (typeName == "SectionMap")
									type = NodeType::SectionMap;
								else
									std::cerr << "Unknown member type attribute \"" << typeName
									          << "\"\n";
							}
							SerializeNode member(memberName);
							member.type = type;
							obj.members.push_back(std::make_pair(member.name, member));
						}
						else if (std::string(memberNode.name()) == "name")
						{
							obj.name = memberNode.text().as_string();
						}
						memberNode = memberNode.next_sibling();
					}
					state.objects.push_back(obj);
				}
				else if (std::string(objectNode.name()) == "enum")
				{
					SerializeEnum sEnum;
					auto externalAttr = objectNode.attribute("external");
					if (!externalAttr.empty())
					{
						std::string externalValue = externalAttr.as_string();
						if (externalValue == "true")
						{
							sEnum.external = true;
						}
						else
						{
							std::cerr << "Unknown enum external attribute \"" << externalValue
							          << "\"\n";
						}
					}
					auto valueNode = objectNode.first_child();
					while (valueNode)
					{
						if (std::string(valueNode.name()) == "value")
						{
							sEnum.values.push_back(valueNode.text().as_string());
						}
						else if (std::string(valueNode.name()) == "name")
						{
							sEnum.name = valueNode.text().as_string();
						}
						valueNode = valueNode.next_sibling();
					}
					state.enums.push_back(sEnum);
				}
				objectNode = objectNode.next_sibling();
			}
		}
		rootNode = rootNode.next_sibling();
	}

	return true;
}

template <typename First, typename Last, typename T, typename F, typename G>
void iterateDepthFirstByNames(First first, Last last, std::ostream &out, std::string T::*name,
                              F &&processElement, G &&elementSize)
{
	std::list<std::string> previousName;
	for (; first != last; ++first)
	{
		auto &e = *first;
		size_t substrStart = 0;
		size_t substrEnd = 0;
		std::list<std::string> currentName;

		// split by namespaces
		while (substrEnd != std::string::npos)
		{
			substrEnd = (e.*name).find("::", substrStart);
			size_t count =
			    substrEnd == std::string::npos ? std::string::npos : substrEnd - substrStart;
			currentName.push_back((e.*name).substr(substrStart, count));
			substrStart = substrEnd + 2;
		}

		// find the position of the first namespace that's different
		auto itP = previousName.begin();
		auto itC = currentName.begin();
		for (; itP != previousName.end() && itC != currentName.end() && *itP == *itC; ++itP, ++itC)
		{
		}

		// set and pop previously stacked tables
		if (previousName.size() > 0)
		{
			for (auto itP2 = --previousName.end(); itP2 != itP; --itP2)
				out << "\tlua_setfield(L, -2, \"" << *itP2 << "\");\n";
			out << "\tlua_setfield(L, -2, \"" << *itP << "\");\n";
		}

		// push new tables
		for (; itC != --currentName.end(); ++itC)
			out << "\tlua_createtable(L, 0, 1);\n";
		out << "\tlua_createtable(L, " << elementSize(e).first << ", " << elementSize(e).second
		    << ");\n";
		processElement(e);

		previousName = std::move(currentName);
	}
	// set and pop whats left
	for (auto itP = --previousName.end(); itP != --previousName.begin(); --itP)
		out << "\tlua_setfield(L, -2, \"" << *itP << "\");\n";
}

void writeHeader(std::ofstream &out, const StateDefinition &state)
{
	out << "// GENERATED HEADER - generated by LuaGamestateSupportGen - do not modify directly\n\n"
	    << "#pragma once\n\n"
	    << "#include \"game/state/gamestate.h\"\n\n"
	    << "#include \"game/state/luagamestate_support.h\"\n\n"
	    << "extern \"C\"\n{\n"
	    << "#include \"dependencies/lua/lua.h\"\n"
	    << "}\n\n"
	    << "namespace OpenApoc {\n\n"
	    << "static constexpr const char* const LUAGAMESTATE_SUPPORT_VERSION = \""
	    << state.hashString << "\";\n"
	    << "class SerializationNode;\n\n";
	for (auto &object : state.objects)
	{
		out << "void pushToLua(lua_State *L, " << object.name << " &obj);\n"
		    << "void pushToLua(lua_State *L, const " << object.name << " &obj);\n";
	}

	for (auto &e : state.enums)
	{
		if (e.external == false)
			continue;
		out << "void pushToLua(lua_State *L, " << e.name << " val);\n";
	}

	out << "\n} // namespace OpenApoc\n";
}

void writeSource(std::ofstream &out, const StateDefinition &state,
                 const std::string &outputHeaderFile)
{
	out << "// GENERATED SOURCE - generated by LuaGamestateSupportGen - do not modify directly\n\n";
	out << "#include \"framework/serialization/serialize.h\"\n";
	out << "#include \"game/state/gamestate_serialize.h\"\n";
	out << "#include \"" << outputHeaderFile << "\"\n\n";

	out << "namespace OpenApoc {\n\n";

	for (auto &object : state.objects)
	{
		out << "void pushToLua(lua_State *L, " << object.name << " &obj);\n"
		    << "void pushToLua(lua_State *L, const " << object.name << " &obj);\n";
	}

	for (auto &e : state.enums)
	{
		if (e.external == true)
			continue;
		out << "inline void pushToLua(lua_State *L, " << e.name << " val);\n";
	}

	out << "namespace\n{\n"
	    << "template<typename T> int getIndexMetamethod(lua_State *L);\n"
	    << "template<typename T> int getIndexConstMetamethod(lua_State *L);\n"
	    << "template<typename T> int newIndexMetamethod(lua_State *L);\n"
	    << "template<typename T> int toStringMetamethod(lua_State *L);\n";

	for (auto &object : state.objects)
	{
		// non-const case
		out << "template<> int getIndexMetamethod<" << object.name << ">(lua_State* L)\n"
		    << "{\n"
		    << "\t" << object.name << "** obj = (" << object.name << "**)lua_touserdata(L, 1);\n"
		    << "\tstd::string key = lua_tostring(L, 2);\n"
		    << "\tlua_settop(L, 0);\n"
		    << "\t";
		for (auto &m : object.members)
		{
			out << "if (key == \"" << m.first << "\") pushToLua(L, (*obj)->" << m.first << ");\n"
			    << "\telse ";
		}
		out << "if (auto method = getLuaObjectMethods<" << object.name
		    << ">(key)) lua_pushcfunction(L, method);\n"
		    << "\telse lua_pushnil(L);\n"
		    << "\treturn 1;\n"
		    << "}\n";

		// const case
		out << "template<> int getIndexConstMetamethod<" << object.name << ">(lua_State* L)\n"
		    << "{\n"
		    << "\tconst " << object.name << "** obj = (const " << object.name
		    << "**)lua_touserdata(L, 1);\n"
		    << "\tstd::string key = lua_tostring(L, 2);\n"
		    << "\tlua_settop(L, 0);\n"
		    << "\t";
		for (auto &m : object.members)
		{
			out << "if (key == \"" << m.first << "\") pushToLua(L, (*obj)->" << m.first << ");\n"
			    << "\telse ";
		}
		out << "if (auto method = getLuaObjectConstMethods<" << object.name
		    << ">(key)) lua_pushcfunction(L, method);\n"
		    << "\telse lua_pushnil(L);\n"
		    << "\treturn 1;\n"
		    << "}\n";

		// non-const only
		out << "template<> int newIndexMetamethod<" << object.name << ">(lua_State *L)\n"
		    << "{\n"
		    << "\t" << object.name << "** obj = (" << object.name << "**)lua_touserdata(L, 1);\n"
		    << "\tstd::string key = lua_tostring(L, 2);\n"
		    << "\t";
		for (auto &m : object.members)
		{
			out << "if (key == \"" << m.first << "\") getFromLua(L, 3, (*obj)->" << m.first
			    << ");\n"
			    << "\telse ";
		}
		out << "luaL_error(L, \"member variable %s not found in %s\", key.c_str(), \""
		    << object.name.c_str() << "\");\n"
		    << "\tlua_pop(L, 3);\n"
		    << "\treturn 0;\n"
		    << "}\n";

		// const and non-const
		out << "template<> int toStringMetamethod<" << object.name << ">(lua_State *L)\n"
		    << "{\n"
		    << "\tconst " << object.name << " **obj = (const " << object.name
		    << "**)lua_touserdata(L, -1);\n"
		    << "\tlua_settop(L, 0);\n"
		    << "\tlua_pushfstring(L, \"[" << object.name << " @ %p]\", (const void*)(*obj));\n"
		    << "\treturn 1;\n"
		    << "}\n";
	}
	out << "} // namespace\n";

	for (auto &object : state.objects)
	{
		// non-const
		out << "void pushToLua(lua_State *L, " << object.name << " &obj)\n{\n"
		    << "\t" << object.name << " **udata = (" << object.name
		    << "**)lua_newuserdata(L, sizeof(&obj));\n"
		    << "\t*udata = &obj;\n"
		    << "\tlua_createtable(L, 0, 2);\n"
		    << "\tlua_pushcfunction(L, getIndexMetamethod<" << object.name << ">);\n"
		    << "\tlua_setfield(L, -2, \"__index\");\n"
		    << "\tlua_pushcfunction(L, newIndexMetamethod<" << object.name << ">);\n"
		    << "\tlua_setfield(L, -2, \"__newindex\");\n"
		    << "\tlua_pushcfunction(L, toStringMetamethod<" << object.name << ">);\n"
		    << "\tlua_setfield(L, -2, \"__tostring\");\n"
		    << "\tlua_setmetatable(L, -2);\n"
		    << "}\n";

		// const
		out << "void pushToLua(lua_State *L, const " << object.name << " &obj)\n{\n"
		    << "\tconst " << object.name << " **udata = (const " << object.name
		    << "**)lua_newuserdata(L, sizeof(&obj));\n"
		    << "\t*udata = &obj;\n"
		    << "\tlua_createtable(L, 0, 2);\n"
		    << "\tlua_pushcfunction(L, getIndexConstMetamethod<" << object.name << ">);\n"
		    << "\tlua_setfield(L, -2, \"__index\");\n"
		    << "\tlua_pushcfunction(L, toStringMetamethod<" << object.name << ">);\n"
		    << "\tlua_setfield(L, -2, \"__tostring\");\n"
		    << "\tlua_setmetatable(L, -2);\n"
		    << "}\n";
	}

	for (auto &e : state.enums)
	{
		if (e.external == false)
			out << "inline\n";
		out << "void pushToLua(lua_State *L, " << e.name
		    << " v) { lua_pushinteger(L, static_cast<int>(v)); }\n";
	}

	out << "void pushLuaEnums(lua_State *L)\n"
	    << "{\n"
	    << "\tlua_createtable(L, 0, 0);\n";

	iterateDepthFirstByNames(state.enums.begin(), state.enums.end(), out, &SerializeEnum::name,
	                         [&out](auto &e) {
		                         for (auto &v : e.values)
		                         {
			                         out << "\tlua_pushinteger(L, static_cast<int>(" << e.name
			                             << "::" << v << "));\n"
			                             << "\tlua_setfield(L, -2, \"" << v << "\");\n";
		                         }
		                     },
	                         [](auto &e) { return std::pair<size_t, size_t>(0, e.values.size()); });
	out << "}\n";
	out << "\n} // namespace OpenApoc\n";
}

int main(int argc, char **argv)
{

	po::options_description desc("Allowed options");
	// These operator() really screw up clang-format
	// clang-format off
	desc.add_options()
		("help", "Show help message")
		("xml,x", po::value<std::string>(),"Input XML file")
		("output-header,h", po::value<std::string>(), "Path to output generated header file")
		("output-source,o", po::value<std::string>(), "Path to output generated source file")
	;
	// clang-format on

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	std::string inputXmlPath;
	std::string outputHeaderPath;
	std::string outputSourcePath;

	if (vm.count("help"))
	{
		std::cout << desc << "\n";
		return 1;
	}
	if (!vm.count("xml"))
	{
		std::cerr << "Must specify input XML file\n" << desc << "\n";
		return 1;
	}
	if (!vm.count("output-header"))
	{
		std::cerr << "Must specify output header file\n" << desc << "\n";
		return 1;
	}
	if (!vm.count("output-source"))
	{
		std::cerr << "Must specify output source file\n" << desc << "\n";
		return 1;
	}

	inputXmlPath = vm["xml"].as<std::string>();
	outputHeaderPath = vm["output-header"].as<std::string>();
	outputSourcePath = vm["output-source"].as<std::string>();

	std::ifstream inputXmlFile(inputXmlPath);
	if (!inputXmlFile)
	{
		std::cerr << "Failed to open input XML file \"" << inputXmlPath << "\"\n";
		return 1;
	}

	std::ofstream outputSourceFile(outputSourcePath);
	if (!outputSourceFile)
	{
		std::cerr << "Failed to open output source file at \"" << outputSourcePath << "\"\n";
		return 1;
	}

	std::ofstream outputHeaderFile(outputHeaderPath);
	if (!outputHeaderFile)
	{
		std::cerr << "Failed to open output header file at \"" << outputHeaderPath << "\"\n";
		return 1;
	}

	StateDefinition state;
	if (!readXml(inputXmlFile, state))
	{
		std::cerr << "Reading input XML file \"" << inputXmlPath << "\" failed\n";
		return 1;
	}

	// sort the items by name so they are ordered like a DFS iteration in a trie
	std::sort(
	    state.enums.begin(), state.enums.end(),
	    [](const SerializeEnum &lhs, const SerializeEnum &rhs) { return lhs.name < rhs.name; });
	std::sort(
	    state.objects.begin(), state.objects.end(),
	    [](const SerializeObject &lhs, const SerializeObject &rhs) { return lhs.name < rhs.name; });

	writeHeader(outputHeaderFile, state);
	writeSource(outputSourceFile, state, outputHeaderPath);

	return 0;
}
