#include "game/ui/components/controlgenerator.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vequipment.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/shared/agent.h"

namespace OpenApoc
{
ControlGenerator ControlGenerator::singleton;

void ControlGenerator::init(GameState &state)
{
	for (int i = 0; i < 3; i++)
	{
		battleSelect.push_back(
		    fw().data->loadImage(format("PCK:xcom3/tacdata/tacbut.pck:xcom3/tacdata/"
		                                "tacbut.tab:%d:xcom3/tacdata/tactical.pal",
		                                25 + i)));
		citySelect.push_back(fw().data->loadImage(format(
		    "PCK:xcom3/ufodata/vs_icon.pck:xcom3/ufodata/vs_icon.tab:%d:xcom3/ufodata/pal_01.dat",
		    37 + i)));
	}

	iconFatal = fw().data->loadImage("battle/battle-fatal.png");
	iconPsiIn = fw().data->loadImage("battle/battle-psi-in.png");
	iconPsiOut = fw().data->loadImage("battle/battle-psi-out.png");

	for (int i = 28; i <= 34; i++)
	{
		unitRanks.push_back(
		    fw().data->loadImage(format("PCK:xcom3/tacdata/tacbut.pck:xcom3/tacdata/"
		                                "tacbut.tab:%d:xcom3/tacdata/tactical.pal",
		                                i)));
	}

	auto img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({0, 0}, Colour{255, 255, 219});
		l.set({0, 1}, Colour{215, 0, 0});
	}
	healthImage = img;
	img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({0, 0}, Colour{160, 236, 252});
		l.set({0, 1}, Colour{4, 100, 252});
	}
	shieldImage = img;
	img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({0, 0}, Colour{150, 150, 150});
		l.set({0, 1}, Colour{97, 101, 105});
	}
	stunImage = img;
	iconShade = fw().data->loadImage("battle/battle-icon-shade.png");

	for (int i = 47; i <= 50; i++)
	{
		icons.push_back(fw().data->loadImage(format(
		    "PCK:xcom3/ufodata/vs_icon.pck:xcom3/ufodata/vs_icon.tab:%s:xcom3/ufodata/pal_01.dat",
		    i)));
	}

	vehiclePassengerCountIcons.emplace_back();
	for (int i = 51; i <= 63; i++)
	{
		vehiclePassengerCountIcons.push_back(fw().data->loadImage(format(
		    "PCK:xcom3/ufodata/vs_icon.pck:xcom3/ufodata/vs_icon.tab:%s:xcom3/ufodata/pal_01.dat",
		    i)));
	}
	labelFont = ui().getFont("smalfont");

	purchaseControlParts.push_back(fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx",
	    45)));
	purchaseControlParts.push_back(fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx",
	    46)));

	purchaseBoxIcon = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 47));
	purchaseXComIcon = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 48));
	purchaseArrow = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 52));

	alienContainedDetain = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 75));
	alienContainedKill = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 76));

	scrollLeft = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 53));
	scrollRight = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 54));

	transactionShade = fw().data->loadImage("city/transaction-shade.png");

	initialised = true;
}

VehicleTileInfo ControlGenerator::createVehicleInfo(GameState &state, sp<Vehicle> v)
{
	VehicleTileInfo t;
	t.vehicle = v;
	t.selected = UnitSelectionState::Unselected;

	for (auto &veh : state.current_city->cityViewSelectedVehicles)
	{
		if (veh == v)
		{
			t.selected = (veh == state.current_city->cityViewSelectedVehicles.front())
			                 ? UnitSelectionState::FirstSelected
			                 : UnitSelectionState::Selected;
			break;
		}
	}

	float maxHealth;
	float currentHealth;
	if (v->getShield() != 0)
	{
		maxHealth = v->getMaxShield();
		currentHealth = v->getShield();
		t.shield = true;
	}
	else
	{
		maxHealth = v->getMaxHealth();
		currentHealth = v->getHealth();
		t.shield = false;
	}

	t.healthProportion = currentHealth / maxHealth;
	// Clamp passengers to 13 as anything beyond that gets the same icon
	t.passengers = std::min(13, v->getPassengers());
	t.faded = v->city != state.current_city;

	auto b = v->currentBuilding;
	if (b)
	{
		if (b == v->homeBuilding)
		{
			t.state = CityUnitState::InBase;
		}
		else
		{
			t.state = CityUnitState::InBuilding;
		}
	}
	else
	{
		t.state = CityUnitState::InMotion;
	}
	return t;
}

