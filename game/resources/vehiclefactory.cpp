#include "game/resources/vehiclefactory.h"

#include "framework/framework.h"
#include "game/resources/gamecore.h"

namespace OpenApoc {

VehicleFactory::VehicleFactory(Framework &fw)
	: fw(fw)
{

}

VehicleFactory::~VehicleFactory()
{

}

static std::map<Vehicle::Direction, std::shared_ptr<Image> >
parseDirectionalSprites(Framework &fw, tinyxml2::XMLElement *root)
{
	std::map<Vehicle::Direction, std::shared_ptr<Image> > sprites;

	for (tinyxml2::XMLElement* node = root->FirstChildElement(); node != nullptr; node = node->NextSiblingElement())
	{
		UString name = node->Name();
		Vehicle::Direction dir;
		if (name == "N")
			dir = Vehicle::Direction::N;
		else if (name == "NNE")
			dir = Vehicle::Direction::NNE;
		else if (name == "NE")
			dir = Vehicle::Direction::NE;
		else if (name == "NEE")
			dir = Vehicle::Direction::NEE;
		else if (name == "E")
			dir = Vehicle::Direction::E;
		else if (name == "SEE")
			dir = Vehicle::Direction::SEE;
		else if (name == "SE")
			dir = Vehicle::Direction::SE;
		else if (name == "SSE")
			dir = Vehicle::Direction::SSE;
		else if (name == "S")
			dir = Vehicle::Direction::S;
		else if (name == "SSW")
			dir = Vehicle::Direction::SSW;
		else if (name == "SW")
			dir = Vehicle::Direction::SW;
		else if (name == "SWW")
			dir = Vehicle::Direction::SWW;
		else if (name == "W")
			dir = Vehicle::Direction::W;
		else if (name == "NWW")
			dir = Vehicle::Direction::NWW;
		else if (name == "NW")
			dir = Vehicle::Direction::NW;
		else if (name == "NNW")
			dir = Vehicle::Direction::NNW;
		else
		{
			LogError("Unknown sprite direction \"%S\"", name.getTerminatedBuffer());
			continue;
		}

		UString spriteName = node->GetText();
		LogInfo("Loading image \"%S\"", spriteName.getTerminatedBuffer());
		if (sprites[dir])
			LogWarning("Replacing directional sprite");
		auto sprite = fw.gamecore->GetImage(spriteName);

		if (!sprite)
			LogError("Failed to load directional sprite");
		sprites[dir] = sprite;
	}
	return sprites;
}

void
VehicleFactory::ParseVehicleDefinition(tinyxml2::XMLElement *root)
{
	VehicleDefinition def;
	def.name = root->Attribute("id");

	UString type = root->Attribute("type");

	if (type == "flying")
		def.type = Vehicle::Type::Flying;
	else if (type == "ground")
		def.type = Vehicle::Type::Ground;
	else
	{
		LogError("Unknown vehicle type \"%S\"", type.getTerminatedBuffer());
		return;
	}

	def.size.x = root->FloatAttribute("sizeX");
	def.size.y = root->FloatAttribute("sizeY");
	def.size.z = root->FloatAttribute("sizeZ");

	for (tinyxml2::XMLElement* node = root->FirstChildElement(); node != nullptr; node = node->NextSiblingElement())
	{
		UString tag = node->Name();
		if (tag == "flat")
		{
			def.sprites[Vehicle::Banking::Flat] = parseDirectionalSprites(fw, node);
		}
		else if (tag == "ascending")
		{
			def.sprites[Vehicle::Banking::Ascending] = parseDirectionalSprites(fw, node);
		}
		else if (tag == "decending")
		{
			def.sprites[Vehicle::Banking::Decending] = parseDirectionalSprites(fw, node);
		}
		else if (tag == "banking_left")
		{
			def.sprites[Vehicle::Banking::Left] = parseDirectionalSprites(fw, node);
		}
		else if (tag == "banking_right")
		{
			def.sprites[Vehicle::Banking::Right] = parseDirectionalSprites(fw, node);
		}
		else
		{
			LogError("Unknown vehicle tag \"%S\"", tag.getTerminatedBuffer());
			continue;
		}
	}

	this->defs[def.name] = def;

}

std::shared_ptr<Vehicle>
VehicleFactory::create(const UString name)
{
	auto &def = this->defs[name];
	auto v = std::shared_ptr<Vehicle>(new Vehicle(def));


	return v;
}

}; //namespace OpenApoc
