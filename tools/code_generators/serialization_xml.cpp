#include "serialization_xml.h"

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