sp<Control> ControlGenerator::createVehicleControl(GameState &state, const VehicleTileInfo &info)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}

	auto frame = singleton.citySelect[(int)info.selected];
	auto baseControl = mksp<Graphic>(frame);
	baseControl->Size = frame->size;
	// FIXME: There's an extra 1 pixel here that's annoying
	baseControl->Size.x -= 1;
	baseControl->Name = "OWNED_VEHICLE_FRAME_" + info.vehicle->name;
	baseControl->setData(info.vehicle);

	auto vehicleIcon = baseControl->createChild<Graphic>(info.vehicle->type->icon);
	vehicleIcon->AutoSize = true;
	vehicleIcon->Location = {1, 1};
	vehicleIcon->Name = "OWNED_VEHICLE_ICON_" + info.vehicle->name;

	// FIXME: Put these somewhere slightly less magic?
	Vec2<int> healthBarOffset = {27, 2};
	Vec2<int> healthBarSize = {3, 20};

	auto healthImg = info.shield ? singleton.shieldImage : singleton.healthImage;

	auto healthGraphic = baseControl->createChild<Graphic>(healthImg);
	// This is a bit annoying as the health bar starts at the bottom, but the coord origin is
	// top-left, so fix that up a bit
	int healthBarHeight = (int)((float)healthBarSize.y * info.healthProportion);
	healthBarOffset.y = healthBarOffset.y + (healthBarSize.y - healthBarHeight);
	healthBarSize.y = healthBarHeight;
	healthGraphic->Location = healthBarOffset;
	healthGraphic->Size = healthBarSize;
	healthGraphic->ImagePosition = FillMethod::Stretch;

	sp<Graphic> stateGraphic;

	stateGraphic = baseControl->createChild<Graphic>(singleton.icons[(int)info.state]);
	stateGraphic->AutoSize = true;
	stateGraphic->Location = {0, 0};
	stateGraphic->Name = "OWNED_VEHICLE_STATE_" + info.vehicle->name;

	if (info.faded)
	{
		auto fadeIcon = baseControl->createChild<Graphic>(singleton.iconShade);
		fadeIcon->AutoSize = true;
		fadeIcon->Location = {1, 1};
	}
	if (info.passengers)
	{
		auto passengerGraphic = vehicleIcon->createChild<Graphic>(
		    singleton.vehiclePassengerCountIcons[info.passengers]);
		passengerGraphic->AutoSize = true;
		passengerGraphic->Location = {0, 0};
		passengerGraphic->Name = "OWNED_VEHICLE_PASSENGERS_" + info.vehicle->name;
	}

	return baseControl;
}

sp<Control> ControlGenerator::createVehicleControl(GameState &state, sp<Vehicle> v)
{
	auto info = createVehicleInfo(state, v);
	return createVehicleControl(state, info);
}

