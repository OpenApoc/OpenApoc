#include "game/ui/components/controlgenerator.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/multilistbox.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/facility.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vequipment.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/city/facilitytype.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/shared/agent.h"

namespace OpenApoc
{
ControlGenerator ControlGenerator::singleton;

const UString ControlGenerator::VEHICLE_ICON_NAME("ICON_V");
const UString ControlGenerator::AGENT_ICON_NAME("ICON_A");
const UString ControlGenerator::LEFT_LIST_NAME("LEFT");
const UString ControlGenerator::RIGHT_LIST_NAME("RIGHT");

void ControlGenerator::init(GameState &state [[maybe_unused]])
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
		    "PCK:xcom3/ufodata/vs_icon.pck:xcom3/ufodata/vs_icon.tab:%d:xcom3/ufodata/pal_01.dat",
		    i)));
	}

	vehiclePassengerCountIcons.emplace_back();
	for (int i = 51; i <= 63; i++)
	{
		vehiclePassengerCountIcons.push_back(fw().data->loadImage(format(
		    "PCK:xcom3/ufodata/vs_icon.pck:xcom3/ufodata/vs_icon.tab:%d:xcom3/ufodata/pal_01.dat",
		    i)));
	}
	labelFont = ui().getFont("smalfont");

	initialised = true;
}

sp<Control> ControlGenerator::createVehicleIcon(GameState &state, sp<Vehicle> vehicle)
{
	auto info = createVehicleInfo(state, vehicle);
	auto icon = createVehicleControl(state, info);
	icon->Name = VEHICLE_ICON_NAME;
	icon->setData(mksp<int>(info.passengers));

	return icon;
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
	// Faded if in other dimension or if haven't left dimension gate yet
	t.faded = v->city != state.current_city || (!v->tileObject && !v->currentBuilding);

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
	baseControl->ToolTipText = info.vehicle->name;

	auto vehicleIcon = baseControl->createChild<Graphic>(info.vehicle->type->icon);
	if (vehicleIcon->getImage())
	{
		vehicleIcon->Size = vehicleIcon->getImage()->size;
	}
	else
	{
		vehicleIcon->AutoSize = true;
	}
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
	if (stateGraphic->getImage())
	{
		stateGraphic->Size = stateGraphic->getImage()->size;
	}
	else
	{
		stateGraphic->AutoSize = true;
	}
	stateGraphic->Location = {0, 0};
	stateGraphic->Name = "OWNED_VEHICLE_STATE_" + info.vehicle->name;

	if (info.faded)
	{
		auto fadeIcon = baseControl->createChild<Graphic>(singleton.iconShade);
		if (fadeIcon->getImage())
		{
			fadeIcon->Size = fadeIcon->getImage()->size;
		}
		else
		{
			fadeIcon->AutoSize = true;
		}
		fadeIcon->Location = {1, 1};
	}
	if (info.passengers)
	{
		auto passengerGraphic = vehicleIcon->createChild<Graphic>(
		    singleton.vehiclePassengerCountIcons[info.passengers]);
		if (passengerGraphic->getImage())
		{
			passengerGraphic->Size = passengerGraphic->getImage()->size;
		}
		else
		{
			passengerGraphic->AutoSize = true;
		}
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

sp<Control> ControlGenerator::createVehicleAssignmentControl(GameState &state, sp<Vehicle> vehicle)
{
	const int controlLength = 200, controlHeight = 24, iconLenght = 36;

	auto control = mksp<Control>();
	control->Size = control->SelectionSize = {controlLength, controlHeight};
	control->setData(vehicle);

	auto icon = createVehicleIcon(state, vehicle);
	icon->Size = {iconLenght, controlHeight};
	icon->setParent(control);

	auto nameLabel = control->createChild<Label>(vehicle->name, singleton.labelFont);
	nameLabel->Size = {controlLength - iconLenght, singleton.labelFont->getFontHeight()};
	nameLabel->Location = {iconLenght, (control->Size.y - nameLabel->Size.y) / 2};

	return control;
}

sp<Control> ControlGenerator::createBuildingAssignmentControl(GameState &state,
                                                              sp<Building> building)
{
	const int controlLength = 200, controlHeight = 24, iconLenght = 36;

	if (!singleton.initialised)
	{
		singleton.init(state);
	}

	auto frame = singleton.citySelect[0];
	auto control = mksp<Graphic>(frame);
	control->Size = control->SelectionSize = {controlLength, controlHeight};
	control->Name = "ORG_FRAME";
	control->setData(building);

	auto buildingIcon =
	    control->createChild<Graphic>(building->owner->icon); // TODO: set vanilla building icon
	buildingIcon->AutoSize = true;
	buildingIcon->Location = {1, 1};
	buildingIcon->Name = "ORG_ICON";

	UString name(building->name);
	for (auto b : state.player_bases)
	{
		if (b.second->building == building)
		{
			name = b.second->name;
			break;
		}
	}
	auto nameLabel = control->createChild<Label>(name, singleton.labelFont);
	nameLabel->Size = {controlLength - iconLenght, singleton.labelFont->getFontHeight()};
	nameLabel->Location = {iconLenght, (control->Size.y - nameLabel->Size.y) / 2};

	return control;
}

sp<Control> ControlGenerator::createAgentAssignmentControl(GameState &state, sp<Agent> agent)
{
	const int controlLength = 200, controlHeight = 24, iconLength = 36;

	if (!singleton.initialised)
	{
		singleton.init(state);
	}

	auto control = mksp<Control>();
	control->setData(agent);
	control->Size = control->SelectionSize = {controlLength, controlHeight};
	control->Name = "AGENT_PORTRAIT";

	auto icon = createAgentIcon(state, agent, UnitSelectionState::Unselected, false);
	icon->Size = {iconLength, controlHeight};
	icon->setParent(control);

	auto nameLabel = control->createChild<Label>(agent->name, singleton.labelFont);
	nameLabel->Size = {controlLength - iconLength, singleton.labelFont->getFontHeight()};
	nameLabel->Location = {iconLength, (control->Size.y - nameLabel->Size.y) / 2};

	return control;
}

sp<Control> ControlGenerator::createAgentIcon(GameState &state, sp<Agent> agent,
                                              UnitSelectionState forcedSelectionState,
                                              bool forceFade)
{
	auto info = createAgentInfo(state, agent, forcedSelectionState, forceFade);
	auto icon = mksp<Graphic>();
	icon->AutoSize = true;
	icon->Name = AGENT_ICON_NAME;
	fillAgentControl(state, icon, info);
	icon->setData(mksp<CityUnitState>(info.state));

	return icon;
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
		i.state = getCityUnitState(a);
	}

	return i;
}

CityUnitState ControlGenerator::getCityUnitState(sp<Agent> agent)
{
	auto building = agent->currentBuilding;
	if (building)
	{
		if (building == agent->homeBuilding)
		{
			return CityUnitState::InBase;
		}
		else
		{
			return CityUnitState::InBuilding;
		}
	}
	else
	{
		if (agent->currentVehicle)
		{
			return CityUnitState::InVehicle;
		}
		else
		{
			return CityUnitState::InMotion;
		}
	}
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
                                                      int width, UnitSkillState skill)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}

	auto baseControl = mksp<Control>();
	baseControl->setData(info.agent);
	baseControl->Name = "AGENT_PORTRAIT";
	baseControl->Size = {width, singleton.labelFont->getFontHeight() * 2};
	if (skill == UnitSkillState::Vertical)
		baseControl->Size.y = singleton.labelFont->getFontHeight() * 3;

	auto frameGraphic = baseControl->createChild<Graphic>();
	frameGraphic->AutoSize = true;
	frameGraphic->Location = {4, 3};

	fillAgentControl(state, frameGraphic, info);

	auto nameLabel = baseControl->createChild<Label>(info.agent->name, singleton.labelFont);
	nameLabel->Location = {40, 0};
	nameLabel->Size = {baseControl->Size.x - 40, singleton.labelFont->getFontHeight() * 2};

	if (skill != UnitSkillState::Hidden)
	{
		auto skillLabel = baseControl->createChild<Label>(
		    format(tr("Skill %d"), info.agent->getSkill()), singleton.labelFont);
		skillLabel->Tint = {192, 192, 192};

		skillLabel->Size = {nameLabel->Size.x, singleton.labelFont->getFontHeight()};
		skillLabel->Location = {40, singleton.labelFont->getFontHeight() * 2};

		if (skill == UnitSkillState::Horizontal)
		{
			skillLabel->Size.x = 45;
			nameLabel->Size.x -= 50;
			skillLabel->Location = {baseControl->Size.x - 45, 0};
		}
	}

	return baseControl;
}

