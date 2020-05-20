#include "framework/data.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/battlecommonimagelist.h"
#include "game/state/rules/battle/battleunitimagepack.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/shared/agent.h"
#include "library/strings_format.h"
#include "tools/extractors/common/doodads.h"
#include "tools/extractors/common/tacp.h"
#include "tools/extractors/extractors.h"
#include <limits>

#define E_TRONLAUN 37  // extra / tronlaun
#define A_QUEENWHP 97  // tactical / aliens / attacks / queenwhp
#define A_SPITTER 98   // tactical / aliens / attacks / spitter
#define A_WORMSPIT 99  // tactical / aliens / attacks / wormspit
#define A_WRMATTAK 100 // tactical / aliens / attacks / wrmattak
#define W_BULLET1 161  // bullet1
#define W_BULLET2 162  // bullet2
#define W_BULLET3 163  // bullet3
#define W_DCANNON1 164 // dcannon1
#define W_DIMNMISL 165 // dimnmisl
#define W_DISRUPTR 166 // disruptr
#define W_ENTROPY 168  // entropy
#define W_MARSEC1 169  // marsec1
#define W_MARSEC2 170  // marsec2
#define W_MEGAPOL 171  // megapol
#define W_MEGASTUN 172 // megastun
#define W_MEGCANON 173 // megcanon
#define W_MEGHIT 174   // meghit
#define W_POWERS 175   // powers
#define W_SNIPER 177   // sniper
#define W_TOXIGUN 181  // toxigun
#define W_TRAKGUN 182  // trakgun
#define W_TRAKHIT 183  // trakgun
#define W_ZAPHIT 184   // zaphit

#define DT_SMOKE 0
#define DT_AG 1
#define DT_INCENDARY 2
#define DT_STUNGAS 3
#define DT_EXPLOSIVE 4
#define DT_STUNGUN 5
#define DT_PSIBLAST 6
#define DT_LASER 8
#define DT_PLASMA 9
#define DT_TOXINA 10
#define DT_TOXINB 11
#define DT_TOXINC 12
#define DT_DISRUPTOR 14
#define DT_EXPLOSIVE2 15
#define DT_BRAINSUCKER 18
#define DT_ENTROPY 16

#define DM_HUMAN 0
#define DM_MUTANT 1

#define IT_MACHINEGUN 7
#define IT_LASERSNIPER 9
#define IT_AUTOCANNON 11
#define IT_HEAVYLAUNCHER 17
#define IT_MINILAUNCHER 21
#define IT_STUNGRAPPLE 25
#define IT_TRACKERGUN 28
#define IT_FORCEWEB 31
#define IT_DISRUPTOR 40
#define IT_DEVASTATOR 41
#define IT_BOOMEROID 42
#define IT_BRAINSUCKERLAUNCHER 44
#define IT_ENTROPYLAUNCHER 45
#define IT_DIMENSIONLAUNCHER 46
#define IT_DIMENSIONMISSILE 47
#define IT_VORTEX 48
#define IT_PERSHIELD 49
#define IT_PERTELEPORT 50
#define IT_PERCLOAK 51
#define IT_BRAINSUCKERPOD 56
#define IT_ENTROPYPOD 57
#define IT_POPPERBOMB 81

