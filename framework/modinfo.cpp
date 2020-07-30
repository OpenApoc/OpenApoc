#include "framework/modinfo.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "framework/logger.h"

namespace OpenApoc
{

using namespace pugi;

// Returns the value if "nodeName" child exists, else empty string
static UString readNode(const char *nodeName, const xml_node &node)
{
	auto childNode = node.child(nodeName);
	if (childNode)
		return childNode.text().get();
	else
		return "";
}

std::optional<ModInfo> ModInfo::getInfo(const UString &path)
{
	xml_document doc;
	ModInfo info;

	auto filePath = path + "/modinfo.xml";
	auto parseResult = doc.load_file(filePath.c_str());
	if (!parseResult)
	{
		LogWarning("Failed to parse ModInfo at \"%s\": %s at offset %u", filePath,
		           parseResult.description(), parseResult.offset);
		return {};
	}
	auto infoNode = doc.child("openapoc_modinfo");
	if (!infoNode)
	{
		LogWarning("ModInfo at \"%s\" doesn't have an \"openapoc_modinfo\" root node", filePath);
		return {};
	}

	info.setName(readNode("name", infoNode));
	info.setAuthor(readNode("author", infoNode));
	info.setVersion(readNode("version", infoNode));
	info.setDescription(readNode("description", infoNode));
	info.setLink(readNode("link", infoNode));
	info.setID(readNode("id", infoNode));
	info.setDataPath(readNode("datapath", infoNode));
	info.setStatePath(readNode("statepath", infoNode));
	info.setMinVersion(readNode("minversion", infoNode));
	info.setModLoadScript(readNode("modloadscript", infoNode));

	auto requiresNode = infoNode.child("requires");
	if (requiresNode)
	{
		for (const auto node : requiresNode.children("entry"))
		{
			info.requirements().push_back(node.value());
		}
	}

	auto conflictsNode = infoNode.child("conflicts");
	if (conflictsNode)
	{
		for (const auto node : conflictsNode.children("entry"))
		{
			info.conflicts().push_back(node.value());
		}
	}
	return info;
}

bool ModInfo::writeInfo(const UString &path)
{
	xml_document doc;
	auto infoNode = doc.append_child("openapoc_modinfo");
	infoNode.append_child("name").text() = name.c_str();
	infoNode.append_child("author").text() = author.c_str();
	infoNode.append_child("version").text() = version.c_str();
	infoNode.append_child("description").text() = description.c_str();
	infoNode.append_child("link").text() = link.c_str();
	infoNode.append_child("id").text() = ID.c_str();
	infoNode.append_child("datapath").text() = dataPath.c_str();
	infoNode.append_child("statepath").text() = statePath.c_str();
	infoNode.append_child("minversion").text() = minVersion.c_str();
	infoNode.append_child("modloadscript").text() = modLoadScript.c_str();

	auto requiresNode = infoNode.append_child("requires");
	for (const auto &requirement : _requirements)
	{
		requiresNode.append_child("entry").text() = requirement.c_str();
	}
	auto conflictsNode = infoNode.append_child("conflicts");
	for (const auto &conflict : _conflicts)
	{
		conflictsNode.append_child("entry").text() = conflict.c_str();
	}

	auto filePath = path + "/modinfo.xml";
	auto saveResult = doc.save_file(filePath.c_str());
	if (!saveResult)
	{
		LogWarning("Failed to save ModInfo to \"%s\"", filePath);
		return false;
	}

	return true;
}
} // namespace OpenApoc