AgentInfo ControlGenerator::createAgentInfo(GameState &state, sp<Agent> a,
                                            UnitSelectionState forcedSelectionState, bool forceFade)
{
	AgentInfo i;

	i.agent = a;
	i.selected = UnitSelectionState::Unselected;
	i.useRank = false;
	i.state = CityUnitState::InMotion;
	i.useState = false;
	i.faded = false;
	i.fatal = false;
	i.psiIn = false;
	i.psiOut = false;
	i.stunProportion = 0.0f;
	i.healthProportion = 0.0f;
	i.shield = false;
	if (!a)
	{
		return i;
	}
	// Rank only displayed on soldiers
	if (a->type->role == AgentType::Role::Soldier)
	{
		i.useRank = true;
		i.rank = a->rank;
	}
	// Check if this is not an special screen like AEquip
	if (forcedSelectionState == UnitSelectionState::NA)
	{
		// Select according to battle state
		if (a->unit)
		{
			for (auto &u : state.current_battle->battleViewSelectedUnits)
			{
				if (u->agent == a)
				{
					i.selected = (u == state.current_battle->battleViewSelectedUnits.front())
					                 ? UnitSelectionState::FirstSelected
					                 : UnitSelectionState::Selected;
					break;
				}
			}
		}
		// Select according to city state
		else
		{
			for (auto &ag : state.current_city->cityViewSelectedAgents)
			{
				if (ag == a)
				{
					i.selected = (ag == state.current_city->cityViewSelectedAgents.front())
					                 ? UnitSelectionState::FirstSelected
					                 : UnitSelectionState::Selected;
					break;
				}
			}
		}
	}
	else // This is a special screen
	{
		i.selected = forcedSelectionState;
		if (!a->unit)
		{
			i.faded = forceFade;
		}
	}
	// Set common agent state
	i.shield = a->getMaxShield(state) > 0;
	float maxHealth;
	float currentHealth;
	if (i.shield)
	{
		currentHealth = a->getShield(state);
		maxHealth = a->getMaxShield(state);
	}
	else
	{
		currentHealth = a->getHealth();
		maxHealth = a->getMaxHealth();
	}
	i.healthProportion = maxHealth == 0.0f ? 0.0f : currentHealth / maxHealth;
	// Set state that is used in battle
	if (a->unit)
	{
		// Stunned health
		if (!i.shield)
		{
			float stunHealth = a->unit->stunDamage;
			i.stunProportion = stunHealth / maxHealth;
			i.stunProportion = clamp(i.stunProportion, 0.0f, i.healthProportion);
		}
		// Fatal wounds
		i.fatal = a->unit->isFatallyWounded();
		// Psi state
		i.psiOut = a->unit->psiTarget != nullptr;
		i.psiIn = !a->unit->psiAttackers.empty();
	}
	// Set state that is used in city
	else
	{
		// City state icon
		i.useState = true;
		auto b = a->currentBuilding;
		if (b)
		{
			if (b == a->homeBuilding)
			{
				i.state = CityUnitState::InBase;
			}
			else
			{
				i.state = CityUnitState::InBuilding;
			}
		}
		else
		{
			if (a->currentVehicle)
			{
				i.state = CityUnitState::InVehicle;
			}
			else
			{
				i.state = CityUnitState::InMotion;
			}
		}
	}

	return i;
}

sp<Control> ControlGenerator::createAgentControl(GameState &state, const AgentInfo &info)
{
	auto baseControl = mksp<Graphic>();
	fillAgentControl(state, baseControl, info);
	baseControl->Location = {0, 0};
	baseControl->Size = singleton.battleSelect[(int)info.selected]->size;
	return baseControl;
}

sp<Control> ControlGenerator::createLargeAgentControl(GameState &state, const AgentInfo &info,
                                                      bool addSkill, bool labMode)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}
	auto size = labMode ? Vec2<int>{159, 31}
	                    : Vec2<int>{130, singleton.labelFont->getFontHeight() * (addSkill ? 3 : 2)};

	auto baseControl = mksp<Control>();
	baseControl->setData(info.agent);
	baseControl->Name = "AGENT_PORTRAIT";
	baseControl->Size = size;

	auto frameGraphic = baseControl->createChild<Graphic>();
	frameGraphic->AutoSize = true;
	frameGraphic->Location = {4, 4};

	fillAgentControl(state, frameGraphic, info);

	auto nameLabel = baseControl->createChild<Label>(info.agent->name, singleton.labelFont);
	nameLabel->Location = {40, 0};
	nameLabel->Size = {labMode ? 72 : 100, labMode ? 31 : singleton.labelFont->getFontHeight() * 2};

	if (addSkill)
	{
		int skill = 0;
		if (info.agent->type->role == AgentType::Role::Physicist)
		{
			skill = info.agent->current_stats.physics_skill;
		}
		else if (info.agent->type->role == AgentType::Role::BioChemist)
		{
			skill = info.agent->current_stats.biochem_skill;
		}
		else if (info.agent->type->role == AgentType::Role::Engineer)
		{
			skill = info.agent->current_stats.engineering_skill;
		}

		auto skillLabel =
		    baseControl->createChild<Label>(format(tr("Skill %s"), skill), singleton.labelFont);
		if (labMode)
		{
			skillLabel->Size = {45, 40};
			skillLabel->Location = {115, 0};
		}
		else
		{
			skillLabel->Size = {100, singleton.labelFont->getFontHeight()};
			skillLabel->Location = {40, singleton.labelFont->getFontHeight() * 2};
		}
	}

	return baseControl;
}

