#include "building.h"
#include "organisation.h"
#include "framework/framework.h"
#include "framework/includes.h"

namespace OpenApoc {

std::vector<std::string> Building::defaultNames =
{
	{"Building"},
	{"The Senate"},
	{"Judgment Central"},
	{"Enforcer Academy"},
	{"Law Control Station"},
	{"Megastation One"},
	{"Megastation Two"},
	{"Phoenix Sanatorium"},
	{"Nightingale Tower"},
	{"Iliad Institute"},
	{"Bosch Institute"},
	{"Marge Piercy Academy"},
	{"Rescue One"},
	{"Rescue Two"},
	{"Rescue Three"},
	{"Quadrax Tower"},
	{"Gygax Memorial Building"},
	{"Parallax Tower"},
	{"Tsunami Building"},
	{"Venus Spires"},
	{"Raven Reaches"},
	{"Mahler Building"},
	{"The Gugarin Institute"},
	{"Lincoln Tower"},
	{"The Armageddon Centre"},
	{"Cyborg Institute"},
	{"Uhuru Tower"},
	{"The Karpov Building"},
	{"Nietzsche Institute"},
	{"Foucault Tower"},
	{"Edifice Tower"},
	{"The Ozone Building"},
	{"The New Empire Tower"},
	{"Descartes Towers"},
	{"Transtellar Spacelines"},
	{"Galactic Central"},
	{"Megavision One"},
	{"Megavision Two"},
	{"Megavision Three"},
	{"Megatribe Warriors"},
	{"Meteor Kings"},
	{"Dog Star Wanderers"},
	{"Garden of Delights"},
	{"The Lineage Foundation"},
	{"Aldous Huxley Emporium"},
	{"Hypermart Zone"},
	{"Acropolis Apartments"},
	{"Atlantis Apartments"},
	{"Babylon Apartments"},
	{"Ptolemy Apartments"},
	{"Habizone Apartments"},
	{"Ecozone Apartments"},
	{"Stellar Apartments"},
	{"Lone Ranger Apartments"},
	{"Eden Mansions"},
	{"Utopia Mansions"},
	{"Nirvana Mansions"},
	{"Cyclops Mansions"},
	{"Cultivator One"},
	{"Cultivator Two"},
	{"Cultivator Three"},
	{"Cityclean One"},
	{"Cityclean Two"},
	{"Cityclean Three"},
	{"Hydrozone One"},
	{"Hydrozone Two"},
	{"Hydrozone Three"},
	{"Appliances One"},
	{"Appliances Two"},
	{"Appliances Three"},
	{"Arms One"},
	{"Arms Two"},
	{"Arms Three"},
	{"Robot One"},
	{"Robot Two"},
	{"Robot Three"},
	{"Car One"},
	{"Car Two"},
	{"Car Three"},
	{"Flyer One"},
	{"Flyer Two"},
	{"Flyer Three"},
	{"Megaflyer One"},
	{"Megaflyer Two"},
	{"Megaflyer Three"},
	{"Construction One"},
	{"Construction Two"},
	{"Construction Three"},
	{"George Orwell Block"},
	{"Thomas More Tower"},
	{"Dickens Estate"},
	{"Oliver Twist Block"},
	{"Campesino Apartments"},
	{"Grey Visitor Towers"},
	{"Scrooge Mansions"},
	{"Borstal Block"},
	{"Heavenly Towers"},
	{"Civic Project"},
	{"Chronos Block"},
	{"Necronomicon Mansions"},
	{"Angel Heart Heights"},
	{"Lovecraft Block"},
	{"Bakunin Block"},
	{"Saturn Block"},
	{"Hades Block"},
	{"Neptune Towers"},
	{"Maze Towers"},
	{"Grimoire Block"},
	{"Durruti Block"},
	{"Blue Doctor Project"},
	{"Enlightenment Towers"},
	{"Renaissance Block"},
	{"Slum City"},
	{"Warehouse One"},
	{"Warehouse Two"},
	{"Warehouse Three"},
	{"Warehouse Four"},
	{"Warehouse Five"},
	{"Warehouse Six"},
	{"Warehouse Seven"},
	{"Warehouse Eight"},
	{"Warehouse Nine"},
	{"Warehouse Ten"},
	{"Warehouse Eleven"},
	{"Warehouse Twelve"},
	{"Energen Building"},
	{"Midas Building"},
	{"Recyclotorium One"},
	{"Recyclotorium Two"},
	{"Recyclotorium Three"},
	{"Temple of the Apocalypse"},
	{"Temple of the Millenium"},
	{"Temple of the Visitors"},
	{"Temple of Humility"},
	{"Temple of Doom"},
	{"Temple of Sanity"},
}
;

Building::Building(Organisation &owner, std::string name, Rect<int> bounds)
	: owner(owner), name(name), bounds(bounds)
{}

std::vector<Building>
loadBuildingsFromBld(Framework &fw, std::string fileName, std::vector<Organisation> &orgList, std::vector<std::string> nameList)
{
	std::vector<Building> buildings;
	auto file = fw.data->load_file("xcom3/ufodata/" + fileName, "rb");
	if (!file)
	{
		std::cerr << "Failed to open building data file: " << fileName << "\n";
		return buildings;
	}
	//Currently known fields in .bld files:
	//0x00 uint16:  Building name index
	//0x02 uint16: Building position X0
	//0x04 uint16: Building position X1
	//0x06 uint16: Building position Y0
	//0x08 uint16: Building position Y1
	//0xc8 uint16: building Owner ID
	//
	//Total size of each buildilg field: 226 bytes
	size_t fileSize = PHYSFS_fileLength(file);
	int numBuildings = fileSize / 226;
	std::cout << "Loading " << numBuildings << " buildings in " << fileSize << "bytes\n";
	for (int b = 0; b < numBuildings; b++)
	{
		if (!PHYSFS_seek(file, b*226))
		{
			std::cerr << "Failed to seek to beginning of building " << b << "\n";
			break;
		}
		uint16_t nameIdx; PHYSFS_readULE16(file, &nameIdx);
		uint16_t x0; PHYSFS_readULE16(file, &x0);
		uint16_t x1; PHYSFS_readULE16(file, &x1);
		uint16_t y0; PHYSFS_readULE16(file, &y0);
		uint16_t y1; PHYSFS_readULE16(file, &y1);
		//Read 10 bytes
		//Skip to byte 200 (190 bytes)
		if (!PHYSFS_seek(file, (b*255)+200))
		{
			std::cerr << "Failed to seek reading building " << b << "\n";
			break;
		}
		uint16_t orgIdx; PHYSFS_readULE16(file, &orgIdx);
		if (nameIdx >= nameList.size() ||
		    nameIdx < 0)
		{
			std::cerr << "Invalid building name IDX " << nameIdx << " (max " << nameList.size() << ") reading building " << b << "\n";
			break;
		}
		if (orgIdx >= orgList.size() ||
		    orgIdx < 0)
		{
			std::cerr << "Invalid building owner IDX " << orgIdx << " (max " << orgList.size() << ") reading building " << b << "\n";
			break;
		}
		if (x0 < 0 || x0 >= 100 ||
		    y0 < 0 || y0 >= 100 ||
			x1 < 1 || x1 > 100 ||
			y1 < 1 || y1 > 100 ||
			x0 >= x1 || y0 >= y1)
		{
			std::cerr << "Invalid position (" << x0 << "," << y0 << ")(" << x1 << "," << y1 << ") reading building " << b << "\n";
			break;
		}
		buildings.emplace_back(orgList[orgIdx], nameList[nameIdx],
			Rect<int>(x0, y0, x1, y1));
		auto &bld = buildings.back();
		std::cout << "Read building \"" << bld.name << "\" owner \"" <<
			bld.owner.name << "\" position (" <<
			bld.bounds.p0.x << "," << bld.bounds.p0.y << "),(" <<
			bld.bounds.p1.x << "," << bld.bounds.p1.y << ")\n";
	}
	PHYSFS_close(file);
	return buildings;
}

}; //namespace OpenApoc