namespace OpenApoc
{

void InitialGameStateExtractor::extractAlienEquipmentSets(GameState &state,
                                                          Difficulty difficulty) const
{
	auto &data_t = this->tacp;
	auto &data_u = this->ufo2p;

	// Equipment sets - score - alien
	{
		if (data_t.agent_equipment_set_score_requirement->count() != 1)
			LogError("Incorrect amount of alien score requirement structures: encountered %u, "
			         "expected 1",
			         (unsigned)data_t.agent_equipment_set_score_requirement->count());
		auto sdata = data_t.agent_equipment_set_score_requirement->get(0);

		if (data_t.agent_equipment_set_score_alien->count() != 1)
			LogError("Incorrect amount of alien score equipment set structures: encountered %u, "
			         "expected 1",
			         (unsigned)data_t.agent_equipment_set_score_alien->count());
		auto data = data_t.agent_equipment_set_score_alien->get(0);
		for (unsigned i = 0; i < 8; i++)
		{
			auto es = mksp<EquipmentSet>();

			UString id = format("%sALIEN_%d", EquipmentSet::getPrefix(), (int)i + 1);
			es->id = id;

			for (unsigned j = 0; j < 10; j++)
			{
				if (data.weapons[j][i].weapon_idx > 0)
				{
					if (data.weapons[j][i].clip_idx > 0)
					{
						es->weapons.push_back(
						    {{&state, format("%s%s", AEquipmentType::getPrefix(),
						                     canon_string(data_u.agent_equipment_names->get(
						                         data.weapons[j][i].weapon_idx)))},
						     {&state, format("%s%s", AEquipmentType::getPrefix(),
						                     canon_string(data_u.agent_equipment_names->get(
						                         data.weapons[j][i].clip_idx)))},
						     std::max((int)data.weapons[j][i].clip_amount, 1)});
					}
					else
					{
						es->weapons.push_back(
						    {{&state, format("%s%s", AEquipmentType::getPrefix(),
						                     canon_string(data_u.agent_equipment_names->get(
						                         data.weapons[j][i].weapon_idx)))}});
					}
				}
				if (data.grenades[j][i].grenade_idx > 0 && data.grenades[j][i].grenade_amount > 0)
				{
					es->grenades.push_back(
					    {{&state, format("%s%s", AEquipmentType::getPrefix(),
					                     canon_string(data_u.agent_equipment_names->get(
					                         data.grenades[j][i].grenade_idx)))},
					     data.grenades[j][i].grenade_amount});
				}
				if (data.equipment[j][i][0] > 0 || data.equipment[j][i][1] > 0)
				{
					if (data.equipment[j][i][0] > 0 && data.equipment[j][i][1] > 0)
					{
						es->equipment.push_back(
						    {{&state, format("%s%s", AEquipmentType::getPrefix(),
						                     canon_string(data_u.agent_equipment_names->get(
						                         data.equipment[j][i][0])))},
						     {&state, format("%s%s", AEquipmentType::getPrefix(),
						                     canon_string(data_u.agent_equipment_names->get(
						                         data.equipment[j][i][1])))}});
					}
					else if (data.equipment[j][i][0] > 0)
					{
						es->equipment.push_back(
						    {{&state, format("%s%s", AEquipmentType::getPrefix(),
						                     canon_string(data_u.agent_equipment_names->get(
						                         data.equipment[j][i][0])))}});
					}
					else
					{
						es->equipment.push_back(
						    {{&state, format("%s%s", AEquipmentType::getPrefix(),
						                     canon_string(data_u.agent_equipment_names->get(
						                         data.equipment[j][i][1])))}});
					}
				}
			}

			int diff = 0;
			switch (difficulty)
			{
				case Difficulty::DIFFICULTY_1:
					diff = 0;
					break;
				case Difficulty::DIFFICULTY_2:
					diff = 1;
					break;
				case Difficulty::DIFFICULTY_3:
					diff = 2;
					break;
				case Difficulty::DIFFICULTY_4:
					diff = 3;
					break;
				case Difficulty::DIFFICULTY_5:
					diff = 4;
					break;
				default:
					LogError("Unknown difficulty");
			}

			es->min_score =
			    i == 0 ? std::numeric_limits<int>::min() : (int)sdata.score[diff][i - 1];
			es->max_score = i == 7 ? std::numeric_limits<int>::max() : (int)sdata.score[diff][i];

			state.equipment_sets_by_score[id] = es;
		}
	}
}

void InitialGameStateExtractor::extractAgentEquipment(GameState &state) const
{
	auto &data_t = this->tacp;
	auto &data_u = this->ufo2p;

	auto gameObjectSpriteTabFileName = UString("xcom3/tacdata/gameobj.tab");
	auto gameObjectSpriteTabFile = fw().data->fs.open(gameObjectSpriteTabFileName);
	if (!gameObjectSpriteTabFile)
	{
		LogError("Failed to open dropped item sprite TAB file \"%s\"", gameObjectSpriteTabFileName);
		return;
	}
	size_t gameObjectSpriteCount = gameObjectSpriteTabFile.size() / 4;

	auto gameObjectShadowSpriteTabFileName = UString("xcom3/tacdata/oshadow.tab");
	auto gameObjectShadowSpriteTabFile = fw().data->fs.open(gameObjectShadowSpriteTabFileName);
	if (!gameObjectShadowSpriteTabFile)
	{
		LogError("Failed to open shadow dropped item sprite TAB file \"%s\"",
		         gameObjectShadowSpriteTabFileName);
		return;
	}
	size_t gameObjectShadowSpriteCount = gameObjectShadowSpriteTabFile.size() / 4;

	auto heldSpriteTabFileName = UString("xcom3/tacdata/unit/equip.tab");
	auto heldSpriteTabFile = fw().data->fs.open(heldSpriteTabFileName);
	if (!heldSpriteTabFile)
	{
		LogError("Failed to open held item sprite TAB file \"%s\"", heldSpriteTabFileName);
		return;
	}
	size_t heldSpriteCount = heldSpriteTabFile.size() / 4 / 8;

	std::map<int, sp<AEquipmentType>> weapons;
	UString tracker_gun_clip_id = "";

	// Hazards
	{
		UString id = format("%s%s", HazardType::getPrefix(), "STUN_GAS");
		auto h = mksp<HazardType>();
		h->doodadType = {&state, "DOODAD_20_STUN_GAS"};
		h->minLifetime = 1;
		h->maxLifetime = 3;
		state.hazard_types[id] = h;
	}
	{
		UString id = format("%s%s", HazardType::getPrefix(), "ALIEN_GAS");
		auto h = mksp<HazardType>();
		h->doodadType = {&state, "DOODAD_19_ALIEN_GAS"};
		// FIXME: Confirm these values
		h->minLifetime = 1;
		h->maxLifetime = 3;
		state.hazard_types[id] = h;
	}
	{
		UString id = format("%s%s", HazardType::getPrefix(), "SMOKE");
		auto h = mksp<HazardType>();
		h->doodadType = {&state, "DOODAD_18_SMOKE"};
		h->minLifetime = 12;
		h->maxLifetime = 24;
		state.hazard_types[id] = h;
	}
	{
		UString id = format("%s%s", HazardType::getPrefix(), "FIRE");
		auto h = mksp<HazardType>();
		h->doodadType = {&state, "DOODAD_17_FIRE"};
		// Fire has a starting deviation of 0 to 2, fire's ttl works in a completely different way
		h->minLifetime = 0;
		h->maxLifetime = 1;
		h->fire = true;
		h->sound = fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/burning.raw:22050");
		state.hazard_types[id] = h;
	}

	for (unsigned j = 0; j <= data_t.damage_type_names->count(); j++)
	{
		// extra enzyme entry for the purpose of implementing the entropy launcher
		unsigned i = j == data_t.damage_type_names->count() ? DT_ENTROPY : j;
		auto d = mksp<DamageType>();

		UString id = data_t.getDTypeId(i);
		if (i != j)
		{
			id = format("%s_SPECIAL", id);
			d->effectType = DamageType::EffectType::Enzyme;
		}

		d->name = data_t.damage_type_names->get(i);

		d->ignore_shield =
		    (i < data_t.damage_types->count()) && (data_t.damage_types->get(i).ignore_shield == 1);

		// Damage icons are located in tacdata icons, starting with id 14 and on
		d->icon_sprite = fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
		                                             "icons.tab:%d:xcom3/tacdata/tactical.pal",
		                                             (int)i + 14));
		switch (i)
		{
			case DT_SMOKE:
				d->explosive = true;
				d->blockType = DamageType::BlockType::Gas;
				d->effectType = DamageType::EffectType::Smoke;
				d->explosionDoodad = {&state, "DOODAD_18_SMOKE"};
				d->hazardType = {&state, "HAZARD_SMOKE"};
				break;
			case DT_AG:
				d->explosive = true;
				d->blockType = DamageType::BlockType::Gas;
				d->explosionDoodad = {&state, "DOODAD_19_ALIEN_GAS"};
				d->hazardType = {&state, "HAZARD_ALIEN_GAS"};
				d->non_violent = true;
				break;
			case DT_INCENDARY:
				d->explosive = true;
				d->effectType = DamageType::EffectType::Fire;
				// uses default explosion doodad
				d->hazardType = {&state, "HAZARD_FIRE"};
				break;
			case DT_STUNGAS:
				d->explosive = true;
				d->blockType = DamageType::BlockType::Gas;
				d->effectType = DamageType::EffectType::Stun;
				d->explosionDoodad = {&state, "DOODAD_20_STUN_GAS"};
				d->hazardType = {&state, "HAZARD_STUN_GAS"};
				break;
			case DT_EXPLOSIVE:
			case DT_EXPLOSIVE2:
				d->explosive = true;
				break;
			case DT_LASER:
			case DT_PLASMA:
			case DT_TOXINA:
			case DT_TOXINB:
			case DT_TOXINC:
			case DT_DISRUPTOR:
				d->non_violent = true;
				break;
			case DT_STUNGUN:
				d->effectType = DamageType::EffectType::Stun;
				break;
			case DT_PSIBLAST:
				d->explosive = true;
				d->blockType = DamageType::BlockType::Psionic;
				break;
			case DT_BRAINSUCKER:
				d->effectType = DamageType::EffectType::Brainsucker;
				break;
		}