sp<Control> ControlGenerator::createLargeAgentControl(GameState &state, sp<Agent> a, bool addSkill,
                                                      UnitSelectionState forcedSelectionState,
                                                      bool forceFade, bool labMode)
{
	auto info = createAgentInfo(state, a, forcedSelectionState, forceFade);
	return createLargeAgentControl(state, info, addSkill, labMode);
}

OrganisationInfo ControlGenerator::createOrganisationInfo(GameState &state, sp<Organisation> org)
{
	auto i = OrganisationInfo();
	i.organisation = org;
	i.selected = state.current_city->cityViewSelectedOrganisation == org;
	return i;
}

sp<Control> ControlGenerator::createOrganisationControl(GameState &state,
                                                        const OrganisationInfo &info)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}

	auto frame = singleton.citySelect[info.selected ? 2 : 0];
	auto baseControl = mksp<Graphic>(frame);
	baseControl->Size = frame->size;
	// FIXME: There's an extra 1 pixel here that's annoying
	baseControl->Size.x -= 1;
	baseControl->Name = "ORG_FRAME_" + info.organisation->name;
	baseControl->setData(info.organisation);

	auto vehicleIcon = baseControl->createChild<Graphic>(info.organisation->icon);
	vehicleIcon->AutoSize = true;
	vehicleIcon->Location = {1, 1};
	vehicleIcon->Name = "ORG_ICON_" + info.organisation->name;

	return baseControl;
}

sp<Control> ControlGenerator::createOrganisationControl(GameState &state, sp<Organisation> org)
{
	auto i = createOrganisationInfo(state, org);
	return createOrganisationControl(state, i);
}

sp<Control> ControlGenerator::createPurchaseControl(GameState &state,
                                                    StateRef<AEquipmentType> agentEquipmentType,
                                                    int stock)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}
	int currentStock = 0;
	int currentPrice = 0;
	if (!agentEquipmentType->bioStorage)
	{
		if (state.economy.find(agentEquipmentType.id) == state.economy.end())
		{
			return nullptr;
		}
		auto &economy = state.economy[agentEquipmentType.id];
		int week = state.gameTime.getWeek();
		if (stock == 0 && economy.currentStock == 0 &&
		    (agentEquipmentType->artifact || economy.weekAvailable == 0 ||
		     economy.weekAvailable > week))
		{
			return nullptr;
		}
		currentStock = economy.currentStock;
		currentPrice = economy.currentPrice;
	}
	else
	{
		if (stock == 0)
		{
			return nullptr;
		}
	}
	bool isAmmo = agentEquipmentType->type == AEquipmentType::Type::Ammo;
	auto iconLeft =
	    agentEquipmentType->bioStorage ? singleton.alienContainedDetain : singleton.purchaseBoxIcon;
	auto iconRight =
	    agentEquipmentType->bioStorage ? singleton.alienContainedKill : singleton.purchaseXComIcon;
	bool transfer = false;
	auto name = agentEquipmentType->name;
	auto manufacturer =
	    agentEquipmentType->bioStorage ? "" : agentEquipmentType->manufacturer->name;
	auto price = currentPrice;
	auto stock1 = stock;
	auto stock2 = currentStock;
	auto control = createTransactionControl(state, isAmmo, iconLeft, iconRight, transfer, name,
	                                        manufacturer, price, stock1, stock2);
	return control;
}

sp<Control> ControlGenerator::createPurchaseControl(GameState &state,
                                                    StateRef<VEquipmentType> vehicleEquipmentType,
                                                    int stock)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}

	if (state.economy.find(vehicleEquipmentType.id) == state.economy.end())
	{
		return nullptr;
	}
	auto &economy = state.economy[vehicleEquipmentType.id];
	int week = state.gameTime.getWeek();
	if (stock == 0 && economy.currentStock == 0 &&
	    (vehicleEquipmentType->manufacturer == state.getPlayer() || economy.weekAvailable == 0 ||
	     economy.weekAvailable > week))
	{
		return nullptr;
	}
	bool isAmmo = false;
	auto iconLeft = singleton.purchaseBoxIcon;
	auto iconRight = singleton.purchaseXComIcon;
	bool transfer = false;
	auto name = vehicleEquipmentType->name;
	auto manufacturer = vehicleEquipmentType->manufacturer->name;
	auto price = economy.currentPrice;
	auto stock1 = stock;
	auto stock2 = economy.currentStock;
	auto control = createTransactionControl(state, isAmmo, iconLeft, iconRight, transfer, name,
	                                        manufacturer, price, stock1, stock2);
	return control;
}

