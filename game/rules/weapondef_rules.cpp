#include "game/rules/rules_private.h"

namespace OpenApoc
{

static bool
ParseVehicleWeaponDefinition(WeaponDef &def, tinyxml2::XMLElement *root)
{
	//FIXME: A lot of these could be shared for all weapon types?
	
	if (!root->Attribute("projectile"))
	{
		LogError("No 'projectile' attribute");
		return false;
	}
	UString projectile = root->Attribute("projectile");
	if (projectile == "beam")
	{
		def.projectileType = WeaponDef::ProjectileType::Beam;
	}
	else
	{
		LogError("Unknown projectile type \"%s\"", projectile.str().c_str());
		return false;
	}

	/* firingSFX is optional */
	def.firingSFX = root->Attribute("firingSFX");

	/* pointDefence is optional */
	if (root->Attribute("pointDefence"))
	{
		UString pointDefence = root->Attribute("pointDefence");
		if (pointDefence == "true")
			def.pointDefence = true;
		else if (pointDefence == "false")
			def.pointDefence = false;
		else
		{
			LogError("Invalid pointDefence attribute \"%s\"", pointDefence.str().c_str());
			return false;
		}
	}
	else
		def.pointDefence = false;

	/* arc is optional */
	if (root->Attribute("arc"))
	{
		UString arc = root->Attribute("arc");
		if (!Strings::IsFloat(arc))
		{
			LogError("Invalid arc attribute \"%s\"", arc.str().c_str());
			return false;
		}
		def.arc = Strings::ToFloat(arc);
	}
	else
		def.arc = 0.0f;
	/* accuracy is optional */
	if (root->Attribute("accuracy"))
	{
		UString accuracy = root->Attribute("accuracy");
		if (!Strings::IsFloat(accuracy))
		{
			LogError("Invalid accuracy attribute \"%s\"", accuracy.str().c_str());
			return false;
		}
		def.accuracy = Strings::ToFloat(accuracy);
	}
	else
		def.accuracy = 0.0f;

	if (!root->Attribute("range"))
	{
		LogError("No 'range' attribute");
		return false;
	}
	UString range = root->Attribute("range");
	if (!Strings::IsFloat(range))
	{
		LogError("Invalid range attribute \"%s\"", range.str().c_str());
		return false;
	}
	def.range = Strings::ToFloat(range);

	if (!root->Attribute("firingDelay"))
	{
		LogError("No 'firingDelay' attribute");
		return false;
	}
	UString firingDelay = root->Attribute("firingDelay");
	if (!Strings::IsFloat(firingDelay))
	{
		LogError("Invalid firingDelay attribute \"%s\"", firingDelay.str().c_str());
		return false;
	}
	def.firingDelay = Strings::ToFloat(firingDelay);

	if (!root->Attribute("projectileDamage"))
	{
		LogError("No 'projectileDamage' attribute");
		return false;
	}
	UString projectileDamage = root->Attribute("projectileDamage");
	if (!Strings::IsFloat(projectileDamage))
	{
		LogError("Invalid projectileDamage attribute \"%s\"", projectileDamage.str().c_str());
		return false;
	}
	def.projectileDamage = Strings::ToFloat(projectileDamage);

	/* projetileTailLength is optional */
	if (root->Attribute("projectileTailLength"))
	{
		UString projectileTailLength = root->Attribute("projectileTailLength");
		if (!Strings::IsFloat(projectileTailLength))
		{
			LogError("Invalid projectileTailLength attribute \"%s\"", projectileTailLength.str().c_str());
			return false;
		}
		def.projectileTailLength = Strings::ToFloat(projectileTailLength);
	}
	else
		def.projectileTailLength = 0.0f;
	
	if (!root->Attribute("projectileSpeed"))
	{
		LogError("No 'projectileSpeed' attribute");
		return false;
	}

	UString projectileSpeed = root->Attribute("projectileSpeed");
	if (!Strings::IsFloat(projectileSpeed))
	{
		LogError("Invalid projectileSpeed attribute \"%s\"", projectileSpeed.str().c_str());
		return false;
	}

	def.projectileSpeed = Strings::ToFloat(projectileSpeed);

	/* projectileTurnRate is optional */
	if (root->Attribute("projectileTurnRate"))
	{
		UString projectileTurnRate = root->Attribute("projectileTurnRate");
		if (!Strings::IsFloat(projectileTurnRate))
		{
			LogError("Invalid projectileTurnRate attribute \"%s\"", projectileTurnRate.str().c_str());
			return false;
		}
		def.projectileTurnRate = Strings::ToFloat(projectileTurnRate);
	}
	else
		def.projectileTurnRate = 0.0f;

	/* hitSprite is optional */
	def.hitSprite = root->Attribute("hitSprite");

	/* beamWidth is optional */
	if (root->Attribute("beamWidth"))
	{
		if (def.projectileType != WeaponDef::ProjectileType::Beam)
		{
			LogError("beamWidth attribute specified on non-beam weapon");
			return false;
		}
		UString beamWidth = root->Attribute("beamWidth");
		if (!Strings::IsFloat(beamWidth))
		{
			LogError("Invalid beamWidth attribute \"%s\"", beamWidth.str().c_str());
			return false;
		}
		def.beamWidth = Strings::ToFloat(beamWidth);
	}
	else
		def.beamWidth = 1.0f;

	/* beamColour is optional */
	if (root->Attribute("beamColour"))
	{
		if (def.projectileType != WeaponDef::ProjectileType::Beam)
		{
			LogError("beamColour attribute specified on non-beam weapon");
			return false;
		}
		UString beamColour = root->Attribute("beamColour");
		/* May be 'r,g,b,a' or 'r,g,b' with implicit 255 alpha */
		auto splitBeamColour = beamColour.split(",");
		if (splitBeamColour.size() != 3 && splitBeamColour.size() != 4)
		{
			LogError("Invalid beamColour attribute \"%s\" (expected 'r,g,b' or 'r,g,b,a')",
				beamColour.str().c_str());
			return false;
		}
		if (!(Strings::IsInteger(splitBeamColour[0]) &&
		      Strings::IsInteger(splitBeamColour[1]) &&
			  Strings::IsInteger(splitBeamColour[2])))
		{
			LogError("Invalid beamColour attribute element \"%s\"", beamColour.str().c_str());
			return false;
		}
		if (splitBeamColour.size() == 4 && !Strings::IsInteger(splitBeamColour[3]))
		{
			LogError("Invalid beamColour attribute element \"%s\"", beamColour.str().c_str());
			return false;
		}
		int r = Strings::ToInteger(splitBeamColour[0]);
		int g = Strings::ToInteger(splitBeamColour[0]);
		int b = Strings::ToInteger(splitBeamColour[0]);
		int a = 255;
		if (splitBeamColour.size() == 4)
			a = Strings::ToInteger(splitBeamColour[3]);

		if (r < 0 || r > 255 ||
		    g < 0 || g > 255 ||
			b < 0 || b > 255 ||
			a < 0 || a > 255)
		{
			LogError("beamColour out of range (0-255) \"%s\"", beamColour.str().c_str());
			return false;
		}
		def.beamColour = Colour{(uint8_t)r,(uint8_t)g,(uint8_t)b,(uint8_t)a};
	}
	else
		def.beamColour = Colour{255,255,255,255};

	if (!root->Attribute("ammoCapacity"))
	{
		LogError("No 'ammoCapacity' attribute");
		return false;
	}

	UString ammoCapacity = root->Attribute("ammoCapacity");
	if (!Strings::IsInteger(ammoCapacity))
	{
		LogError("Invalid ammoCapacity attribute \"%s\"", ammoCapacity.str().c_str());
		return false;
	}
	def.ammoCapacity = Strings::ToInteger(ammoCapacity);

	/* ammoPerShot is optional */
	if (root->Attribute("ammoPerShot"))
	{
		UString ammoPerShot = root->Attribute("ammoPerShot");
		if (!Strings::IsInteger(ammoPerShot))
		{
			LogError("Invalid ammoPerShot attribute \"%s\"", ammoPerShot.str().c_str());
			return false;
		}
		def.ammoPerShot = Strings::ToInteger(ammoPerShot);
	}
	else
		def.ammoPerShot = 1;
	
	def.ammoType = root->Attribute("ammoType");

	return true;
}

bool
RulesLoader::ParseWeaponDefinition(Framework &fw, Rules &rules, tinyxml2::XMLElement *root)
{
	std::ignore = fw;
	if (UString(root->Name()) != "weapon")
	{
		LogError("Called on unexpected node \"%s\"", root->Name());
		return false;
	}

	WeaponDef def;

	if (!root->Attribute("name"))
	{
		LogError("No 'name' attribute");
		return false;
	}
	def.name = root->Attribute("name");

	if (!root->Attribute("type"))
	{
		LogError("No 'type' attribute");
		return false;
	}

	UString type = root->Attribute("type");
	if (type == "vehicle")
	{
		def.type = WeaponDef::Type::Vehicle;
		if (!ParseVehicleWeaponDefinition(def, root))
		{
			LogError("Failed to load weapon \"%s\"", def.name.str().c_str());
			return false;
		}
	}
	else
	{
		LogError("Unknown weapon type \"%s\"", type.str().c_str());
		return false;
	}

	LogInfo("Loading weapon \"%s\"", def.name.str().c_str());

	rules.weapons.emplace(def.name, def);

	return true;
}
}; //namespace OpenApoc
