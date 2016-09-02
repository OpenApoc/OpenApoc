#include "tacp.h"
#include "crc32.h"
#include "framework/framework.h"

#include <iomanip>
#include <iterator>

namespace OpenApoc
{

/* This is the crc32 of the tacp.exe found on my steam version of apoc
 * It's likely there are other executables around with different CRCs, but we
 * need to make sure the offsets are the same, then we can add them to an
 * 'allowed' list, or have a map of 'known' CRCs with offsets of the various
 * tables */
uint32_t expected_tacp_crc32 = 0xd26df29e;

TACP::TACP(std::string file_name)
{
	auto file = fw().data->fs.open(file_name);

	if (!file)
	{
		LogError("Failed to open \"%s\"", file_name.c_str());
		exit(1);
	}

	std::istream_iterator<uint8_t> eof;
	std::istream_iterator<uint8_t> sof(file);

	auto crc32 = crc(sof, eof);

	if (crc32 != expected_tacp_crc32)
	{
		LogError("File \"%s\"\" has an unknown crc32 value of 0x%08lx - expected 0x%08x",
		         file_name.c_str(), crc32, expected_tacp_crc32);
	}

	file.seekg(0, std::ios::beg);
	file.clear();

	// hand-filling damage mod names as they are not present in the game exe
	{
		auto vec = std::vector<std::string>();
		vec.emplace_back("Human");
		vec.emplace_back("Mutant");
		vec.emplace_back("Android");
		vec.emplace_back("Alien Egg");
		vec.emplace_back("Multiworm");
		vec.emplace_back("Hyperworm");
		vec.emplace_back("Crysalis");
		vec.emplace_back("Brainsucker");
		vec.emplace_back("Queenspawn");
		vec.emplace_back("Anthropod");
		vec.emplace_back("Psimorph");
		vec.emplace_back("Spitter");
		vec.emplace_back("Megaspawn");
		vec.emplace_back("Popper");
		vec.emplace_back("Skeletoid");
		vec.emplace_back("Micronoid Aggregate");
		vec.emplace_back("Disruptor Shield");
		vec.emplace_back("Megapol Armor");
		vec.emplace_back("Marsec Armor");
		vec.emplace_back("X-COM Disruptor Armor");
		vec.emplace_back("Terrain 1?");
		vec.emplace_back("Terrain 2?");
		vec.emplace_back("Gun Emplacement");
		this->damage_modifier_names.reset(new StrTab(vec));
	}

	this->agent_equipment.reset(new DataChunk<AgentEquipmentData>(
	    file, AGENT_EQUIPMENT_DATA_OFFSET_START, AGENT_EQUIPMENT_DATA_OFFSET_END));
}

} // namespace OpenApoc