sp<Control> ControlGenerator::createPurchaseControl(GameState &state,
                                                    StateRef<VAmmoType> vehicleAmmoType, int stock)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}

	if (state.economy.find(vehicleAmmoType.id) == state.economy.end())
	{
		return nullptr;
	}
	auto &economy = state.economy[vehicleAmmoType.id];
	int week = state.gameTime.getWeek();
	if (stock == 0 && economy.currentStock == 0 &&
	    (vehicleAmmoType->manufacturer == state.getPlayer() || economy.weekAvailable == 0 ||
	     economy.weekAvailable > week))
	{
		return nullptr;
	}
	bool isAmmo = true;
	auto iconLeft = singleton.purchaseBoxIcon;
	auto iconRight = singleton.purchaseXComIcon;
	bool transfer = false;
	auto name = vehicleAmmoType->name;
	auto manufacturer = vehicleAmmoType->manufacturer->name;
	auto price = economy.currentPrice;
	auto stock1 = stock;
	auto stock2 = economy.currentStock;
	auto control = createTransactionControl(state, isAmmo, iconLeft, iconRight, transfer, name,
	                                        manufacturer, price, stock1, stock2);
	return control;
}

sp<Control> ControlGenerator::createPurchaseControl(GameState &state,
                                                    StateRef<VehicleType> vehicleType, int stock)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}

	if (state.economy.find(vehicleType.id) == state.economy.end())
	{
		return nullptr;
	}
	auto &economy = state.economy[vehicleType.id];
	int week = state.gameTime.getWeek();
	if (stock == 0 && economy.currentStock == 0 &&
	    (vehicleType->manufacturer == state.getPlayer() || economy.weekAvailable == 0 ||
	     economy.weekAvailable > week))
	{
		return nullptr;
	}
	bool isAmmo = false;
	auto iconLeft = singleton.purchaseBoxIcon;
	auto iconRight = singleton.purchaseXComIcon;
	bool transfer = false;
	auto name = vehicleType->name;
	auto manufacturer = vehicleType->manufacturer->name;
	auto price = economy.currentPrice;
	auto stock1 = stock;
	auto stock2 = economy.currentStock;
	auto control = createTransactionControl(state, isAmmo, iconLeft, iconRight, transfer, name,
	                                        manufacturer, price, stock1, stock2);
	return control;
}

sp<Control> ControlGenerator::createPurchaseControl(GameState &state, StateRef<Vehicle> vehicle,
                                                    int stock)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}

	if (state.economy.find(vehicle->type.id) == state.economy.end())
	{
		return nullptr;
	}
	auto &economy = state.economy[vehicle->type.id];
	bool isAmmo = false;
	auto iconLeft = singleton.purchaseBoxIcon;
	auto iconRight = singleton.purchaseXComIcon;
	bool transfer = false;
	auto name = vehicle->name;
	auto manufacturer = vehicle->type->manufacturer->name;
	auto price = economy.currentPrice;
	for (auto &e : vehicle->equipment)
	{
		if (state.economy.find(e->type.id) != state.economy.end())
		{
			price += state.economy[e->type.id].currentPrice;
			// FIXME: ADD PRICE OF AMMO
		}
	}
	auto stock1 = stock;
	auto stock2 = 0;
	auto control = createTransactionControl(state, isAmmo, iconLeft, iconRight, transfer, name,
	                                        manufacturer, price, stock1, stock2);
	return control;
}

sp<Control> ControlGenerator::createTransferControl(GameState &state,
                                                    StateRef<AEquipmentType> agentEquipmentType,
                                                    int stock1, int stock2)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}
	if (stock1 == 0 && stock2 == 0 &&
	    (agentEquipmentType->bioStorage || agentEquipmentType->artifact))
	{
		return nullptr;
	}
	bool isAmmo = agentEquipmentType->type == AEquipmentType::Type::Ammo;
	auto icon = agentEquipmentType->bioStorage ? singleton.alienContainedDetain
	                                           : singleton.purchaseXComIcon;
	bool transfer = true;
	auto name = agentEquipmentType->name;
	auto manufacturer = "";
	auto price = 0;
	auto control = createTransactionControl(state, isAmmo, icon, icon, transfer, name, manufacturer,
	                                        price, stock1, stock2);
	return control;
}