		state.damage_types[id] = d;
	}

	// Explosion sounds for damage types
	{
		state.damage_types["DAMAGETYPE_ANTI-ALIEN_GAS"]->explosionSounds.push_back(
		    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/explosns/gasexpls.raw:22050"));
		state.damage_types["DAMAGETYPE_EXPLOSIVE"]->explosionSounds.push_back(
		    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/explosns/explosn1.raw:22050"));
		state.damage_types["DAMAGETYPE_EXPLOSIVE"]->explosionSounds.push_back(
		    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/explosns/explosn2.raw:22050"));
		state.damage_types["DAMAGETYPE_EXPLOSIVE_1"]->explosionSounds.push_back(
		    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/explosns/explosn1.raw:22050"));
		state.damage_types["DAMAGETYPE_EXPLOSIVE_1"]->explosionSounds.push_back(
		    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/explosns/explosn2.raw:22050"));
		state.damage_types["DAMAGETYPE_INCENDIARY"]->explosionSounds.push_back(
		    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/explosns/firexpls.raw:22050"));
		state.damage_types["DAMAGETYPE_PSIONIC_BLAST"]->explosionSounds.push_back(
		    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/explosns/psigrnad.raw:22050"));
		state.damage_types["DAMAGETYPE_SMOKE"]->explosionSounds.push_back(
		    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/explosns/gasexpls.raw:22050"));
		state.damage_types["DAMAGETYPE_STUN_GAS"]->explosionSounds.push_back(
		    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/explosns/gasexpls.raw:22050"));
	}

	for (unsigned i = 0; i < data_t.damage_modifiers->count(); i++)
	{
		auto d = mksp<DamageModifier>();
		auto ddata = data_t.damage_modifiers->get(i);

		UString id = data_t.getDModId(i);

		state.damage_modifiers[id] = d;

		for (unsigned j = 0; j < 18; j++)
		{
			state.damage_types[data_t.getDTypeId(j)]->modifiers[{&state, id}] =
			    ddata.damage_type_data[j];
			// Extra entry for special entropy damage type
			if (j == DT_ENTROPY)
			{
				state.damage_types[format("%s_SPECIAL", data_t.getDTypeId(j))]
				    ->modifiers[{&state, id}] = ddata.damage_type_data[j];
			}
		}

		state.damage_types[data_t.getDTypeId(DT_BRAINSUCKER)]->modifiers[{&state, id}] =
		    (i == DM_HUMAN || i == DM_MUTANT) ? 100 : 0;
	}

	for (unsigned i = 0; i < data_t.agent_equipment->count(); i++)
	{
		auto e = mksp<AEquipmentType>();
		auto edata = data_t.agent_equipment->get(i);

		e->name = data_u.agent_equipment_names->get(i);
		UString id = format("%s%s", AEquipmentType::getPrefix(), canon_string(e->name));

		e->id = id;

		// Mark two-handed items
		switch (edata.sprite_idx)
		{
			case IT_MACHINEGUN:
			case IT_LASERSNIPER:
			case IT_AUTOCANNON:
			case IT_HEAVYLAUNCHER:
			case IT_MINILAUNCHER:
			case IT_STUNGRAPPLE:
			case IT_TRACKERGUN:
			case IT_FORCEWEB:
			case IT_DEVASTATOR:
			case IT_BRAINSUCKERLAUNCHER:
			case IT_DIMENSIONLAUNCHER:
				e->two_handed = true;
				break;
			default:
				e->two_handed = false;
				break;
		}

		// Dependency
		e->research_dependency.type = ResearchDependency::Type::All;
		switch (edata.sprite_idx)
		{
			case IT_DISRUPTOR:
				e->research_dependency.topics.emplace(&state, "RESEARCH_DISRUPTOR_GUN");
				break;
			case IT_DEVASTATOR:
				e->research_dependency.topics.emplace(&state, "RESEARCH_DEVASTATOR_CANNON");
				break;
			case IT_BOOMEROID:
				e->research_dependency.topics.emplace(&state, "RESEARCH_BOOMEROID");
				break;
			case IT_BRAINSUCKERLAUNCHER:
				e->research_dependency.topics.emplace(&state, "RESEARCH_BRAINSUCKER_LAUNCHER");
				e->research_dependency.topics.emplace(&state, "RESEARCH_BRAINSUCKER_PODS");
				break;
			case IT_BRAINSUCKERPOD:
				e->bioStorage = true;
				e->bioRemains = {&state, "AEQUIPMENTTYPE_BRAINSUCKER_ALIVE"};
				e->store_space = 1;
				e->research_dependency.topics.emplace(&state, "RESEARCH_BRAINSUCKER_LAUNCHER");
				e->research_dependency.topics.emplace(&state, "RESEARCH_BRAINSUCKER_PODS");
				break;
			case IT_ENTROPYLAUNCHER:
				e->research_dependency.topics.emplace(&state, "RESEARCH_ENTROPY_LAUNCHER");
				e->research_dependency.topics.emplace(&state, "RESEARCH_ENTROPY_POD");
				break;
			case IT_DIMENSIONLAUNCHER:
				e->research_dependency.topics.emplace(&state,
				                                      "RESEARCH_DIMENSION_MISSILE_LAUNCHER");
				e->research_dependency.topics.emplace(&state, "RESEARCH_DIMENSION_MISSILE");
				break;
			case IT_DIMENSIONMISSILE:
				e->research_dependency.topics.emplace(&state,
				                                      "RESEARCH_DIMENSION_MISSILE_LAUNCHER");
				e->research_dependency.topics.emplace(&state, "RESEARCH_DIMENSION_MISSILE");
				break;
			case IT_VORTEX:
				e->research_dependency.topics.emplace(&state, "RESEARCH_VORTEX_MINE");
				break;
			case IT_PERSHIELD:
				e->research_dependency.topics.emplace(&state, "RESEARCH_PERSONAL_DISRUPTOR_SHIELD");
				break;
			case IT_PERTELEPORT:
				e->research_dependency.topics.emplace(&state, "RESEARCH_PERSONAL_TELEPORTER");
				break;
			case IT_PERCLOAK:
				e->research_dependency.topics.emplace(&state, "RESEARCH_PERSONAL_CLOAKING_FIELD");
				break;
			case IT_ENTROPYPOD:
				e->research_dependency.topics.emplace(&state, "RESEARCH_ENTROPY_LAUNCHER");
				e->research_dependency.topics.emplace(&state, "RESEARCH_ENTROPY_POD");
				break;
		}

		// Mark brainsucker launcher
		if (edata.sprite_idx == 44)
		{
			e->launcher = true;
		}

		e->artifact = edata.artifact != 0;

		unsigned payload_idx = std::numeric_limits<unsigned>::max();
		switch (edata.type)
		{
			case AGENT_EQUIPMENT_TYPE_ARMOR:
			{
				auto adata = data_t.agent_armor->get(edata.data_idx);
				e->type = AEquipmentType::Type::Armor;
				e->damage_modifier = {&state, data_t.getDModId(adata.damage_modifier)};
				e->armor = adata.armor;
				e->max_ammo = adata.armor;
				UString bodyPartLetter = "";
				int armoredUnitPicIndex = 0;
				int armorBodyPicIndex = 0;
				switch (adata.body_part)
				{
					case AGENT_ARMOR_BODY_PART_LEGS:
						bodyPartLetter = "b";
						armorBodyPicIndex = 4;
						e->body_part = BodyPart::Legs;
						break;
					case AGENT_ARMOR_BODY_PART_BODY:
						bodyPartLetter = "a";
						armorBodyPicIndex = 2;
						e->body_part = BodyPart::Body;
						// Vanilla decides if flight is enabled by checking if a "marsec" damage mod
						// body armor is equipped
						if (adata.damage_modifier == 18)
							e->provides_flight = true;
						break;
					case AGENT_ARMOR_BODY_PART_LEFT_ARM:
						bodyPartLetter = "d";
						armorBodyPicIndex = 3;
						e->body_part = BodyPart::LeftArm;
						break;
					case AGENT_ARMOR_BODY_PART_RIGHT_ARM:
						bodyPartLetter = "e";
						armorBodyPicIndex = 1;
						e->body_part = BodyPart::RightArm;
						break;
					case AGENT_ARMOR_BODY_PART_HELMET:
						bodyPartLetter = "c";
						armorBodyPicIndex = 0;
						e->body_part = BodyPart::Helmet;
						break;
					default:
						LogError("Unexpected body part type %d for ID %s", (int)adata.body_part,
						         id);
				}
				switch (adata.damage_modifier)
				{
					case 17:
						armoredUnitPicIndex = 2;
						break;
					case 18:
						armoredUnitPicIndex = 3;
						break;
					case 19:
						armoredUnitPicIndex = 4;
						break;
					default:
						LogError("Unexpected damage modifier %d for ID %s",
						         (int)adata.damage_modifier, id);
						break;
				}
				e->body_image_pack = {&state, format("%s%s%d%s", BattleUnitImagePack::getPrefix(),
				                                     "xcom", armoredUnitPicIndex, bodyPartLetter)};
				// Body sprites are stored in armour.pck file, in head-left-body-right-legs order
				// Since armor damage modifier values start with 17, we can subtract that to get
				// armor index
				e->body_sprite = fw().data->loadImage(
				    format("PCK:xcom3/ufodata/armour.pck:xcom3/ufodata/"
				           "armour.tab:%d:xcom3/tacdata/equip.pal",
				           (int)((adata.damage_modifier - 17) * 5 + armorBodyPicIndex)));
			}
			break;
			case AGENT_EQUIPMENT_TYPE_WEAPON:
			{
				auto wdata = data_t.agent_weapons->get(edata.data_idx);
				if (wdata.ammo_effect[0] == 255)
				{
					e->type = (edata.sprite_idx == IT_POPPERBOMB) ? AEquipmentType::Type::Popper
					                                              : AEquipmentType::Type::Grenade;
					payload_idx = wdata.grenade_effect;
					e->max_ammo = 1;
					e->recharge = 0;
				}
				else
				{
					e->type = AEquipmentType::Type::Weapon;
					bool has_ammo = false;
					// Entry #0 is "EMPTY" so we skip it
					for (unsigned j = 1; j < data_t.agent_general->count(); j++)
					{
						if (has_ammo)
							break;
						auto gdata = data_t.agent_general->get(j);
						// Fix for buggy tracer gun ammo
						if (gdata.ammo_type == 0xffff && gdata.ammo_type_duplicate == 1)
						{
							if (wdata.ammo_type == 8)
								has_ammo = true;
							continue;
						}
						// Look for a clip with matching ammo type
						if ((gdata.ammo_type != 0xffff
						         ? gdata.ammo_type
						         : gdata.ammo_type_duplicate) == wdata.ammo_type)
							has_ammo = true;
					}

					if (has_ammo)
					{
						// All proper weapons have ammo types defined after them, so we can add ammo
						// when we parse it
						weapons[wdata.ammo_type] = e;
					}
					else
					{
						payload_idx = wdata.ammo_effect[0];
						e->max_ammo = wdata.ammo_rounds[0];
						// Alien weapons start from index 29, and are not marked as recharging
						// despite doing so. Therefore, mark them manually
						e->recharge = (edata.data_idx > 28) ? 1 : wdata.ammo_recharge[0];
						if (e->recharge > 0)
						{
							e->rechargeTB = e->max_ammo;
						}
					}
				}
			}
			break;
			case AGENT_EQUIPMENT_TYPE_GENERAL:
			{
				auto gdata = data_t.agent_general->get(edata.data_idx);
				switch (gdata.type)
				{
					case AGENT_GENERAL_TYPE_AMMO_OR_LOOT:
						if (gdata.ammo_effect == 255)
						{
							e->type = AEquipmentType::Type::Loot;
						}
						else
						{
							e->type = AEquipmentType::Type::Ammo;
							payload_idx = gdata.ammo_effect;
							e->max_ammo = gdata.ammo_rounds;
							e->recharge = gdata.ammo_recharge;
							e->rechargeTB = gdata.ammo_rounds;
							if (gdata.ammo_type == 65535 && gdata.ammo_type_duplicate == 1)
								// Buggy tracker gun clip has incorrect values in these fields AND
								// comes before it's weapon!
								tracker_gun_clip_id = id;
							else
								// Behave normally
								e->weapon_types.emplace_back(
								    &state,
								    weapons[gdata.ammo_type != 0xffff ? gdata.ammo_type
								                                      : gdata.ammo_type_duplicate]
								        ->id);
						}
						break;
					case AGENT_GENERAL_TYPE_MOTION_SCANNER:
						e->type = AEquipmentType::Type::MotionScanner;
						break;
					case AGENT_GENERAL_TYPE_STRUCTURE_PROBE:
						e->type = AEquipmentType::Type::StructureProbe;
						break;
					case AGENT_GENERAL_TYPE_VORTEX_ANALYZER:
						e->type = AEquipmentType::Type::VortexAnalyzer;
						break;
					case AGENT_GENERAL_TYPE_MULTI_TRACKER:
						e->type = AEquipmentType::Type::MultiTracker;
						break;
					case AGENT_GENERAL_TYPE_MIND_SHIELD:
						e->type = AEquipmentType::Type::MindShield;
						break;
					case AGENT_GENERAL_TYPE_MIND_BENDER:
						e->type = AEquipmentType::Type::MindBender;
						break;
					case AGENT_GENERAL_TYPE_ALIEN_DETECTOR:
						e->type = AEquipmentType::Type::AlienDetector;
						break;
					case AGENT_GENERAL_TYPE_DISRUPTOR_SHIELD:
						e->type = AEquipmentType::Type::DisruptorShield;
						e->damage_modifier = {&state, data_t.getDModId(16)};
						e->max_ammo = 100;
						e->recharge = 1;
						e->rechargeTB = 12;
						e->shield_graphic = {&state, "DOODAD_27_SHIELD"};
						break;
					case AGENT_GENERAL_TYPE_TELEPORTER:
						e->type = AEquipmentType::Type::Teleporter;
						e->max_ammo = gdata.ammo_rounds;
						e->recharge = gdata.ammo_recharge;
						e->rechargeTB = gdata.ammo_rounds;
						break;
					case AGENT_GENERAL_TYPE_CLOAKING_FIELD:
						e->type = AEquipmentType::Type::CloakingField;
						break;
					case AGENT_GENERAL_TYPE_DIMENSION_FORCE_FIELD:
						e->type = AEquipmentType::Type::DimensionForceField;
						break;
					case AGENT_GENERAL_TYPE_MEDI_KIT:
						e->type = AEquipmentType::Type::MediKit;
						break;
					default:
						LogError("Unexpected general type %d for ID %s", (int)gdata.type, id);
				}
			}
			break;
			default:
				LogInfo("Encountered empty item in ID %s, moving on", id);
				continue;
		}

		e->weight = edata.weight;

		e->shadow_offset = BATTLE_SHADOW_OFFSET;
		e->dropped_offset = BATTLE_IMAGE_OFFSET;

		if (edata.sprite_idx < gameObjectSpriteCount)
			e->dropped_sprite =
			    fw().data->loadImage(format("PCK:xcom3/tacdata/gameobj.pck:xcom3/tacdata/"
			                                "gameobj.tab:%d",
			                                (int)edata.sprite_idx));

		if (edata.sprite_idx < gameObjectShadowSpriteCount)
			e->dropped_shadow_sprite =
			    fw().data->loadImage(format("PCKSHADOW:xcom3/tacdata/oshadow.pck:xcom3/tacdata/"
			                                "oshadow.tab:%d",
			                                (int)edata.sprite_idx));

		// Held sprites begin from 0, which corresponds to item 1, Megapol AP Grenade
		// Armor pieces go last, and held sprites for every single item after the first armor piece
		// are identical
		// There is a total 60 of them
		int held_sprite_index = std::min((int)edata.sprite_idx, (int)heldSpriteCount - 1);
		e->held_image_pack = {
		    &state, format("%s%s%d", BattleUnitImagePack::getPrefix(), "item", held_sprite_index)};

		e->equipscreen_sprite = fw().data->loadImage(format(
		    "PCK:xcom3/ufodata/pequip.pck:xcom3/ufodata/pequip.tab:%d:xcom3/tacdata/tactical.pal",
		    (int)edata.sprite_idx));
		e->equipscreen_size = {edata.size_x, edata.size_y};

		e->manufacturer = {&state, data_u.getOrgId(edata.manufacturer)};

		e->store_space = edata.store_space;
		e->armor = edata.armor;
		e->score = edata.score;

		// Read payload information
		// Only for Ammo, or weapons and grenades with built-in ammo
		if (payload_idx != std::numeric_limits<unsigned>::max())
		{
			if (payload_idx >= data_t.agent_payload->count())
				LogError("Invalid payload index %u for ID %s", payload_idx, id);

			auto pdata = data_t.agent_payload->get(payload_idx);

			e->speed = pdata.speed;
			e->damage = pdata.damage;
			e->accuracy = 100 - pdata.accuracy;
			e->fire_delay = pdata.fire_delay * TICKS_MULTIPLIER;
			e->guided = pdata.guided != 0 ? true : false;
			e->turn_rate = pdata.turn_rate;
			e->ttl = pdata.ttl;
			e->range = pdata.range;
			UString doodad_id = "";
			switch (pdata.explosion_graphic)
			{
				case TAC_DOODAD_21: // tac 115 - 125
					doodad_id = "DOODAD_21_AP";
					break;
				case TAC_DOODAD_22: // tac 126 - 136
					doodad_id = "DOODAD_22_LASER";
					break;
				case TAC_DOODAD_23: // tac 137 - 147
					doodad_id = "DOODAD_23_PLASMA";
					break;
				case TAC_DOODAD_24: // tac 148 - 158
					doodad_id = "DOODAD_24_DISRUPTOR";
					break;
				case TAC_DOODAD_25: // tac 159 - 169
					doodad_id = "DOODAD_25_DEVASTATOR";
					break;
				case TAC_DOODAD_26: // tac 170 - 180
					doodad_id = "DOODAD_26_STUN";
					break;
				case TAC_DOODAD_27: // tac 181 - 185 shield
					doodad_id = "DOODAD_27_SHIELD";
					break;
				case TAC_DOODAD_28: // tac 186 - 192
					doodad_id = "DOODAD_28_ENZYME";
					break;
			}
			if (doodad_id != "")
			{
				e->explosion_graphic = {&state, doodad_id};
			}

			UString fire_sfx_path = "";
			switch (pdata.fire_sfx)
			{
				case E_TRONLAUN:
					fire_sfx_path = "extra/tronlaun";
					break;
				case A_QUEENWHP:
					fire_sfx_path = "tactical/aliens/attacks/queenwhp";
					break;
				case A_SPITTER:
					fire_sfx_path = "tactical/aliens/attacks/spitter";
					break;
				case A_WORMSPIT:
					fire_sfx_path = "tactical/aliens/attacks/wormspit";
					break;
				case A_WRMATTAK:
					fire_sfx_path = "tactical/aliens/attacks/wrmattak";
					break;
				case W_BULLET1:
					fire_sfx_path = "tactical/weapons/bullet1";
					break;
				case W_BULLET2:
					fire_sfx_path = "tactical/weapons/bullet2";
					break;
				case W_BULLET3:
					fire_sfx_path = "tactical/weapons/bullet3";
					break;
				case W_DCANNON1:
					fire_sfx_path = "tactical/weapons/dcannon1";
					break;
				case W_DIMNMISL:
					fire_sfx_path = "tactical/weapons/dimnmisl";
					break;
				case W_DISRUPTR:
					fire_sfx_path = "tactical/weapons/disruptr";
					break;
				case W_ENTROPY:
					fire_sfx_path = "tactical/weapons/entropy";
					break;
				case W_MARSEC1:
					fire_sfx_path = "tactical/weapons/marsec1";
					break;
				case W_MARSEC2:
					fire_sfx_path = "tactical/weapons/marsec2";
					break;
				case W_MEGAPOL:
					fire_sfx_path = "tactical/weapons/megapol";
					break;
				case W_MEGASTUN:
					fire_sfx_path = "tactical/weapons/megastun";
					break;
				case W_MEGCANON:
					fire_sfx_path = "tactical/weapons/megcanon";
					break;
				case W_MEGHIT:
					fire_sfx_path = "tactical/weapons/meghit";
					break;
				case W_POWERS:
					fire_sfx_path = "tactical/weapons/powers";
					break;
				case W_SNIPER:
					fire_sfx_path = "tactical/weapons/sniper";
					break;
				case W_TOXIGUN:
					fire_sfx_path = "tactical/weapons/toxigun";
					break;
				case W_TRAKGUN:
					fire_sfx_path = "tactical/weapons/trakgun";
					break;
				case W_TRAKHIT:
					fire_sfx_path = "tactical/weapons/trakhit";
					break;
				case W_ZAPHIT:
					fire_sfx_path = "tactical/weapons/zaphit";
					break;
			}
			if (fire_sfx_path != "")
			{
				e->fire_sfx = fw().data->loadSample("RAWSOUND:xcom3/rawsound/" + fire_sfx_path +
				                                    ".raw:22050");
			}

			UString impact_sfx_path = "";
			switch (pdata.impact_sfx)
			{
				case E_TRONLAUN:
					impact_sfx_path = "extra/tronlaun";
					break;
				case A_QUEENWHP:
					impact_sfx_path = "tactical/aliens/attacks/queenwhp";
					break;
				case A_SPITTER:
					impact_sfx_path = "tactical/aliens/attacks/spitter";
					break;
				case A_WORMSPIT:
					impact_sfx_path = "tactical/aliens/attacks/wormspit";
					break;
				case A_WRMATTAK:
					impact_sfx_path = "tactical/aliens/attacks/wrmattak";
					break;
				case W_BULLET1:
					impact_sfx_path = "tactical/weapons/bullet1";
					break;
				case W_BULLET2:
					impact_sfx_path = "tactical/weapons/bullet2";
					break;
				case W_BULLET3:
					impact_sfx_path = "tactical/weapons/bullet3";
					break;
				case W_DCANNON1:
					impact_sfx_path = "tactical/weapons/dcannon1";
					break;
				case W_DIMNMISL:
					impact_sfx_path = "tactical/weapons/dimnmisl";
					break;
				case W_DISRUPTR:
					impact_sfx_path = "tactical/weapons/disruptr";
					break;
				case W_ENTROPY:
					impact_sfx_path = "tactical/weapons/entropy";
					break;
				case W_MARSEC1:
					impact_sfx_path = "tactical/weapons/marsec1";
					break;
				case W_MARSEC2:
					impact_sfx_path = "tactical/weapons/marsec2";
					break;
				case W_MEGAPOL:
					impact_sfx_path = "tactical/weapons/megapol";
					break;
				case W_MEGASTUN:
					impact_sfx_path = "tactical/weapons/megastun";
					break;
				case W_MEGCANON:
					impact_sfx_path = "tactical/weapons/megcanon";
					break;
				case W_MEGHIT:
					impact_sfx_path = "tactical/weapons/meghit";
					break;
				case W_POWERS:
					impact_sfx_path = "tactical/weapons/powers";
					break;
				case W_SNIPER:
					impact_sfx_path = "tactical/weapons/sniper";
					break;
				case W_TOXIGUN:
					impact_sfx_path = "tactical/weapons/toxigun";
					break;
				case W_TRAKGUN:
					impact_sfx_path = "tactical/weapons/trakgun";
					break;
				case W_TRAKHIT:
					impact_sfx_path = "tactical/weapons/trakhit";
					break;
				case W_ZAPHIT:
					impact_sfx_path = "tactical/weapons/zaphit";
					break;
			}
			if (impact_sfx_path != "")
			{
				e->impact_sfx = fw().data->loadSample("RAWSOUND:xcom3/rawsound/" + impact_sfx_path +
				                                      ".raw:22050");
			}

			if (id == "AEQUIPMENTTYPE_ENTROPY_POD")
			{
				// Change entropy pod's damage type to the one that applies debuff
				e->damage_type = {&state, "DAMAGETYPE_ENTROPY_ENZYME_SPECIAL"};
			}
			else
			{
				e->damage_type = {&state, data_t.getDTypeId(pdata.damage_type)};
			}
			if (id == "AEQUIPMENTTYPE_BRAINSUCKER_POD")
			{
				e->trigger_type = TriggerType::Proximity;
			}
			else
			{
				switch (pdata.trigger_type)
				{
					case AGENT_GRENADE_TRIGGER_TYPE_NORMAL:
						e->trigger_type = TriggerType::Timed;
						break;
					case AGENT_GRENADE_TRIGGER_TYPE_PROXIMITY:
						e->trigger_type = TriggerType::Proximity;
						break;
					case AGENT_GRENADE_TRIGGER_TYPE_BOOMEROID:
						e->trigger_type = TriggerType::Boomeroid;
						break;
					default:
						LogError("Unexpected grenade trigger type %d for ID %s",
						         (int)pdata.trigger_type, id);
				}
			}
			e->explosion_depletion_rate = pdata.explosion_depletion_rate;

			auto projectile_sprites = data_t.projectile_sprites->get(pdata.projectile_image);
			// Determine tail size
			e->tail_size = 0;
			for (int i = 0; i < 36; i++)
			{
				if (projectile_sprites.sprites[i] != 255)
				{
					e->tail_size = i + 1;
				}
			}

			for (int i = 0; i < e->tail_size; i++)
			{
				UString sprite_path = "";
				if (projectile_sprites.sprites[i] != 255)
				{
					sprite_path = format("bulletsprites/battle/%02u.png",
					                     (unsigned)projectile_sprites.sprites[i]);
				}
				else
				{
					sprite_path = ""; // a 'gap' in the projectile trail
				}
				e->projectile_sprites.push_back(fw().data->loadImage(sprite_path));
			}
		}

		state.agent_equipment[id] = e;
	}

	// Fix the tracker gun ammo
	state.agent_equipment[tracker_gun_clip_id]->weapon_types.emplace_back(&state, weapons[8]->id);

	// Equipment sets - score (level) - human
	{
		if (data_t.agent_equipment_set_score_human->count() != 1)
			LogError("Incorrect amount of human score equipment set structures: encountered %u, "
			         "expected 1",
			         (unsigned)data_t.agent_equipment_set_score_human->count());
		auto data = data_t.agent_equipment_set_score_human->get(0);
		for (unsigned i = 0; i < 12; i++)
		{
			for (unsigned i = 0; i < 12; i++)
			{
				auto es = mksp<EquipmentSet>();

				UString id = format("%sHUMAN_%d", EquipmentSet::getPrefix(), (int)i + 1);
				es->id = id;

				for (unsigned j = 0; j < 10; j++)
				{
					if (data.weapons[j][i].weapon_idx > 0)
					{
						if (data.weapons[j][i].clip_idx > 0)
						{
							es->weapons.push_back(
							    {{&state, format("%s%s", AEquipmentType::getPrefix(),
							                     canon_string(data_u.agent_equipment_names->get(
							                         data.weapons[j][i].weapon_idx)))},
							     {&state, format("%s%s", AEquipmentType::getPrefix(),
							                     canon_string(data_u.agent_equipment_names->get(
							                         data.weapons[j][i].clip_idx)))},
							     std::max((int)data.weapons[j][i].clip_amount, 1)});
						}
						else
						{
							es->weapons.push_back(
							    {{&state, format("%s%s", AEquipmentType::getPrefix(),
							                     canon_string(data_u.agent_equipment_names->get(
							                         data.weapons[j][i].weapon_idx)))}});
						}
					}
					if (data.grenades[j][i].grenade_idx > 0 &&
					    data.grenades[j][i].grenade_amount > 0)
					{
						es->grenades.push_back(
						    {{&state, format("%s%s", AEquipmentType::getPrefix(),
						                     canon_string(data_u.agent_equipment_names->get(
						                         data.grenades[j][i].grenade_idx)))},
						     data.grenades[j][i].grenade_amount});
					}
					if (data.equipment[j][i][0] > 0 || data.equipment[j][i][1] > 0)
					{
						if (data.equipment[j][i][0] > 0 && data.equipment[j][i][1] > 0)
						{
							es->equipment.push_back(
							    {{&state, format("%s%s", AEquipmentType::getPrefix(),
							                     canon_string(data_u.agent_equipment_names->get(
							                         data.equipment[j][i][0])))},
							     {&state, format("%s%s", AEquipmentType::getPrefix(),
							                     canon_string(data_u.agent_equipment_names->get(
							                         data.equipment[j][i][1])))}});
						}
						else if (data.equipment[j][i][0] > 0)
						{
							es->equipment.push_back(
							    {{&state, format("%s%s", AEquipmentType::getPrefix(),
							                     canon_string(data_u.agent_equipment_names->get(
							                         data.equipment[j][i][0])))}});
						}
						else
						{
							es->equipment.push_back(
							    {{&state, format("%s%s", AEquipmentType::getPrefix(),
							                     canon_string(data_u.agent_equipment_names->get(
							                         data.equipment[j][i][1])))}});
						}
					}
				}

				es->min_score = i == 0 ? std::numeric_limits<int>::min() : i + 1;
				es->max_score = i == 11 ? std::numeric_limits<int>::max() : i + 2;

				state.equipment_sets_by_level[id] = es;
			}
		}
	}
}

} // namespace OpenApoc