sp<Control> ControlGenerator::createLargeAgentControl(GameState &state, sp<Agent> a, int width,
                                                      UnitSkillState skill,
                                                      UnitSelectionState forcedSelectionState,
                                                      bool forceFade)
{
	auto info = createAgentInfo(state, a, forcedSelectionState, forceFade);
	return createLargeAgentControl(state, info, width, skill);
}

/**
 * Create lab icon control with quantity label.
 * @state - the game state
 * @facility - lab facility
 * @return - lab icon control
 */
sp<Control> ControlGenerator::createLabControl(sp<GameState> state, sp<Facility> facility)
{
	if (!singleton.initialised)
	{
		singleton.init(*state);
	}

	auto graphic = mksp<Graphic>(facility->type->sprite);
	graphic->AutoSize = true;

	auto spriteSize = facility->type->sprite->size;
	auto label = graphic->createChild<Label>();
	label->setFont(singleton.labelFont);
	label->Size = {20, label->getFont()->getFontHeight()};
	label->Location = {(spriteSize[0] - label->Size[0]) / 2, (spriteSize[1] - label->Size[1]) / 2};
	label->TextHAlign = HorizontalAlignment::Centre;

	return graphic;
}

/**
 * Control containing two MultilistBox for assignment state.
 * @controlLength - length of the control
 */
sp<Control> ControlGenerator::createDoubleListControl(const int controlLength)
{
	auto rubberItem = mksp<Control>();
	rubberItem->Size = Vec2<int>{controlLength, 1};
	rubberItem->setFuncPreRender([](sp<Control> control) {
		int sizeY = 1;
		for (auto &c : control->Controls)
		{
			if (!c->isVisible())
				continue;
			sizeY = std::max(sizeY, c->Location.y + c->Size.y);
		}
		control->Size.y = sizeY;
	});

	auto leftList = rubberItem->createChild<MultilistBox>();
	leftList->Name = LEFT_LIST_NAME;

	auto rightList = rubberItem->createChild<MultilistBox>();
	rightList->Location = Vec2<int>{controlLength / 2, 0};
	rightList->Name = RIGHT_LIST_NAME;

	return rubberItem;
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
	baseControl->ToolTipText = tr(info.organisation->name);
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
	baseControl->ToolTipText = info.agent->name;

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
	        this->passengers == other.passengers && this->state == other.state &&
	        this->faded == other.faded);
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
} // namespace OpenApoc