sp<Control> ControlGenerator::createTransferControl(GameState &state,
                                                    StateRef<VEquipmentType> vehicleEquipmentType,
                                                    int stock1, int stock2)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}
	if (stock1 == 0 && stock2 == 0 && vehicleEquipmentType->manufacturer == state.getPlayer())
	{
		return nullptr;
	}
	bool isAmmo = false;
	auto icon = singleton.purchaseXComIcon;
	bool transfer = true;
	auto name = vehicleEquipmentType->name;
	auto manufacturer = "";
	auto price = 0;
	auto control = createTransactionControl(state, isAmmo, icon, icon, transfer, name, manufacturer,
	                                        price, stock1, stock2);
	return control;
}

sp<Control> ControlGenerator::createTransferControl(GameState &state,
                                                    StateRef<VAmmoType> vehicleAmmoType, int stock1,
                                                    int stock2)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}
	if (stock1 == 0 && stock2 == 0 && vehicleAmmoType->manufacturer == state.getPlayer())
	{
		return nullptr;
	}
	bool isAmmo = true;
	auto icon = singleton.purchaseXComIcon;
	bool transfer = true;
	auto name = vehicleAmmoType->name;
	auto manufacturer = "";
	auto price = 0;
	auto control = createTransactionControl(state, isAmmo, icon, icon, transfer, name, manufacturer,
	                                        price, stock1, stock2);
	return control;
}

sp<Control> ControlGenerator::createTransferControl(GameState &state, StateRef<Vehicle> vehicle,
                                                    int stock1, int stock2)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}
	bool isAmmo = false;
	auto icon = singleton.purchaseXComIcon;
	bool transfer = true;
	auto name = vehicle->name;
	auto manufacturer = "";
	auto price = 0;
	auto control = createTransactionControl(state, isAmmo, icon, icon, transfer, name, manufacturer,
	                                        price, stock1, stock2);
	return control;
}

