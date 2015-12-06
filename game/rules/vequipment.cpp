#include "game/rules/rules.h"
#include "game/rules/rules_private.h"
#include "game/rules/rules_helper.h"
#include "game/rules/vequipment.h"
#include "framework/framework.h"

namespace OpenApoc
{

static bool ParseVehicleWeaponNode(tinyxml2::XMLElement *node, VEquipmentType &equipment)
{
	if (equipment.type != VEquipmentType::Type::Weapon)
	{
		LogError("Called on non-weapon equipment");
		return false;
	}
	auto &weapon = static_cast<VWeaponType &>(equipment);
	UString nodeName(node->Name());

	if (nodeName == "speed")
	{
		if (!ReadElement(node, weapon.speed))
		{
			LogError("Failed to read speed text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "projectile_image")
	{
		if (!ReadElement(node, weapon.projectile_image))
		{
			LogError("Failed to read projectile_image text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "damage")
	{
		if (!ReadElement(node, weapon.damage))
		{
			LogError("Failed to read damage text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "accuracy")
	{
		if (!ReadElement(node, weapon.accuracy))
		{
			LogError("Failed to read accuracy text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "fire_delay")
	{
		if (!ReadElement(node, weapon.fire_delay))
		{
			LogError("Failed to read fire_delay text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "tail_size")
	{
		if (!ReadElement(node, weapon.tail_size))
		{
			LogError("Failed to read tail_size text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "guided")
	{
		if (!ReadElement(node, weapon.guided))
		{
			LogError("Failed to read guided text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "turn_rate")
	{
		if (!ReadElement(node, weapon.turn_rate))
		{
			LogError("Failed to read turn_rate text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "range")
	{
		if (!ReadElement(node, weapon.range))
		{
			LogError("Failed to read range text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "firing_arc_1")
	{
		if (!ReadElement(node, weapon.firing_arc_1))
		{
			LogError("Failed to read firing_arc_1 text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "firing_arc_2")
	{
		if (!ReadElement(node, weapon.firing_arc_2))
		{
			LogError("Failed to read firing_arc_2 text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "point_defence")
	{
		if (!ReadElement(node, weapon.point_defence))
		{
			LogError("Failed to read point_defence text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "fire_sfx")
	{
		if (!ReadElement(node, weapon.fire_sfx_path))
		{
			LogError("Failed to read fire_sfx text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "explosion_graphic")
	{
		if (!ReadElement(node, weapon.explosion_graphic))
		{
			LogError("Failed to read explosion_graphic text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "icon")
	{
		if (!ReadElement(node, weapon.icon_path))
		{
			LogError("Failed to read icon text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}

	LogError("Unexpected node \"%s\" for vehicle weapon ID \"%s\"", nodeName.c_str(),
	         equipment.id.c_str());
	return false;
}

static bool ParseVehicleEngineNode(tinyxml2::XMLElement *node, VEquipmentType &equipment)
{
	if (equipment.type != VEquipmentType::Type::Engine)
	{
		LogError("Called on non-engine equipment");
		return false;
	}
	auto &engine = static_cast<VEngineType &>(equipment);
	UString node_name(node->Name());

	if (node_name == "top_speed")
	{
		if (!ReadElement(node, engine.top_speed))
		{
			LogError("Failed to read top_speed text \"%s\" for vehicle engine ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (node_name == "power")
	{
		if (!ReadElement(node, engine.power))
		{
			LogError("Failed to read power text \"%s\" for vehicle engine ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}

	LogError("Unexpected node \"%s\" for vehicle engine ID \"%s\"", node_name.c_str(),
	         equipment.id.c_str());
	return false;
}

static bool ParseVehicleGeneralNode(tinyxml2::XMLElement *node, VEquipmentType &equipment)
{
	if (equipment.type != VEquipmentType::Type::General)
	{
		LogError("Called on non-engine equipment");
		return false;
	}
	auto &gequipment = static_cast<VGeneralEquipmentType &>(equipment);
	UString node_name(node->Name());

	if (node_name == "accuracy_modifier")
	{
		if (!ReadElement(node, gequipment.accuracy_modifier))
		{
			LogError("Failed to read accuracy_modifier text \"%s\" for vehicle general equipment "
			         "ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (node_name == "cargo_space")
	{
		if (!ReadElement(node, gequipment.cargo_space))
		{
			LogError(
			    "Failed to read cargo_space text \"%s\" for vehicle general equipment ID \"%s\"",
			    node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (node_name == "passengers")
	{
		if (!ReadElement(node, gequipment.passengers))
		{
			LogError(
			    "Failed to read passengers text \"%s\" for vehicle general equipment ID \"%s\"",
			    node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (node_name == "alien_space")
	{
		if (!ReadElement(node, gequipment.alien_space))
		{
			LogError(
			    "Failed to read alien_space text \"%s\" for vehicle general equipment ID \"%s\"",
			    node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (node_name == "missile_jamming")
	{
		if (!ReadElement(node, gequipment.missile_jamming))
		{
			LogError("Failed to read missile_jamming text \"%s\" for vehicle general equipment ID "
			         "\"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (node_name == "shielding")
	{
		if (!ReadElement(node, gequipment.shielding))
		{
			LogError("Failed to read shielding text \"%s\" for vehicle general equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (node_name == "cloaking")
	{
		if (!ReadElement(node, gequipment.cloaking))
		{
			LogError("Failed to read cloaking text \"%s\" for vehicle general equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (node_name == "teleporting")
	{
		if (!ReadElement(node, gequipment.teleporting))
		{
			LogError(
			    "Failed to read teleporting text \"%s\" for vehicle general equipment ID \"%s\"",
			    node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}

	LogError("Unexpected node \"%s\" for vehicle general equipment ID \"%s\"", node_name.c_str(),
	         equipment.id.c_str());
	return false;
}

static bool ParseVehicleEquipmentNode(tinyxml2::XMLElement *node, VEquipmentType &equipment)
{
	UString nodeName(node->Name());

	if (nodeName == "usable_by")
	{
		std::map<UString, VEquipmentType::User> user_map = {
		    {"air", VEquipmentType::User::Air}, {"ground", VEquipmentType::User::Ground},
		};
		VEquipmentType::User user;
		if (!ReadElement(node, user_map, user))
		{
			LogError("Invalid user \"%s\" for vehicle weapon ID \"%s\"", node->GetText(),
			         equipment.id.c_str());
			return false;
		}
		equipment.users.insert(user);
		return true;
	}
	else if (nodeName == "weight")
	{
		if (!ReadElement(node, equipment.weight))
		{
			LogError("Failed to read weight text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "max_ammo")
	{
		if (!ReadElement(node, equipment.max_ammo))
		{
			LogError("Failed to read max_ammo text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "ammo_type")
	{
		if (!ReadElement(node, equipment.ammo_type))
		{
			LogError("Failed to read ammo_type text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "sprite")
	{
		if (!ReadElement(node, equipment.equipscreen_sprite_name))
		{
			LogError("Failed to read sprite text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "size")
	{
		if (!ReadElement(node, equipment.equipscreen_size))
		{
			LogError("Failed to read size text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "manufacturer")
	{
		if (!ReadElement(node, equipment.manufacturer))
		{
			LogError("Failed to read manufacturer text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "store_space")
	{
		if (!ReadElement(node, equipment.store_space))
		{
			LogError("Failed to read store_space text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	else if (nodeName == "idem")
	{
		// FIXME: WTF is this?
		LogWarning("Ignoring \"idem\" node");
		return true;
	}
	else if (nodeName == "name")
	{
		if (!ReadElement(node, equipment.name))
		{
			LogError("Failed to read name text \"%s\" for vehicle equipment ID \"%s\"",
			         node->GetText(), equipment.id.c_str());
			return false;
		}
		return true;
	}
	return false;
}

bool RulesLoader::ParseVehicleEquipment(Framework &fw, Rules &rules, tinyxml2::XMLElement *rootNode)
{
	if (UString(rootNode->Name()) != UString("vehicle_equipment"))
	{
		LogError("Called on unexpected node \"%s\"", rootNode->Name());
		return false;
	}
	UString id;
	if (!ReadAttribute(rootNode, "id", id))
	{
		LogError("No \"id\" attribute on vehicle equipment node");
		return false;
	}
	VEquipmentType::Type type;

	std::map<UString, VEquipmentType::Type> typeMap{
	    {"weapon", VEquipmentType::Type::Weapon},
	    {"general", VEquipmentType::Type::General},
	    {"engine", VEquipmentType::Type::Engine},
	};
	if (!ReadAttribute(rootNode, "type", typeMap, type))
	{
		LogError("Invalid \"type\" in vehicle equipment \"%s\"", id.c_str());
		return false;
	}

	if (rules.vehicle_equipment[id] == nullptr)
	{
		switch (type)
		{
			case VEquipmentType::Type::Weapon:
			{
				rules.vehicle_equipment[id].reset(new VWeaponType(id));
				break;
			}
			case VEquipmentType::Type::General:
			{
				rules.vehicle_equipment[id].reset(new VGeneralEquipmentType(id));
				break;
			}
			case VEquipmentType::Type::Engine:
			{
				rules.vehicle_equipment[id].reset(new VEngineType(id));
				break;
			}
			default:
				LogError("Invalid VEquipmentType type");
				return false;
		}
	}
	else
	{
		LogInfo("Overriding existing ID \"%s\"", id.c_str());
	}
	auto &equipment = *rules.vehicle_equipment[id];
	if (equipment.type != type)
	{
		LogError("Trying to change type of VEquipmentType id \"%s\"", id.c_str());
		return false;
	}
	for (tinyxml2::XMLElement *e = rootNode->FirstChildElement(); e != nullptr;
	     e = e->NextSiblingElement())
	{
		if (ParseVehicleEquipmentNode(e, equipment))
			continue;
		switch (type)
		{
			case VEquipmentType::Type::Weapon:
				if (ParseVehicleWeaponNode(e, equipment))
					continue;
				break;
			case VEquipmentType::Type::General:
				if (ParseVehicleGeneralNode(e, equipment))
					continue;
				break;
			case VEquipmentType::Type::Engine:
				if (ParseVehicleEngineNode(e, equipment))
					continue;
				break;
		}
		LogError("Unexpected node \"%s\" on vehicle_equipment id \"%s\"", e->Name(), id.c_str());
		return false;
	}

	return true;
}

VEquipmentType::VEquipmentType(Type type, const UString &id)
    : type(type), id(id), weight(0), max_ammo(0), store_space(0)
{
}

bool VEquipmentType::isValid(Framework &fw, Rules &rules)
{
	if (!RulesLoader::isValidEquipmentID(id))
	{
		LogError("Invalid ID \"%s\" on vehicle_equipment", id.c_str());
		return false;
	}
	if (!RulesLoader::isValidOrganisation(rules, this->manufacturer))
	{
		LogError("Unknown equipment \"%s\" has invalid manufacturer \"%s\"", this->id.c_str(),
		         this->manufacturer.c_str());
		return false;
	}
	if (this->users.empty())
	{
		LogError("Vehicle equipment \"%s\" has no valid users", id.c_str());
		return false;
	}
	if (!RulesLoader::isValidStringID(name))
	{
		LogError("Vehicle equipment \"%s\" has invalid name string \"%s\"", id.c_str(),
		         name.c_str());
		return false;
	}
	if (!equipscreen_sprite)
	{
		equipscreen_sprite = fw.data->load_image(equipscreen_sprite_name);
	}
	if (!equipscreen_sprite)
	{
		LogError("Vehicle equipment \"%s\" equipscreen sprite ", id.c_str());
		return false;
	}

	return true;
}

VWeaponType::VWeaponType(const UString &id)
    : VEquipmentType(VEquipmentType::Type::Weapon, id), speed(0), projectile_image(0), damage(0),
      accuracy(0), fire_delay(0), tail_size(0), guided(false), turn_rate(0), range(0),
      firing_arc_1(0), firing_arc_2(0), point_defence(false)
{
}

bool VWeaponType::isValid(Framework &fw, Rules &rules)
{
	if (!VEquipmentType::isValid(fw, rules))
		return false;
	if (speed == 0)
	{
		LogError("Vehicle weapon \"%s\" has zero speed", id.c_str());
		return false;
	}
	if (fire_delay == 0)
	{
		LogError("Vehicle weapon \"%s\" has zero fire_delay", id.c_str());
		return false;
	}
	if (tail_size == 0)
	{
		LogError("Vehicle weapon \"%s\" has zero tail_size", id.c_str());
		return false;
	}
	if (guided && turn_rate == 0)
	{
		LogError("Vehicle weapon \"%s\" has zero turn_rate but is set as guided", id.c_str());
		return false;
	}
	if (range == 0)
	{
		LogError("Vehicle weapon \"%s\" has zero range", id.c_str());
		return false;
	}
	if (firing_arc_1 == 0)
	{
		LogError("Vehicle weapon \"%s\" has zero firing_arc_1", id.c_str());
		return false;
	}
	if (firing_arc_2 == 0)
	{
		LogError("Vehicle weapon \"%s\" has zero firing_arc_1", id.c_str());
		return false;
	}
	if (icon_path == "")
	{
		LogError("Vehicle weapon \"%s\" has no icon", id.c_str());
		return false;
	}
	this->icon = fw.data->load_image(this->icon_path);
	if (!this->icon)
	{
		LogError("Vehicle weapon \"%s\" failed to load icon image \"%s\"", id.c_str(),
		         this->icon_path.c_str());
		return false;
	}

	if (this->fire_sfx_path != "")
	{
		this->fire_sfx = fw.data->load_sample(this->fire_sfx_path);
		if (!this->fire_sfx)
		{
			LogError("Vehicle weapon \"%s\" failed to load fire sfx \"%s\"", id.c_str(),
			         this->fire_sfx_path.c_str());
			return false;
		}
	}

	return true;
}

VGeneralEquipmentType::VGeneralEquipmentType(const UString &id)
    : VEquipmentType(VEquipmentType::Type::General, id), accuracy_modifier(0), cargo_space(0),
      passengers(0), alien_space(0), missile_jamming(0), shielding(0), cloaking(false),
      teleporting(false)
{
}

bool VGeneralEquipmentType::isValid(Framework &fw, Rules &rules)
{
	if (!VEquipmentType::isValid(fw, rules))
		return false;
	if (this->accuracy_modifier == 0 && this->cargo_space == 0 && this->passengers == 0 &&
	    this->alien_space == 0 && this->missile_jamming == 0 && this->shielding == 0 &&
	    this->cloaking == false && this->teleporting == false)
	{
		LogError("Vehicle general equipment ID \"%s\" has no effects", this->id.c_str());
		return false;
	}
	return true;
}

VEngineType::VEngineType(const UString &id)
    : VEquipmentType(VEquipmentType::Type::Engine, id), power(0), top_speed(0)
{
}

bool VEngineType::isValid(Framework &fw, Rules &rules)
{
	if (!VEquipmentType::isValid(fw, rules))
		return false;
	if (this->power == 0)
	{
		LogError("Vehicle engine \"%s\" has zero power", id.c_str());
		return false;
	}
	if (this->top_speed == 0)
	{
		LogError("Vehicle engine \"%s\" has zero top_speed", id.c_str());
		return false;
	}
	return true;
}
} // namespace OpenApoc