sp<Control> ControlGenerator::createTransactionControl(GameState &state, bool isAmmo,
                                                       sp<Image> iconLeft, sp<Image> iconRight,
                                                       bool transfer, UString name,
                                                       UString manufacturer, int price, int stock1,
                                                       int stock2)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}

	auto size = Vec2<int>{173 + 178 - 2, 44};

	auto baseControl = mksp<Control>();
	// baseControl->setData(info.agent);
	baseControl->Name = "TRANSACTION_CONTROL_XYZ";
	baseControl->Size = size;

	auto bgLeft = baseControl->createChild<Graphic>(singleton.purchaseControlParts[0]);
	bgLeft->AutoSize = true;
	bgLeft->Location = {0, 0};
	auto bgRight = baseControl->createChild<Graphic>(singleton.purchaseControlParts[1]);
	bgRight->AutoSize = true;
	bgRight->Location = {173 - 1, 0};

	if (isAmmo)
	{
		auto arrow = baseControl->createChild<Graphic>(singleton.purchaseArrow);
		arrow->AutoSize = true;
		arrow->Location = {4, 0};
	}

	auto iconL = baseControl->createChild<Graphic>(iconLeft);
	iconL->Name = baseControl->Name + "_ICON_LEFT";
	iconL->Size = {22, 20};
	iconL->ImageHAlign = HorizontalAlignment::Centre;
	iconL->ImageVAlign = VerticalAlignment::Centre;
	iconL->Location = {58, 22};
	auto iconR = baseControl->createChild<Graphic>(iconRight);
	iconR->Name = baseControl->Name + "_ICON_RIGHT";
	iconR->Size = {22, 20};
	iconR->ImageHAlign = HorizontalAlignment::Centre;
	iconR->ImageVAlign = VerticalAlignment::Centre;
	iconR->Location = {270, 22};

	if (name.length() > 0)
	{
		auto label = baseControl->createChild<Label>(name, singleton.labelFont);
		label->Location = {isAmmo ? 32 : 11, 1};
		label->Size = {256, 16};
		label->TextHAlign = HorizontalAlignment::Left;
		label->TextVAlign = VerticalAlignment::Centre;
	}
	if (manufacturer.length() > 0)
	{
		auto label = baseControl->createChild<Label>(manufacturer, singleton.labelFont);
		label->Location = {34, 1};
		label->Size = {256, 16};
		label->TextHAlign = HorizontalAlignment::Right;
		label->TextVAlign = VerticalAlignment::Centre;
	}
	if (price != 0)
	{
		auto label = baseControl->createChild<Label>(format("$%d", price), singleton.labelFont);
		label->Location = {290, 1};
		label->Size = {47, 16};
		label->TextHAlign = HorizontalAlignment::Right;
		label->TextVAlign = VerticalAlignment::Centre;
	}

	auto stockLeft = baseControl->createChild<Label>(format("%d", stock2), singleton.labelFont);
	stockLeft->Name = baseControl->Name + "_STOCK_LEFT";
	stockLeft->Location = {11, 24};
	stockLeft->Size = {32, 14};
	stockLeft->TextHAlign = HorizontalAlignment::Right;
	stockLeft->TextVAlign = VerticalAlignment::Centre;
	auto stockRight = baseControl->createChild<Label>(format("%d", stock1), singleton.labelFont);
	stockRight->Name = baseControl->Name + "_STOCK_RIGHT";
	stockRight->Location = {303, 24};
	stockRight->Size = {32, 14};
	stockRight->TextHAlign = HorizontalAlignment::Right;
	stockRight->TextVAlign = VerticalAlignment::Centre;

	auto changeLeft = baseControl->createChild<Label>("", singleton.labelFont);
	changeLeft->Name = baseControl->Name + "_CHANGE_LEFT";
	changeLeft->Location = {50, 24};
	changeLeft->Size = {32, 14};
	changeLeft->TextHAlign = HorizontalAlignment::Right;
	changeLeft->TextVAlign = VerticalAlignment::Centre;
	auto changeRight = baseControl->createChild<Label>("", singleton.labelFont);
	changeRight->Name = baseControl->Name + "_CHANGE_RIGHT";
	changeRight->Location = {264, 24};
	changeRight->Size = {30, 14};
	changeRight->TextHAlign = HorizontalAlignment::Right;
	changeRight->TextVAlign = VerticalAlignment::Centre;

	auto scroll = baseControl->createChild<ScrollBar>();
	scroll->Name = baseControl->Name + "_SCROLL";
	scroll->Location = {102, 24};
	scroll->Size = {147, 19};
	scroll->Minimum = 0;
	scroll->Maximum = stock1 + stock2;
	scroll->setValue(stock1);

	auto buttonScrollLeft = baseControl->createChild<GraphicButton>(nullptr, singleton.scrollLeft);
	buttonScrollLeft->Size = singleton.scrollLeft->size;
	buttonScrollLeft->Location = {87, 22};
	buttonScrollLeft->ScrollBarPrev = scroll;
	auto buttonScrollRight =
	    baseControl->createChild<GraphicButton>(nullptr, singleton.scrollRight);
	buttonScrollRight->Size = singleton.scrollRight->size;
	buttonScrollRight->Location = {247, 22};
	buttonScrollRight->ScrollBarNext = scroll;

	if (stock1 == 0 && stock2 == 0)
	{
		auto shade = baseControl->createChild<Graphic>(singleton.transactionShade);
		shade->AutoSize = true;
		shade->Location = {0, 0};
	}

	return baseControl;
}

int ControlGenerator::getFontHeight(GameState &state)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}
	return singleton.labelFont->getFontHeight();
}

sp<Control> ControlGenerator::createAgentControl(GameState &state, sp<Agent> a,
                                                 UnitSelectionState forcedSelectionState,
                                                 bool forceFade)
{
	auto info = createAgentInfo(state, a, forcedSelectionState, forceFade);
	return createAgentControl(state, info);
}

void ControlGenerator::fillAgentControl(GameState &state, sp<Graphic> baseControl,
                                        const AgentInfo &info)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}

	baseControl->Controls.clear();
	if (!info.agent)
	{
		baseControl->setImage(nullptr);
		return;
	}
	baseControl->setImage(singleton.battleSelect[(int)info.selected]);
	baseControl->setData(info.agent);

	auto unitIcon = baseControl->createChild<Graphic>(info.agent->getPortrait().icon);
	unitIcon->AutoSize = true;
	unitIcon->Location = {2, 1};

	if (info.useRank)
	{
		auto rankIcon = baseControl->createChild<Graphic>(singleton.unitRanks[(int)info.rank]);
		rankIcon->AutoSize = true;
		rankIcon->Location = {0, 0};
	}
	if (info.healthProportion > 0.0f)
	{
		// FIXME: Put these somewhere slightly less magic?
		Vec2<int> healthBarOffset = {27, 2};
		Vec2<int> healthBarSize = {3, 20};

		auto healthImg = info.shield ? singleton.shieldImage : singleton.healthImage;
		auto healthGraphic = baseControl->createChild<Graphic>(healthImg);
		// This is a bit annoying as the health bar starts at the bottom, but the coord origin is
		// top-left, so fix that up a bit
		int healthBarHeight = (int)((float)healthBarSize.y * info.healthProportion);
		healthBarOffset.y = healthBarOffset.y + (healthBarSize.y - healthBarHeight);
		healthBarSize.y = healthBarHeight;
		healthGraphic->Location = healthBarOffset;
		healthGraphic->Size = healthBarSize;
		healthGraphic->ImagePosition = FillMethod::Stretch;
	}
	if (info.stunProportion > 0.0f)
	{
		// FIXME: Put these somewhere slightly less magic?
		Vec2<int> healthBarOffset = {27, 2};
		Vec2<int> healthBarSize = {3, 20};

		auto healthImg = singleton.stunImage;
		auto healthGraphic = baseControl->createChild<Graphic>(healthImg);
		// This is a bit annoying as the health bar starts at the bottom, but the coord origin is
		// top-left, so fix that up a bit
		int healthBarHeight = (int)((float)healthBarSize.y * info.stunProportion);
		healthBarOffset.y = healthBarOffset.y + (healthBarSize.y - healthBarHeight);
		healthBarSize.y = healthBarHeight;
		healthGraphic->Location = healthBarOffset;
		healthGraphic->Size = healthBarSize;
		healthGraphic->ImagePosition = FillMethod::Stretch;
	}
	if (info.useState)
	{
		auto stateGraphic = baseControl->createChild<Graphic>(singleton.icons[(int)info.state]);
		stateGraphic->AutoSize = true;
		stateGraphic->Location = {0, 0};
	}
	if (config().getBool("OpenApoc.NewFeature.AdditionalUnitIcons"))
	{
		if (info.fatal)
		{
			auto icon = baseControl->createChild<Graphic>(singleton.iconFatal);
			icon->AutoSize = true;
			icon->Location = {0, 0};
		}
		if (info.psiIn)
		{
			auto icon = baseControl->createChild<Graphic>(singleton.iconPsiIn);
			icon->AutoSize = true;
			icon->Location = {0, 0};
		}
		if (info.psiOut)
		{
			auto icon = baseControl->createChild<Graphic>(singleton.iconPsiOut);
			icon->AutoSize = true;
			icon->Location = {0, 0};
		}
	}
	if (info.faded)
	{
		auto fadeIcon = baseControl->createChild<Graphic>(singleton.iconShade);
		fadeIcon->AutoSize = true;
		fadeIcon->Location = {2, 1};
	}
}

bool VehicleTileInfo::operator==(const VehicleTileInfo &other) const
{
	return (this->vehicle == other.vehicle && this->selected == other.selected &&
	        this->healthProportion == other.healthProportion && this->shield == other.shield &&
	        this->passengers == other.passengers && this->state == other.state);
}

bool VehicleTileInfo::operator!=(const VehicleTileInfo &other) const { return !(*this == other); }
bool AgentInfo::operator==(const AgentInfo &other) const
{
	return (this->agent == other.agent && this->rank == other.rank &&
	        this->useRank == other.useRank && this->state == other.state &&
	        this->useState == other.useState && this->fatal == other.fatal &&
	        this->psiIn == other.psiIn && this->psiOut == other.psiOut &&
	        this->selected == other.selected && this->healthProportion == other.healthProportion &&
	        this->stunProportion == other.stunProportion && this->shield == other.shield);
}

bool AgentInfo::operator!=(const AgentInfo &other) const { return !(*this == other); }
bool OrganisationInfo::operator==(const OrganisationInfo &other) const
{
	return (this->organisation == other.organisation && this->selected == other.selected);
}
bool OrganisationInfo::operator!=(const OrganisationInfo &other) const { return !(*this == other); }
}
