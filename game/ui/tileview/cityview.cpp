#include "game/ui/tileview/cityview.h"
#include "forms/checkbox.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/radiobutton.h"
#include "forms/scrollbar.h"
#include "forms/ticker.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/keycodes.h"
#include "framework/palette.h"
#include "framework/renderer.h"
#include "framework/sound.h"
#include "framework/trace.h"
#include "game/state/battle/battle.h"
#include "game/state/city/agentmission.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/facility.h"
#include "game/state/city/research.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/message.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/battlemap.h"
#include "game/state/rules/city/baselayout.h"
#include "game/state/rules/city/citycommonsamplelist.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/rules/city/ufopaedia.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/shared/agent.h"
#include "game/state/shared/organisation.h"
#include "game/state/shared/projectile.h"
#include "game/state/tilemap/collision.h"
#include "game/state/tilemap/tileobject_projectile.h"
#include "game/state/tilemap/tileobject_scenery.h"
#include "game/state/tilemap/tileobject_vehicle.h"
#include "game/ui/base/basescreen.h"
#include "game/ui/base/researchscreen.h"
#include "game/ui/base/vequipscreen.h"
#include "game/ui/battle/battlebriefing.h"
#include "game/ui/city/alertscreen.h"
#include "game/ui/city/basebuyscreen.h"
#include "game/ui/city/basedefensescreen.h"
#include "game/ui/city/baseselectscreen.h"
#include "game/ui/city/buildingscreen.h"
#include "game/ui/city/infiltrationscreen.h"
#include "game/ui/city/scorescreen.h"
#include "game/ui/components/basegraphics.h"
#include "game/ui/components/controlgenerator.h"
#include "game/ui/components/locationscreen.h"
#include "game/ui/general/aequipscreen.h"
#include "game/ui/general/ingameoptions.h"
#include "game/ui/general/messagebox.h"
#include "game/ui/general/messagelogscreen.h"
#include "game/ui/general/notificationscreen.h"
#include "game/ui/ufopaedia/ufopaediacategoryview.h"
#include "game/ui/ufopaedia/ufopaediaview.h"
#include "library/sp.h"
#include "library/strings_format.h"
#include <glm/glm.hpp>

// Uncomment to start with paused
#define DEBUG_START_PAUSE

namespace OpenApoc
{
namespace
{

static const std::vector<UString> TAB_FORM_NAMES = {
    "city/tab1", "city/tab2", "city/tab3", "city/tab4",
    "city/tab5", "city/tab6", "city/tab7", "city/tab8",
};

std::shared_future<void> loadBattleVehicle(sp<GameState> state, StateRef<Vehicle> ufo,
                                           StateRef<Vehicle> playerVehicle)
{

	auto loadTask = fw().threadPoolEnqueue([state, ufo, playerVehicle]() -> void {
		std::list<StateRef<Agent>> agents;
		for (auto &a : playerVehicle->currentAgents)
		{
			if (a->type->role == AgentType::Role::Soldier)
			{
				agents.push_back(a);
			}
		}

		StateRef<Organisation> org = ufo->owner;
		bool hotseat = false;
		const std::map<StateRef<AgentType>, int> *aliens = nullptr;
		Battle::beginBattle(*state, hotseat, org, agents, aliens, playerVehicle, ufo);
	});

	return loadTask;
}
} // anonymous namespace

bool CityView::handleClickedBuilding(StateRef<Building> building, bool rightClick,
                                     CitySelectionState selState)
{
	if (vanillaControls)
	{
		switch (selState)
		{
			case CitySelectionState::Normal:
			{
				// Location (building) screen
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH, mksp<BuildingScreen>(this->state, building)});
				return true;
			}
			case CitySelectionState::AttackBuilding:
			{
				orderAttack(building);
				setSelectionState(CitySelectionState::Normal);
				return true;
			}
			case CitySelectionState::GotoBuilding:
			{
				orderMove(building, modifierRCtrl || modifierLCtrl);
				setSelectionState(CitySelectionState::Normal);
				return true;
			}
			case CitySelectionState::AttackVehicle:
			case CitySelectionState::GotoLocation:
			case CitySelectionState::ManualControl:
				break;
		}
		return false;
	}

	// [Alt] + [Shift] gives bulding orders (left = attack, right = move)
	if ((modifierLShift || modifierRShift) && (modifierLAlt || modifierRAlt))
	{
		if (rightClick)
		{
			orderMove(building, modifierRCtrl || modifierLCtrl);
		}
		else
		{
			orderAttack(building);
		}
		return true;
	}
	// [Shift] pass through to a scenery click
	if (modifierLShift || modifierRShift)
	{
		return false;
	}
	// [Alt] opens ufopaedia screens
	if (modifierLAlt || modifierRAlt)
	{
		StateRef<UfopaediaEntry> ufopaediaEntry;
		if (rightClick)
		{
			ufopaediaEntry = building->owner->ufopaedia_entry;
		}
		else
		{
			ufopaediaEntry = building->function->ufopaedia_entry;
		}
		tryOpenUfopaediaEntry(ufopaediaEntry);
		return true;
	}
	// No modifiers
	switch (selState)
	{
		case CitySelectionState::Normal:
		{
			if (rightClick)
			{
				// Location (building) screen
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH, mksp<BuildingScreen>(this->state, building)});
			}
			else
			{
				// Base / buy screen
				if (building->base)
				{
					// Base screen
					state->current_base = building->base;
					this->uiTabs[0]
					    ->findControlTyped<Label>("TEXT_BASE_NAME")
					    ->setText(this->state->current_base->name);
					fw().stageQueueCommand(
					    {StageCmd::Command::PUSH, mksp<BaseScreen>(this->state)});
				}
				else if (building->base_layout && building->owner == state->getGovernment())
				{
					// Base buy screen
					fw().stageQueueCommand(
					    {StageCmd::Command::PUSH, mksp<BaseBuyScreen>(state, building)});
				}
			}
			return true;
		}
		case CitySelectionState::AttackBuilding:
		{
			orderAttack(building);
			setSelectionState(CitySelectionState::Normal);
			return true;
		}
		case CitySelectionState::GotoBuilding:
		{
			orderMove(building, modifierRCtrl || modifierLCtrl);
			setSelectionState(CitySelectionState::Normal);
			return true;
		}
		case CitySelectionState::AttackVehicle:
		case CitySelectionState::GotoLocation:
		case CitySelectionState::ManualControl:
			break;
	}
	return false;
}

bool CityView::handleClickedVehicle(StateRef<Vehicle> vehicle, bool rightClick,
                                    CitySelectionState selState)
{
	if (vanillaControls)
	{
		switch (selState)
		{
			case CitySelectionState::Normal:
			{
				orderSelect(vehicle, rightClick, modifierLCtrl || modifierRCtrl);
				break;
			}
			case CitySelectionState::AttackVehicle:
			{
				orderAttack(vehicle, modifierLCtrl || modifierRCtrl);
				setSelectionState(CitySelectionState::Normal);
				break;
			}
		}
		return true;
	}

	// [Alt] + [Shift] gives bulding orders (therefore do nothing)
	if ((modifierLShift || modifierRShift) && (modifierLAlt || modifierRAlt))
	{
		return false;
	}
	// [Shift] gives attack order on left click and move on right click
	if (modifierLShift || modifierRShift)
	{
		if (rightClick)
		{
			orderFollow(vehicle);
		}
		else
		{
			orderAttack(vehicle, modifierLCtrl || modifierRCtrl);
		}
		return true;
	}
	// [Alt] opens info screens
	if (modifierLAlt || modifierRAlt)
	{
		StateRef<UfopaediaEntry> ufopaediaEntry;
		if (rightClick)
		{
			ufopaediaEntry = vehicle->owner->ufopaedia_entry;
		}
		else
		{
			ufopaediaEntry = vehicle->type->ufopaedia_entry;
		}
		tryOpenUfopaediaEntry(ufopaediaEntry);
		return true;
	}
	switch (selState)
	{
		case CitySelectionState::ManualControl:
		{
			if (rightClick)
			{
				orderFollow(vehicle);
			}
			else
			{
				orderFire(vehicle->position);
			}
			return true;
		}
		case CitySelectionState::Normal:
		{
			orderSelect(vehicle, rightClick, modifierLCtrl || modifierRCtrl);
			return true;
		}
		case CitySelectionState::AttackVehicle:
		{
			orderAttack(vehicle, modifierLCtrl || modifierRCtrl);
			setSelectionState(CitySelectionState::Normal);
			return true;
		}
	}
	return false;
}

bool CityView::handleClickedAgent(StateRef<Agent> agent, bool rightClick,
                                  CitySelectionState selState)
{
	orderSelect(agent, rightClick, modifierLCtrl || modifierRCtrl);
	return true;
}

bool CityView::handleClickedProjectile(sp<Projectile> projectile, bool rightClick,
                                       CitySelectionState selState)
{
	if (vanillaControls)
	{
		return false;
	}
	if (modifierLAlt || modifierRAlt)
	{
		if (rightClick)
		{
			tryOpenUfopaediaEntry(projectile->firerVehicle->owner->ufopaedia_entry);
			return true;
		}
	}
	switch (selState)
	{
		case CitySelectionState::ManualControl:
		{
			if (rightClick)
			{
				orderMove(projectile->position, modifierRCtrl || modifierLCtrl);
			}
			else
			{
				orderFire(projectile->position);
			}
			return true;
		}
		case CitySelectionState::AttackBuilding:
		case CitySelectionState::AttackVehicle:
		case CitySelectionState::GotoBuilding:
		case CitySelectionState::GotoLocation:
		case CitySelectionState::Normal:
			break;
	}
	return false;
}

bool CityView::handleClickedOrganisation(StateRef<Organisation> organisation, bool rightClick,
                                         CitySelectionState selState)
{
	if (rightClick)
	{
		tryOpenUfopaediaEntry(organisation->ufopaedia_entry);
		return true;
	}
	else
	{
		if (state->current_city->cityViewSelectedOrganisation == organisation)
		{
			if (++organisation->lastClickedBuilding >= organisation->buildings.size())
			{
				organisation->lastClickedBuilding = 0;
			}
			this->setScreenCenterTile(
			    organisation->buildings[organisation->lastClickedBuilding]->crewQuarters);
		}
		else
		{
			state->current_city->cityViewSelectedOrganisation = organisation;
		}
	}
	return true;
}

void CityView::tryOpenUfopaediaEntry(StateRef<UfopaediaEntry> ufopaediaEntry)
{
	if (ufopaediaEntry && ufopaediaEntry->dependency.satisfied())
	{
		sp<UfopaediaCategory> ufopaedia_category;
		for (auto &cat : this->state->ufopaedia)
		{
			for (auto &entry : cat.second->entries)
			{
				if (ufopaediaEntry == entry.second)
				{
					ufopaedia_category = cat.second;
					break;
				}
			}
			if (ufopaedia_category)
				break;
		}
		if (!ufopaedia_category)
		{
			LogError("No UFOPaedia category found for entry %s", ufopaediaEntry->title);
		}
		fw().stageQueueCommand(
		    {StageCmd::Command::PUSH,
		     mksp<UfopaediaCategoryView>(state, ufopaedia_category, ufopaediaEntry)});
	}
}

void CityView::orderGoToBase()
{
	if (activeTab == uiTabs[1])
	{
		for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		{
			if (v && v->owner == this->state->getPlayer())
			{
				LogInfo("Goto base for vehicle \"%s\"", v->name);
				auto bld = v->homeBuilding;
				if (!bld)
				{
					LogError("Vehicle \"%s\" has no building", v->name);
				}
				LogInfo("Vehicle \"%s\" goto building \"%s\"", v->name, bld->name);
				// FIXME: Don't clear missions if not replacing current mission
				v->setMission(*this->state, VehicleMission::gotoBuilding(*this->state, *v, bld));
			}
		}
		return;
	}
	if (activeTab == uiTabs[2])
	{
		for (auto &a : this->state->current_city->cityViewSelectedAgents)
		{
			LogInfo("Goto base for vehicle \"%s\"", a->name);
			auto bld = a->homeBuilding;
			if (!bld)
			{
				LogError("Vehicle \"%s\" has no building", a->name);
			}
			LogInfo("Vehicle \"%s\" goto building \"%s\"", a->name, bld->name);
			// FIXME: Don't clear missions if not replacing current mission
			a->setMission(*this->state, AgentMission::gotoBuilding(*this->state, *a, bld));
		}
		return;
	}
}

void CityView::orderMove(Vec3<float> position, bool alternative)
{
	bool useTeleporter =
	    alternative && config().getBool("OpenApoc.NewFeature.AllowManualCityTeleporters");
	if (activeTab == uiTabs[1])
	{
		state->current_city->groupMove(*state, state->current_city->cityViewSelectedVehicles,
		                               position, useTeleporter);
		return;
	}
}

void CityView::orderMove(StateRef<Building> building, bool alternative)
{
	bool useTeleporter =
	    alternative && config().getBool("OpenApoc.NewFeature.AllowManualCityTeleporters");
	if (activeTab == uiTabs[1])
	{
		for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		{
			if (v && v->owner == this->state->getPlayer())
			{
				LogInfo("Vehicle \"%s\" goto building \"%s\"", v->name, building->name);
				// FIXME: Don't clear missions if not replacing current mission
				v->setMission(*state,
				              VehicleMission::gotoBuilding(*state, *v, building, useTeleporter));
			}
		}
		return;
	}
	if (activeTab == uiTabs[2])
	{
		bool useTaxi = alternative && config().getBool("OpenApoc.NewFeature.AllowSoldierTaxiUse");
		for (auto &a : this->state->current_city->cityViewSelectedAgents)
		{
			if (a->type->role != AgentType::Role::Soldier)
			{
				continue;
			}
			LogInfo("Agent \"%s\" goto building \"%s\"", a->name, building->name);
			// FIXME: Don't clear missions if not replacing current mission
			a->setMission(*state,
			              AgentMission::gotoBuilding(*state, *a, building, useTeleporter, useTaxi));
		}
	}
}

void CityView::orderSelect(StateRef<Vehicle> vehicle, bool inverse, bool additive)
{
	auto pos = std::find(state->current_city->cityViewSelectedVehicles.begin(),
	                     state->current_city->cityViewSelectedVehicles.end(), vehicle);
	if (inverse)
	{
		// Vehicle in selection => remove
		if (pos != state->current_city->cityViewSelectedVehicles.end())
		{
			state->current_city->cityViewSelectedVehicles.erase(pos);
		}
	}
	else
	{
		// Vehicle not selected
		if (pos == state->current_city->cityViewSelectedVehicles.end())
		{
			// Selecting non-owned vehicles is always additive to current selection
			if (additive || vehicle->owner != state->getPlayer())
			{
				// Whenever adding clear any non-player vehicles from selection
				if (!state->current_city->cityViewSelectedVehicles.empty() &&
				    state->current_city->cityViewSelectedVehicles.front()->owner !=
				        state->getPlayer())
				{
					state->current_city->cityViewSelectedVehicles.pop_front();
				}
				state->current_city->cityViewSelectedVehicles.push_front(vehicle);
			}
			else
			{
				// Vehicle not in selection => replace selection with vehicle
				state->current_city->cityViewSelectedVehicles.clear();
				state->current_city->cityViewSelectedVehicles.push_back(vehicle);
			}
		}
		// Vehicle is selected
		else
		{
			// First move vehicle to front
			state->current_city->cityViewSelectedVehicles.erase(pos);
			// If moving vehicle to front, deselect any non-owned vehicle, unless it's that one
			if (!state->current_city->cityViewSelectedVehicles.empty() &&
			    state->current_city->cityViewSelectedVehicles.front()->owner != state->getPlayer())
			{
				state->current_city->cityViewSelectedVehicles.pop_front();
			}
			state->current_city->cityViewSelectedVehicles.push_front(vehicle);
			// Then if not additive then zoom to vehicle
			if (!additive)
			{
				this->setScreenCenterTile(vehicle->position);
				if (vehicle->owner == state->getPlayer())
				{
					activeTab = uiTabs[1];
				}
				else
				{
					activeTab = uiTabs[6];
				}
			}
		}
	}
	if (state->current_city->cityViewSelectedVehicles.empty() ||
	    state->current_city->cityViewSelectedVehicles.front()->owner != state->getPlayer())
	{
		return;
	}
	vehicle = state->current_city->cityViewSelectedVehicles.front();
	if (vehicle->owner != state->getPlayer())
	{
		vehicle = *++state->current_city->cityViewSelectedVehicles.begin();
	}
	auto vehicleForm = this->uiTabs[1];
	// FIXME: Proper multiselect handle for vehicle controls
	LogWarning("FIX: Proper multiselect handle for vehicle controls");
	switch (vehicle->altitude)
	{
		case Vehicle::Altitude::Highest:
			vehicleForm->findControlTyped<RadioButton>("BUTTON_ALTITUDE_HIGHEST")->setChecked(true);
			break;
		case Vehicle::Altitude::High:
			vehicleForm->findControlTyped<RadioButton>("BUTTON_ALTITUDE_HIGH")->setChecked(true);
			break;
		case Vehicle::Altitude::Standard:
			vehicleForm->findControlTyped<RadioButton>("BUTTON_ALTITUDE_STANDARD")
			    ->setChecked(true);
			break;
		case Vehicle::Altitude::Low:
			vehicleForm->findControlTyped<RadioButton>("BUTTON_ALTITUDE_LOW")->setChecked(true);
			break;
	}

	switch (vehicle->attackMode)
	{
		case Vehicle::AttackMode::Aggressive:
			vehicleForm->findControlTyped<RadioButton>("BUTTON_ATTACK_MODE_AGGRESSIVE")
			    ->setChecked(true);
			break;
		case Vehicle::AttackMode::Standard:
			vehicleForm->findControlTyped<RadioButton>("BUTTON_ATTACK_MODE_STANDARD")
			    ->setChecked(true);
			break;
		case Vehicle::AttackMode::Defensive:
			vehicleForm->findControlTyped<RadioButton>("BUTTON_ATTACK_MODE_DEFENSIVE")
			    ->setChecked(true);
			break;
		case Vehicle::AttackMode::Evasive:
			vehicleForm->findControlTyped<RadioButton>("BUTTON_ATTACK_MODE_EVASIVE")
			    ->setChecked(true);
			break;
	}
}

void CityView::orderSelect(StateRef<Agent> agent, bool inverse, bool additive)
{
	auto pos = std::find(state->current_city->cityViewSelectedAgents.begin(),
	                     state->current_city->cityViewSelectedAgents.end(), agent);
	if (inverse)
	{
		// Agent in selection => remove
		if (pos != state->current_city->cityViewSelectedAgents.end())
		{
			state->current_city->cityViewSelectedAgents.erase(pos);
		}
	}
	else
	{
		// Agent not selected
		if (pos == state->current_city->cityViewSelectedAgents.end())
		{
			// If additive add
			if (additive)
			{
				state->current_city->cityViewSelectedAgents.push_front(agent);
			}
			else
			{
				// Agent not in selection => replace selection with agent
				state->current_city->cityViewSelectedAgents.clear();
				state->current_city->cityViewSelectedAgents.push_back(agent);
			}
		}
		// Agent is selected
		else
		{
			// First move vehicle to front
			state->current_city->cityViewSelectedAgents.erase(pos);
			state->current_city->cityViewSelectedAgents.push_front(agent);
			// Then if not additive then zoom to agent
			if (!additive)
			{
				if (agent->currentVehicle)
				{
					this->setScreenCenterTile(agent->currentVehicle->position);
				}
				else
				{
					this->setScreenCenterTile(agent->position);
				}
			}
		}
	}

	if (state->current_city->cityViewSelectedAgents.empty())
	{
		return;
	}
	agent = state->current_city->cityViewSelectedAgents.front();
	auto agentForm = this->uiTabs[2];

	// Set form stuff for agent
	// FIXME: Implement agent select controls for psi and phys train
	LogWarning("FIX Implement agent select controls for psi and phys train");
}

void CityView::orderFire(Vec3<float> position)
{
	if (activeTab == uiTabs[1])
	{
		for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		{
			if (v && v->owner == this->state->getPlayer())
			{
				v->setManualFirePosition(position);
			}
		}
		return;
	}
}

void CityView::orderAttack(StateRef<Vehicle> vehicle, bool forced)
{
	if (activeTab == uiTabs[1])
	{
		for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		{
			if (v && v->owner == this->state->getPlayer() && v != vehicle)
			{
				if (forced || (!vehicle->crashed && !vehicle->falling && !vehicle->sliding))
				{
					if (vehicle->owner == state->getPlayer() &&
					    !config().getBool("OpenApoc.NewFeature.AllowAttackingOwnedVehicles"))
					{
						v->setMission(*state,
						              VehicleMission::followVehicle(*this->state, *v, vehicle));
					}
					else
					{
						v->setMission(*state,
						              VehicleMission::attackVehicle(*this->state, *v, vehicle));
					}
				}
				else
				{
					bool foundSoldier = vehicle->owner == state->getPlayer() ||
					                    state->getPlayer()->isRelatedTo(vehicle->owner) !=
					                        Organisation::Relation::Hostile;
					for (auto &a : v->currentAgents)
					{
						if (a->type->role == AgentType::Role::Soldier)
						{
							foundSoldier = true;
							break;
						}
					}
					if (vehicle->owner != state->getAliens() && !v->type->canRescueCrashed)
					{
						foundSoldier = false;
					}
					if (foundSoldier)
					{
						v->setMission(*state,
						              VehicleMission::recoverVehicle(*this->state, *v, vehicle));
					}
				}
			}
		}
		return;
	}
}

void CityView::orderFollow(StateRef<Vehicle> vehicle)
{
	if (activeTab == uiTabs[1])
	{
		for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		{
			if (v && v->owner == this->state->getPlayer() && v != vehicle)
			{
				v->setMission(*state, VehicleMission::followVehicle(*this->state, *v, vehicle));
			}
		}
		return;
	}
}

void CityView::orderAttack(StateRef<Building> building)
{
	if (activeTab == uiTabs[1])
	{
		for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		{
			if (v && v->owner == this->state->getPlayer())
			{
				v->setMission(*state, VehicleMission::attackBuilding(*this->state, *v, building));
			}
		}
	}
}

CityView::CityView(sp<GameState> state)
    : CityTileView(*state->current_city->map, Vec3<int>{TILE_X_CITY, TILE_Y_CITY, TILE_Z_CITY},
                   Vec2<int>{STRAT_TILE_X, STRAT_TILE_Y}, TileViewMode::Isometric,
                   state->current_city->cityViewScreenCenter, *state),
      baseForm(ui().getForm("city/city")), overlayTab(ui().getForm("city/overlay")),
      updateSpeed(CityUpdateSpeed::Speed1), lastSpeed(CityUpdateSpeed::Pause), state(state),
      followVehicle(false), selectionState(CitySelectionState::Normal),
      day_palette(fw().data->loadPalette("xcom3/ufodata/pal_01.dat")),
      twilight_palette(fw().data->loadPalette("xcom3/ufodata/pal_02.dat")),
      night_palette(fw().data->loadPalette("xcom3/ufodata/pal_03.dat"))
{
	std::vector<sp<Palette>> newPal;
	newPal.resize(3);
	for (int j = 0; j <= 15; j++)
	{
		colorCurrent = j;
		newPal[0] = mksp<Palette>();
		newPal[1] = mksp<Palette>();
		newPal[2] = mksp<Palette>();

		for (int i = 0; i < 255 - 4; i++)
		{
			newPal[0]->setColour(i, day_palette->getColour(i));
			newPal[1]->setColour(i, twilight_palette->getColour(i));
			newPal[2]->setColour(i, night_palette->getColour(i));
		}
		for (int i = 0; i < 3; i++)
		{
			// Yellow color, for owned indicators, pulsates from (3/8r 3/8g 0b) to (8/8r 8/8g 0b)
			newPal[i]->setColour(255 - 3, Colour((colorCurrent * 16 * 5 + 255 * 3) / 8,
			                                     (colorCurrent * 16 * 5 + 255 * 3) / 8, 0));
			// Red color, for enemy indicators, pulsates from (3/8r 0g 0b) to (8/8r 0g 0b)
			newPal[i]->setColour(255 - 2, Colour((colorCurrent * 16 * 5 + 255 * 3) / 8, 0, 0));
			// Pink color, for neutral indicators, pulsates from (3/8r 0g 3/8b) to (8/8r 0g 8/8b)
			newPal[i]->setColour(255 - 1, Colour((colorCurrent * 16 * 5 + 255 * 3) / 8, 0,
			                                     (colorCurrent * 16 * 5 + 255 * 3) / 8));
			// Blue color, for misc. indicators, pulsates from (0r 3/8g 3/8b) to (0r 8/8g 8/8b)
			newPal[i]->setColour(255 - 0, Colour(0, (colorCurrent * 16 * 5 + 255 * 3) / 8,
			                                     (colorCurrent * 16 * 5 + 255 * 3) / 8));
		}

		mod_day_palette.push_back(newPal[0]);
		mod_twilight_palette.push_back(newPal[1]);
		mod_night_palette.push_back(newPal[2]);
	}

	overlayTab->setVisible(false);
	overlayTab->findControlTyped<GraphicButton>("BUTTON_CLOSE")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { setSelectionState(CitySelectionState::Normal); });
	baseForm->findControlTyped<RadioButton>("BUTTON_SPEED1")->setChecked(true);
	for (auto &formName : TAB_FORM_NAMES)
	{
		sp<Form> f(ui().getForm(formName));
		if (!f)
		{
			LogError("Failed to load form \"%s\"", formName);
			return;
		}
		f->takesFocus = false;
		this->uiTabs.push_back(f);
	}
	this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];

	// Refresh base views
	resume();

	if (state->newGame)
	{
		auto bld = state->current_base->building;
		if (!bld)
		{
			LogError("Base with invalid bld");
		}
		auto bldBounds = bld->bounds;

		Vec2<int> buildingCenter = (bldBounds.p0 + bldBounds.p1) / 2;
		this->setScreenCenterTile(buildingCenter);
	}

	this->baseForm->findControl("BUTTON_TAB_1")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_city->cityViewPageIndex = 0;
		    this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];
		});
	this->baseForm->findControl("BUTTON_TAB_2")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_city->cityViewPageIndex = 1;
		    this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];
		});
	this->baseForm->findControl("BUTTON_TAB_3")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_city->cityViewPageIndex = 2;
		    this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];
		});
	this->baseForm->findControl("BUTTON_TAB_4")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_city->cityViewPageIndex = 3;
		    this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];
		});
	this->baseForm->findControl("BUTTON_TAB_5")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_city->cityViewPageIndex = 4;
		    this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];
		});
	this->baseForm->findControl("BUTTON_TAB_6")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_city->cityViewPageIndex = 5;
		    this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];
		});
	this->baseForm->findControl("BUTTON_TAB_7")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_city->cityViewPageIndex = 6;
		    this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];
		});
	this->baseForm->findControl("BUTTON_TAB_8")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_city->cityViewPageIndex = 7;
		    this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];
		});
	this->baseForm->findControl("BUTTON_FOLLOW_VEHICLE")
	    ->addCallback(FormEventType::CheckBoxChange, [this](FormsEvent *e) {
		    this->followVehicle =
		        std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy)->isChecked();
		});
	this->baseForm->findControl("BUTTON_TOGGLE_STRATMAP")
	    ->addCallback(FormEventType::CheckBoxChange, [this](FormsEvent *e) {
		    bool strategy = std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy)->isChecked();
		    this->setViewMode(strategy ? TileViewMode::Strategy : TileViewMode::Isometric);
		});
	this->baseForm->findControl("BUTTON_SPEED0")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = CityUpdateSpeed::Pause; });
	this->baseForm->findControl("BUTTON_SPEED1")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = CityUpdateSpeed::Speed1; });
	this->baseForm->findControl("BUTTON_SPEED2")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = CityUpdateSpeed::Speed2; });
	this->baseForm->findControl("BUTTON_SPEED3")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = CityUpdateSpeed::Speed3; });
	this->baseForm->findControl("BUTTON_SPEED4")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = CityUpdateSpeed::Speed4; });
	this->baseForm->findControl("BUTTON_SPEED5")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = CityUpdateSpeed::Speed5; });
	this->baseForm->findControl("BUTTON_SHOW_ALIEN_INFILTRATION")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH, mksp<InfiltrationScreen>(this->state)});
		});
	this->baseForm->findControl("BUTTON_SHOW_SCORE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<ScoreScreen>(this->state)});
		});
	this->baseForm->findControl("BUTTON_SHOW_UFOPAEDIA")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<UfopaediaView>(this->state)});
		});
	this->baseForm->findControl("BUTTON_SHOW_OPTIONS")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<InGameOptions>(this->state)});
		});
	this->baseForm->findControl("BUTTON_SHOW_LOG")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH, mksp<MessageLogScreen>(this->state, *this)});
		});
	this->baseForm->findControl("BUTTON_ZOOM_EVENT")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    if (baseForm->findControlTyped<Ticker>("NEWS_TICKER")->hasMessages())
		    {
			    this->zoomLastEvent();
		    }
		});

	auto baseManagementForm = this->uiTabs[0];
	baseManagementForm->findControl("BUTTON_SHOW_BASE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<BaseScreen>(this->state)});
		});
	baseManagementForm->findControl("BUTTON_BUILD_BASE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH, mksp<BaseSelectScreen>(this->state, this->centerPos)});
		});
	auto vehicleForm = this->uiTabs[1];
	vehicleForm->findControl("BUTTON_EQUIP_VEHICLE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    auto equipScreen = mksp<VEquipScreen>(this->state);
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    equipScreen->setSelectedVehicle(v);
				    break;
			    }
		    }
		    fw().stageQueueCommand({StageCmd::Command::PUSH, equipScreen});
		});
	vehicleForm->findControl("BUTTON_VEHICLE_BUILDING")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    fw().stageQueueCommand(
				        {StageCmd::Command::PUSH, mksp<LocationScreen>(this->state, v)});
				    break;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_GOTO_BUILDING")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    setSelectionState(CitySelectionState::GotoBuilding);
				    break;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_GOTO_LOCATION")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    setSelectionState(CitySelectionState::GotoLocation);
				    break;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_VEHICLE_ATTACK")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    setSelectionState(CitySelectionState::AttackVehicle);
				    break;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_VEHICLE_ATTACK_BUILDING")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    setSelectionState(CitySelectionState::AttackBuilding);
				    break;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_GOTO_BASE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) { orderGoToBase(); });

	vehicleForm->findControl("BUTTON_ATTACK_MODE_AGGRESSIVE")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    v->attackMode = Vehicle::AttackMode::Aggressive;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_ATTACK_MODE_STANDARD")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    v->attackMode = Vehicle::AttackMode::Standard;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_ATTACK_MODE_DEFENSIVE")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    v->attackMode = Vehicle::AttackMode::Defensive;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_ATTACK_MODE_EVASIVE")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    v->attackMode = Vehicle::AttackMode::Evasive;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_ALTITUDE_HIGHEST")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    v->altitude = Vehicle::Altitude::Highest;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_ALTITUDE_HIGH")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    v->altitude = Vehicle::Altitude::High;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_ALTITUDE_STANDARD")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    v->altitude = Vehicle::Altitude::Standard;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_ALTITUDE_LOW")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    v->altitude = Vehicle::Altitude::Low;
			    }
		    }
		});
	auto agentForm = this->uiTabs[2];
	agentForm->findControl("BUTTON_AGENT_BUILDING")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    if (!this->state->current_city->cityViewSelectedAgents.empty())
		    {
			    fw().stageQueueCommand(
			        {StageCmd::Command::PUSH,
			         mksp<LocationScreen>(
			             this->state, this->state->current_city->cityViewSelectedAgents.front())});
		    }
		});
	agentForm->findControl("BUTTON_EQUIP_AGENT")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH,
		         mksp<AEquipScreen>(this->state,
		                            !this->state->current_city->cityViewSelectedAgents.empty()
		                                ? this->state->current_city->cityViewSelectedAgents.front()
		                                : nullptr)});
		});

	agentForm->findControl("BUTTON_GOTO_BUILDING")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    if (!this->state->current_city->cityViewSelectedAgents.empty())
		    {
			    setSelectionState(CitySelectionState::GotoBuilding);
		    }
		});
	agentForm->findControl("BUTTON_GOTO_BASE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) { orderGoToBase(); });

	this->uiTabs[3]
	    ->findControl("BUTTON_RESEARCH")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    sp<Facility> lab;
		    for (auto &a : this->state->current_city->cityViewSelectedAgents)
		    {
			    if (a && a->type->role == AgentType::Role::BioChemist)
			    {
				    this->state->current_base = a->homeBuilding->base;
				    if (a->assigned_to_lab)
				    {
					    auto thisRef = StateRef<Agent>{this->state.get(), a};
					    for (auto &fac : this->state->current_base->facilities)
					    {
						    if (!fac->lab)
						    {
							    continue;
						    }
						    auto it = std::find(fac->lab->assigned_agents.begin(),
						                        fac->lab->assigned_agents.end(), thisRef);
						    if (it != fac->lab->assigned_agents.end())
						    {
							    lab = fac;
							    break;
						    }
					    }
				    }
				    else
				    {
					    for (auto &f : this->state->current_base->facilities)
					    {
						    if (f->type->capacityType == FacilityType::Capacity::Chemistry)
						    {
							    lab = f;
							    break;
						    }
					    }
				    }
				    break;
			    }
		    }
		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH, mksp<ResearchScreen>(this->state, lab)});
		});
	this->uiTabs[4]
	    ->findControl("BUTTON_RESEARCH")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    sp<Facility> lab;
		    for (auto &a : this->state->current_city->cityViewSelectedAgents)
		    {
			    if (a && a->type->role == AgentType::Role::Engineer)
			    {
				    this->state->current_base = a->homeBuilding->base;
				    if (a->assigned_to_lab)
				    {
					    auto thisRef = StateRef<Agent>{this->state.get(), a};
					    for (auto &fac : this->state->current_base->facilities)
					    {
						    if (!fac->lab)
						    {
							    continue;
						    }
						    auto it = std::find(fac->lab->assigned_agents.begin(),
						                        fac->lab->assigned_agents.end(), thisRef);
						    if (it != fac->lab->assigned_agents.end())
						    {
							    lab = fac;
							    break;
						    }
					    }
				    }
				    else
				    {
					    for (auto &f : this->state->current_base->facilities)
					    {
						    if (f->type->capacityType == FacilityType::Capacity::Workshop)
						    {
							    lab = f;
							    break;
						    }
					    }
				    }
				    break;
			    }
		    }
		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH, mksp<ResearchScreen>(this->state, lab)});
		});
	this->uiTabs[5]
	    ->findControl("BUTTON_RESEARCH")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    sp<Facility> lab;
		    for (auto &a : this->state->current_city->cityViewSelectedAgents)
		    {
			    if (a && a->type->role == AgentType::Role::Physicist)
			    {
				    this->state->current_base = a->homeBuilding->base;
				    if (a->assigned_to_lab)
				    {
					    auto thisRef = StateRef<Agent>{this->state.get(), a};
					    for (auto &fac : this->state->current_base->facilities)
					    {
						    if (!fac->lab)
						    {
							    continue;
						    }
						    auto it = std::find(fac->lab->assigned_agents.begin(),
						                        fac->lab->assigned_agents.end(), thisRef);
						    if (it != fac->lab->assigned_agents.end())
						    {
							    lab = fac;
							    break;
						    }
					    }
				    }
				    else
				    {
					    for (auto &f : this->state->current_base->facilities)
					    {
						    if (f->type->capacityType == FacilityType::Capacity::Physics)
						    {
							    lab = f;
							    break;
						    }
					    }
				    }
				    break;
			    }
		    }
		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH, mksp<ResearchScreen>(this->state, lab)});
		});

	this->uiTabs[7]
	    ->findControl("BUTTON_SHOW_ALL")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    this->state->current_city->cityViewOrgButtonIndex = 0;
		    uiTabs[7]->findControlTyped<ListBox>("ORGANISATION_LIST")->scroller->setValue(0);
		});
	this->uiTabs[7]
	    ->findControl("BUTTON_SHOW_ALLIED")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    this->state->current_city->cityViewOrgButtonIndex = 1;
		    uiTabs[7]->findControlTyped<ListBox>("ORGANISATION_LIST")->scroller->setValue(0);
		});
	this->uiTabs[7]
	    ->findControl("BUTTON_SHOW_FRIENDLY")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    this->state->current_city->cityViewOrgButtonIndex = 2;
		    uiTabs[7]->findControlTyped<ListBox>("ORGANISATION_LIST")->scroller->setValue(0);
		});
	this->uiTabs[7]
	    ->findControl("BUTTON_SHOW_NEUTRAL")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    this->state->current_city->cityViewOrgButtonIndex = 3;
		    uiTabs[7]->findControlTyped<ListBox>("ORGANISATION_LIST")->scroller->setValue(0);
		});
	this->uiTabs[7]
	    ->findControl("BUTTON_SHOW_UNFRIENDLY")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    this->state->current_city->cityViewOrgButtonIndex = 4;
		    uiTabs[7]->findControlTyped<ListBox>("ORGANISATION_LIST")->scroller->setValue(0);
		});
	this->uiTabs[7]
	    ->findControl("BUTTON_SHOW_HOSTILE")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    this->state->current_city->cityViewOrgButtonIndex = 5;
		    uiTabs[7]->findControlTyped<ListBox>("ORGANISATION_LIST")->scroller->setValue(0);
		});
	this->uiTabs[7]
	    ->findControl("BUTTON_BRIBE")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { LogWarning("Implement bribery screen"); });

	auto font = ui().getFont("smallset");
	for (int i = 0; i <= state->current_city->roadSegments.size(); i++)
	{
		debugLabelsOK.push_back(font->getString(format("%d", i)));
		debugLabelsDead.push_back(font->getString(format("-%d-", i)));
	}

#ifdef DEBUG_START_PAUSE
	setUpdateSpeed(CityUpdateSpeed::Pause);
#endif
}

CityView::~CityView() = default;

void CityView::begin()
{
	vanillaControls = !config().getBool("OpenApoc.NewFeature.OpenApocCityControls");
	CityTileView::begin();
	if (state->newGame)
	{
		state->newGame = false;
		baseForm->findControlTyped<Ticker>("NEWS_TICKER")
		    ->addMessage(tr("Welcome to X-COM Apocalypse"));
	}
}

void CityView::resume()
{
	vanillaControls = !config().getBool("OpenApoc.NewFeature.OpenApocCityControls");
	CityTileView::resume();
	modifierLAlt = false;
	modifierLCtrl = false;
	modifierLShift = false;
	modifierRAlt = false;
	modifierRCtrl = false;
	modifierRShift = false;

	this->uiTabs[0]->findControlTyped<Label>("TEXT_BASE_NAME")->setText(state->current_base->name);
	miniViews.clear();
	int b = 0;
	for (auto &pair : state->player_bases)
	{
		auto &viewBase = pair.second;
		auto viewName = format("BUTTON_BASE_%d", ++b);
		auto view = this->uiTabs[0]->findControlTyped<GraphicButton>(viewName);
		if (!view)
		{
			LogError("Failed to find UI control matching \"%s\"", viewName);
		}
		view->setData(viewBase);
		auto viewImage = BaseGraphics::drawMiniBase(viewBase);
		view->setImage(viewImage);
		view->setDepressedImage(viewImage);
		view->addCallback(FormEventType::ButtonClick, [this](FormsEvent *e) {
			auto clickedBase =
			    StateRef<Base>(this->state.get(), e->forms().RaisedBy->getData<Base>());
			if (clickedBase == this->state->current_base)
			{
				this->setScreenCenterTile(clickedBase->building->crewQuarters);
			}
			else
			{
				this->state->current_base = clickedBase;
				this->uiTabs[0]
				    ->findControlTyped<Label>("TEXT_BASE_NAME")
				    ->setText(this->state->current_base->name);
			}
		});
		miniViews.push_back(view);
	}
}

void CityView::render()
{
	TRACE_FN;

	if (!this->surface)
	{
		this->drawCity = true;
		this->surface = mksp<Surface>(fw().displayGetSize());
	}

	if (drawCity)
	{
		this->drawCity = false;
		RendererSurfaceBinding b(*fw().renderer, this->surface);

		CityTileView::render();
		if (DEBUG_SHOW_VEHICLE_PATH)
		{
			for (auto &pair : state->vehicles)
			{
				auto v = pair.second;
				if (v->city != state->current_city)
					continue;
				auto vTile = v->tileObject;
				if (!vTile)
					continue;
				if (v->missions.empty())
				{
					continue;
				}
				auto &path = v->missions.front()->currentPlannedPath;
				Vec3<float> prevPos = vTile->getPosition();
				for (auto &pos : path)
				{
					Vec3<float> posf = pos;
					posf += Vec3<float>{0.5f, 0.5f, 0.5f};
					Vec2<float> screenPosA = this->tileToOffsetScreenCoords(prevPos);
					Vec2<float> screenPosB = this->tileToOffsetScreenCoords(posf);

					fw().renderer->drawLine(screenPosA, screenPosB, Colour{255, 255, 192, 255});

					prevPos = posf;
				}
			}
		}
		if (DEBUG_SHOW_ROAD_PATHFINDING)
		{
			static const auto lineColorFriend = Colour(0, 0, 0, 255);
			static const auto lineColorEnemy = Colour(255, 0, 0, 255);

			for (int i = 0; i < state->current_city->roadSegments.size(); i++)
			{
				auto &s = state->current_city->roadSegments[i];
				if (s.empty())
				{
					continue;
				}
				// Connections
				auto color = (s.connections.size() > 2 && s.tilePosition.size() > 1)
				                 ? lineColorEnemy
				                 : lineColorFriend;
				int count = 0;
				for (auto &c : s.connections)
				{
					Vec3<float> thisPos =
					    s.connections.front() == c ? s.tilePosition.front() : s.tilePosition.back();
					thisPos += Vec3<float>{0.5f, 0.5f, 0.0f};
					auto &s2 = state->current_city->roadSegments[c];
					Vec3<float> tarPos = s2.connections.front() == i ? s2.tilePosition.front()
					                                                 : s2.tilePosition.back();
					tarPos += Vec3<float>{0.5f, 0.5f, 0.0f};
					fw().renderer->drawLine(
					    this->tileToOffsetScreenCoords(thisPos),
					    this->tileToOffsetScreenCoords(thisPos * 0.6f + tarPos * 0.4f), color);
					if (s.connections.size() > 2 && s.tilePosition.size() > 1)
					{
						auto &img = debugLabelsOK[count++];
						fw().renderer->draw(img,
						                    this->tileToOffsetScreenCoords(thisPos) +
						                        Vec2<float>{count * 8, -10});
					}
				}
				// Tiles
				for (auto j = 0; j < s.tilePosition.size(); j++)
				{
					auto &img = s.tileIntact[j] ? debugLabelsOK[i] : debugLabelsDead[i];
					fw().renderer->draw(img, this->tileToOffsetScreenCoords(s.tilePosition[j]));
				}
			}
		}

		activeTab->render();
		baseForm->render();
		overlayTab->render();
		if (activeTab == uiTabs[0])
		{
			// Highlight selected base
			for (auto &view : miniViews)
			{
				auto viewBase = view->getData<Base>();
				if (state->current_base == viewBase)
				{
					Vec2<int> pos = uiTabs[0]->Location + view->Location - 1;
					Vec2<int> size = view->Size + 2;
					fw().renderer->drawRect(pos, size, Colour{255, 0, 0});
					break;
				}
			}
		}
	}

	// If there's a modal dialog, darken the screen
	if (fw().stageGetCurrent() != this->shared_from_this())
	{
		fw().renderer->drawTinted(this->surface, {0, 0}, {128, 128, 128, 255});
	}
	else
	{
		fw().renderer->draw(this->surface, {0, 0});
	}
}

void CityView::update()
{
	this->drawCity = true;
	CityTileView::update();

	unsigned int ticks = 0;
	bool turbo = false;
	switch (this->updateSpeed)
	{
		case CityUpdateSpeed::Pause:
			ticks = 0;
			break;
		/* POSSIBLE FIXME: 'vanilla' apoc appears to implement Speed1 as 1/2 speed - that is
		    * only
		    * every other call calls the update loop, meaning that the later update tick counts are
		    * halved as well.
		    * This effectively means that all openapoc tick counts count for 1/2 the value of
		    * vanilla
		    * apoc ticks */
		case CityUpdateSpeed::Speed1:
			ticks = 1;
			break;
		case CityUpdateSpeed::Speed2:
			ticks = 2;
			break;
		case CityUpdateSpeed::Speed3:
			ticks = 4;
			break;
		case CityUpdateSpeed::Speed4:
			ticks = 6;
			break;
		case CityUpdateSpeed::Speed5:
			if (!this->state->canTurbo())
			{
				setUpdateSpeed(CityUpdateSpeed::Speed1);
				ticks = 1;
			}
			else
			{
				turbo = true;
			}
			break;
	}
	baseForm->findControl("BUTTON_SPEED5")->Enabled = this->state->canTurbo();

	if (turbo)
	{
		this->state->updateTurbo();
		if (!this->state->canTurbo())
		{
			setUpdateSpeed(CityUpdateSpeed::Speed1);
		}
	}
	else
	{
		while (ticks > 0)
		{
			int ticksPerUpdate = UPDATE_EVERY_TICK ? 1 : ticks;
			state->update(ticksPerUpdate);
			ticks -= ticksPerUpdate;
		}
	}

	updateSelectedUnits();

	// Update time display
	auto clockControl = baseForm->findControlTyped<Label>("CLOCK");
	clockControl->setText(state->gameTime.getLongTimeString());

	// Pulsate palette colors
	colorCurrent += (colorForward ? 1 : -1);
	if (colorCurrent <= 0 || colorCurrent >= 15)
	{
		colorCurrent = clamp(colorCurrent, 0, 15);
		colorForward = !colorForward;
	}

	// The palette fades from pal_03 at 3am to pal_02 at 6am then pal_01 at 9am
	// The reverse for 3pm, 6pm & 9pm

	auto hour = state->gameTime.getHours();
	sp<Palette> interpolated_palette;
	if (hour < 3 || hour >= 21)
	{
		interpolated_palette = this->mod_night_palette[colorCurrent];
	}
	else if (hour >= 9 && hour < 15)
	{
		interpolated_palette = this->mod_day_palette[colorCurrent];
	}
	else
	{
		sp<Palette> palette1;
		sp<Palette> palette2;
		float factor = 0;

		float hours_float = hour + (float)state->gameTime.getMinutes() / 60.0f;

		if (hour >= 3 && hour < 6)
		{
			palette1 = this->mod_night_palette[colorCurrent];
			palette2 = this->mod_twilight_palette[colorCurrent];
			factor = clamp((hours_float - 3.0f) / 3.0f, 0.0f, 1.0f);
		}
		else if (hour >= 6 && hour < 9)
		{
			palette1 = this->mod_twilight_palette[colorCurrent];
			palette2 = this->mod_day_palette[colorCurrent];
			factor = clamp((hours_float - 6.0f) / 3.0f, 0.0f, 1.0f);
		}
		else if (hour >= 15 && hour < 18)
		{
			palette1 = this->mod_day_palette[colorCurrent];
			palette2 = this->mod_twilight_palette[colorCurrent];
			factor = clamp((hours_float - 15.0f) / 3.0f, 0.0f, 1.0f);
		}
		else if (hour >= 18 && hour < 21)
		{
			palette1 = this->mod_twilight_palette[colorCurrent];
			palette2 = this->mod_night_palette[colorCurrent];
			factor = clamp((hours_float - 18.0f) / 3.0f, 0.0f, 1.0f);
		}
		else
		{
			LogError("Unhandled hoursClamped %d", hour);
		}

		interpolated_palette = mksp<Palette>();
		for (int i = 0; i < 256; i++)
		{
			auto &colour1 = palette1->getColour(i);
			auto &colour2 = palette2->getColour(i);
			Colour interpolated_colour;

			interpolated_colour.r = (int)mix((float)colour1.r, (float)colour2.r, factor);
			interpolated_colour.g = (int)mix((float)colour1.g, (float)colour2.g, factor);
			interpolated_colour.b = (int)mix((float)colour1.b, (float)colour2.b, factor);
			interpolated_colour.a = (int)mix((float)colour1.a, (float)colour2.a, factor);
			interpolated_palette->setColour(i, interpolated_colour);
		}
	}

	this->pal = interpolated_palette;

	// Update owned vehicle controls
	if (activeTab == uiTabs[1])
	{
		auto ownedVehicleList = uiTabs[1]->findControlTyped<ListBox>("OWNED_VEHICLE_LIST");
		if (!ownedVehicleList)
		{
			LogError("Failed to find \"OWNED_VEHICLE_LIST\" control on city tab \"%s\"",
			         TAB_FORM_NAMES[1]);
		}

		int currentVehicleIndex = -1;
		std::set<sp<Vehicle>> vehiclesMIA;
		for (auto &i : ownedVehicleInfoList)
		{
			vehiclesMIA.insert(i.vehicle);
		}
		for (auto &v : state->vehicles)
		{
			auto vehicle = v.second;
			if (vehicle->owner != state->getPlayer() || v.second->isDead())
			{
				continue;
			}
			currentVehicleIndex++;
			auto info = ControlGenerator::createVehicleInfo(*state, vehicle);
			vehiclesMIA.erase(info.vehicle);
			bool redo = currentVehicleIndex >= ownedVehicleInfoList.size() ||
			            ownedVehicleInfoList[currentVehicleIndex] != info;
			if (redo)
			{
				auto control = ControlGenerator::createVehicleControl(*state, info);
				control->addCallback(FormEventType::MouseDown, [this, vehicle](FormsEvent *e) {
					if (!this->vanillaControls)
					{
						if (Event::isPressed(e->forms().MouseInfo.Button,
						                     Event::MouseButton::Right))
						{
							// [Alt/Ctrl] + [Shift] opens equipment
							if ((modifierLShift || modifierRShift) &&
							    (modifierLAlt || modifierRAlt || modifierLCtrl || modifierRCtrl))
							{
								// Equipscreen for owner vehicles
								auto equipScreen = mksp<VEquipScreen>(this->state);
								equipScreen->setSelectedVehicle(vehicle);
								fw().stageQueueCommand({StageCmd::Command::PUSH, equipScreen});
								return;
							}
							// [Shift] opens location
							if (modifierLShift || modifierRShift)
							{
								// Location screen
								fw().stageQueueCommand(
								    {StageCmd::Command::PUSH,
								     mksp<LocationScreen>(this->state, vehicle)});
								return;
							}
						}
					}
					handleClickedVehicle(
					    StateRef<Vehicle>{state.get(), Vehicle::getId(*state, vehicle)},
					    Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Right),
					    CitySelectionState::Normal);
				});
				if (currentVehicleIndex >= ownedVehicleInfoList.size())
				{
					ownedVehicleInfoList.push_back(info);
				}
				else
				{
					ownedVehicleInfoList[currentVehicleIndex] = info;
				}
				ownedVehicleList->replaceItem(control);
			}
		}
		for (auto &v : vehiclesMIA)
		{
			ownedVehicleList->removeByData<Vehicle>(v);
		}
		ownedVehicleInfoList.resize(currentVehicleIndex + 1);
	}

	// Update soldier agent controls
	if (activeTab == uiTabs[2])
	{
		auto ownedAgentList = uiTabs[2]->findControlTyped<ListBox>("OWNED_AGENT_LIST");
		if (!ownedAgentList)
		{
			LogError("Failed to find \"OWNED_AGENT_LIST\" control on city tab \"%s\"",
			         TAB_FORM_NAMES[2]);
		}
		int currentAgentIndex = -1;
		std::set<sp<Agent>> agentsMIA;
		for (auto &i : ownedSoldierInfoList)
		{
			agentsMIA.insert(i.agent);
		}

		for (auto &a : state->agents)
		{
			auto agent = a.second;
			if (agent->owner != state->getPlayer() || agent->isDead() ||
			    agent->type->role != AgentType::Role::Soldier)
			{
				continue;
			}
			currentAgentIndex++;
			auto info = ControlGenerator::createAgentInfo(*state, agent);
			agentsMIA.erase(info.agent);
			bool redo = currentAgentIndex >= ownedSoldierInfoList.size() ||
			            ownedSoldierInfoList[currentAgentIndex] != info;
			if (redo)
			{
				auto control = ControlGenerator::createAgentControl(*state, info);
				control->addCallback(FormEventType::MouseDown, [this, agent](FormsEvent *e) {
					if (!this->vanillaControls)
					{
						if (Event::isPressed(e->forms().MouseInfo.Button,
						                     Event::MouseButton::Right))
						{
							// [Alt/Ctrl] + [Shift] opens equipment
							if ((modifierLShift || modifierRShift) &&
							    (modifierLAlt || modifierRAlt || modifierLCtrl || modifierRCtrl))
							{
								// Equipscreen for owner vehicles
								auto equipScreen = mksp<AEquipScreen>(this->state, agent);
								fw().stageQueueCommand({StageCmd::Command::PUSH, equipScreen});
								return;
							}
							// [Shift] opens location
							if (modifierLShift || modifierRShift)
							{
								// Location screen
								fw().stageQueueCommand({StageCmd::Command::PUSH,
								                        mksp<LocationScreen>(this->state, agent)});
								return;
							}
						}
					}
					handleClickedAgent(
					    StateRef<Agent>{state.get(), Agent::getId(*state, agent)},
					    Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Right),
					    CitySelectionState::Normal);

				});
				if (currentAgentIndex >= ownedSoldierInfoList.size())
				{
					ownedSoldierInfoList.push_back(info);
				}
				else
				{
					ownedSoldierInfoList[currentAgentIndex] = info;
				}
				ownedAgentList->replaceItem(control);
			}
		}
		for (auto &a : agentsMIA)
		{
			ownedAgentList->removeByData<Agent>(a);
		}
		ownedSoldierInfoList.resize(currentAgentIndex + 1);
	}

	// Update bio agent controls
	if (activeTab == uiTabs[3])
	{
		auto ownedAgentList = uiTabs[3]->findControlTyped<ListBox>("OWNED_AGENT_LIST");
		if (!ownedAgentList)
		{
			LogError("Failed to find \"OWNED_AGENT_LIST\" control on city tab \"%s\"",
			         TAB_FORM_NAMES[3]);
		}
		int currentAgentIndex = -1;
		std::set<sp<Agent>> agentsMIA;
		for (auto &i : ownedBioInfoList)
		{
			agentsMIA.insert(i.agent);
		}
		for (auto &a : state->agents)
		{
			auto agent = a.second;
			if (agent->owner != state->getPlayer() || agent->isDead() ||
			    agent->type->role != AgentType::Role::BioChemist)
			{
				continue;
			}
			currentAgentIndex++;
			auto info = ControlGenerator::createAgentInfo(*state, agent);
			agentsMIA.erase(info.agent);
			bool redo = currentAgentIndex >= ownedBioInfoList.size() ||
			            ownedBioInfoList[currentAgentIndex] != info;
			if (redo)
			{
				auto control = ControlGenerator::createAgentControl(*state, info);
				control->addCallback(FormEventType::MouseDown, [this, agent](FormsEvent *e) {
					if (!this->vanillaControls)
					{
						if (Event::isPressed(e->forms().MouseInfo.Button,
						                     Event::MouseButton::Right))
						{
							// [Shift] opens location
							if (modifierLShift || modifierRShift)
							{
								// Location screen
								fw().stageQueueCommand({StageCmd::Command::PUSH,
								                        mksp<LocationScreen>(this->state, agent)});
								return;
							}
						}
					}
					handleClickedAgent(
					    StateRef<Agent>{state.get(), Agent::getId(*state, agent)},
					    Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Right),
					    CitySelectionState::Normal);

				});
				if (currentAgentIndex >= ownedBioInfoList.size())
				{
					ownedBioInfoList.push_back(info);
				}
				else
				{
					ownedBioInfoList[currentAgentIndex] = info;
				}
				ownedAgentList->replaceItem(control);
			}
		}
		for (auto &a : agentsMIA)
		{
			ownedAgentList->removeByData<Agent>(a);
		}
		ownedBioInfoList.resize(currentAgentIndex + 1);
	}

	// Update engi agent controls
	if (activeTab == uiTabs[4])
	{
		auto ownedAgentList = uiTabs[4]->findControlTyped<ListBox>("OWNED_AGENT_LIST");
		if (!ownedAgentList)
		{
			LogError("Failed to find \"OWNED_AGENT_LIST\" control on city tab \"%s\"",
			         TAB_FORM_NAMES[4]);
		}
		int currentAgentIndex = -1;
		std::set<sp<Agent>> agentsMIA;
		for (auto &i : ownedEngineerInfoList)
		{
			agentsMIA.insert(i.agent);
		}
		for (auto &a : state->agents)
		{
			auto agent = a.second;
			if (agent->owner != state->getPlayer() || agent->isDead() ||
			    agent->type->role != AgentType::Role::Engineer)
			{
				continue;
			}
			currentAgentIndex++;
			auto info = ControlGenerator::createAgentInfo(*state, agent);
			agentsMIA.erase(info.agent);
			bool redo = currentAgentIndex >= ownedEngineerInfoList.size() ||
			            ownedEngineerInfoList[currentAgentIndex] != info;
			if (redo)
			{
				auto control = ControlGenerator::createAgentControl(*state, info);
				control->addCallback(FormEventType::MouseDown, [this, agent](FormsEvent *e) {
					if (!this->vanillaControls)
					{
						if (Event::isPressed(e->forms().MouseInfo.Button,
						                     Event::MouseButton::Right))
						{
							// [Shift] opens location
							if (modifierLShift || modifierRShift)
							{
								// Location screen
								fw().stageQueueCommand({StageCmd::Command::PUSH,
								                        mksp<LocationScreen>(this->state, agent)});
								return;
							}
						}
					}
					handleClickedAgent(
					    StateRef<Agent>{state.get(), Agent::getId(*state, agent)},
					    Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Right),
					    CitySelectionState::Normal);

				});
				if (currentAgentIndex >= ownedEngineerInfoList.size())
				{
					ownedEngineerInfoList.push_back(info);
				}
				else
				{
					ownedEngineerInfoList[currentAgentIndex] = info;
				}
				ownedAgentList->replaceItem(control);
			}
		}
		for (auto &a : agentsMIA)
		{
			ownedAgentList->removeByData<Agent>(a);
		}
		ownedEngineerInfoList.resize(currentAgentIndex + 1);
	}

	// Update phys agent controls
	if (activeTab == uiTabs[5])
	{
		auto ownedAgentList = uiTabs[5]->findControlTyped<ListBox>("OWNED_AGENT_LIST");
		if (!ownedAgentList)
		{
			LogError("Failed to find \"OWNED_AGENT_LIST\" control on city tab \"%s\"",
			         TAB_FORM_NAMES[5]);
		}
		int currentAgentIndex = -1;
		std::set<sp<Agent>> agentsMIA;
		for (auto &i : ownedPhysicsInfoList)
		{
			agentsMIA.insert(i.agent);
		}
		for (auto &a : state->agents)
		{
			auto agent = a.second;
			if (agent->owner != state->getPlayer() || agent->isDead() ||
			    agent->type->role != AgentType::Role::Physicist)
			{
				continue;
			}
			currentAgentIndex++;
			auto info = ControlGenerator::createAgentInfo(*state, agent);
			agentsMIA.erase(info.agent);
			bool redo = currentAgentIndex >= ownedPhysicsInfoList.size() ||
			            ownedPhysicsInfoList[currentAgentIndex] != info;
			if (redo)
			{
				auto control = ControlGenerator::createAgentControl(*state, info);
				control->addCallback(FormEventType::MouseDown, [this, agent](FormsEvent *e) {
					if (!this->vanillaControls)
					{
						if (Event::isPressed(e->forms().MouseInfo.Button,
						                     Event::MouseButton::Right))
						{
							// [Shift] opens location
							if (modifierLShift || modifierRShift)
							{
								// Location screen
								fw().stageQueueCommand({StageCmd::Command::PUSH,
								                        mksp<LocationScreen>(this->state, agent)});
								return;
							}
						}
					}
					handleClickedAgent(
					    StateRef<Agent>{state.get(), Agent::getId(*state, agent)},
					    Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Right),
					    CitySelectionState::Normal);

				});
				if (currentAgentIndex >= ownedPhysicsInfoList.size())
				{
					ownedPhysicsInfoList.push_back(info);
				}
				else
				{
					ownedPhysicsInfoList[currentAgentIndex] = info;
				}
				ownedAgentList->replaceItem(control);
			}
		}
		for (auto &a : agentsMIA)
		{
			ownedAgentList->removeByData<Agent>(a);
		}
		ownedPhysicsInfoList.resize(currentAgentIndex + 1);
	}

	// Update owned vehicle controls
	if (activeTab == uiTabs[6])
	{
		auto hostileVehicleList = uiTabs[6]->findControlTyped<ListBox>("HOSTILE_VEHICLE_LIST");
		if (!hostileVehicleList)
		{
			LogError("Failed to find \"HOSTILE_VEHICLE_LIST\" control on city tab \"%s\"",
			         TAB_FORM_NAMES[6]);
		}

		int currentVehicleIndex = -1;
		std::set<sp<Vehicle>> vehiclesMIA;
		for (auto &i : hostileVehicleInfoList)
		{
			vehiclesMIA.insert(i.vehicle);
		}
		for (auto &v : state->vehicles)
		{
			auto vehicle = v.second;
			if (!v.second->tileObject || v.second->city != state->current_city ||
			    v.second->isDead())
			{
				continue;
			}
			if (state->getPlayer()->isRelatedTo(vehicle->owner) != Organisation::Relation::Hostile)
			{
				if (state->current_city->cityViewSelectedVehicles.empty() ||
				    state->current_city->cityViewSelectedVehicles.front() != v.second)
				{
					continue;
				}
			}

			currentVehicleIndex++;
			auto info = ControlGenerator::createVehicleInfo(*state, vehicle);
			vehiclesMIA.erase(info.vehicle);
			bool redo = currentVehicleIndex >= hostileVehicleInfoList.size() ||
			            hostileVehicleInfoList[currentVehicleIndex] != info;
			if (redo)
			{
				auto control = ControlGenerator::createVehicleControl(*state, info);
				control->addCallback(FormEventType::MouseDown, [this, vehicle](FormsEvent *e) {
					// if (!this->vanillaControls)
					//{
					//	if (Event::isPressed(e->forms().MouseInfo.Button,
					//		Event::MouseButton::Right))
					//	{
					//		// [Alt/Ctrl] + [Shift] opens equipment
					//		if ((modifierLShift || modifierRShift) &&
					//			(modifierLAlt || modifierRAlt || modifierLCtrl ||
					//				modifierRCtrl))
					//		{
					//			// Equipscreen for owner vehicles
					//			auto equipScreen = mksp<VEquipScreen>(this->state);
					//			equipScreen->setSelectedVehicle(vehicle);
					//			fw().stageQueueCommand({ StageCmd::Command::PUSH, equipScreen });
					//			return;
					//		}
					//		// [Shift] opens location
					//		if (modifierLShift || modifierRShift)
					//		{
					//			// Location screen
					//			fw().stageQueueCommand(
					//			{ StageCmd::Command::PUSH,
					//				mksp<LocationScreen>(this->state, vehicle) });
					//			return;
					//		}
					//	}
					//}
					handleClickedVehicle(
					    StateRef<Vehicle>{state.get(), Vehicle::getId(*state, vehicle)},
					    Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Right),
					    CitySelectionState::Normal);
				});
				if (currentVehicleIndex >= hostileVehicleInfoList.size())
				{
					hostileVehicleInfoList.push_back(info);
				}
				else
				{
					hostileVehicleInfoList[currentVehicleIndex] = info;
				}
				hostileVehicleList->replaceItem(control);
			}
		}
		for (auto &v : vehiclesMIA)
		{
			hostileVehicleList->removeByData<Vehicle>(v);
		}
		hostileVehicleInfoList.resize(currentVehicleIndex + 1);
	}

	// Update org
	if (activeTab == uiTabs[7])
	{
		auto orgList = uiTabs[7]->findControlTyped<ListBox>("ORGANISATION_LIST");
		if (!orgList)
		{
			LogError("Failed to find \"ORGANISATION_LIST\" control on city tab \"%s\"",
			         TAB_FORM_NAMES[7]);
		}

		int currentOrgIndex = -1;
		std::set<sp<Organisation>> orgsMIA;
		for (auto &i : organisationInfoList)
		{
			orgsMIA.insert(i.organisation);
		}
		for (auto &o : state->organisations)
		{
			auto org = o.second;
			if (state->getCivilian() == org || state->getAliens() == org ||
			    state->getPlayer() == org)
			{
				continue;
			}
			auto rel =
			    state->current_city->cityViewOrgButtonIndex == 0
			        ? (Organisation::Relation)0
			        : (Organisation::Relation)(state->current_city->cityViewOrgButtonIndex - 1);
			if (state->current_city->cityViewOrgButtonIndex != 0 &&
			    state->getPlayer()->isRelatedTo({state.get(), o.first}) != rel)
			{
				continue;
			}
			currentOrgIndex++;
			auto info = ControlGenerator::createOrganisationInfo(*state, org);
			orgsMIA.erase(info.organisation);
			bool redo = currentOrgIndex >= organisationInfoList.size() ||
			            organisationInfoList[currentOrgIndex] != info;
			if (redo)
			{
				auto control = ControlGenerator::createOrganisationControl(*state, info);
				control->addCallback(FormEventType::MouseDown, [this, org](FormsEvent *e) {
					// if (!this->vanillaControls)
					//{
					//	if (Event::isPressed(e->forms().MouseInfo.Button,
					//		Event::MouseButton::Right))
					//	{
					//		// [Alt/Ctrl] + [Shift] opens equipment
					//		if ((modifierLShift || modifierRShift) &&
					//			(modifierLAlt || modifierRAlt || modifierLCtrl ||
					//				modifierRCtrl))
					//		{
					//			// Equipscreen for owner vehicles
					//			auto equipScreen = mksp<VEquipScreen>(this->state);
					//			equipScreen->setSelectedVehicle(vehicle);
					//			fw().stageQueueCommand({ StageCmd::Command::PUSH, equipScreen });
					//			return;
					//		}
					//		// [Shift] opens location
					//		if (modifierLShift || modifierRShift)
					//		{
					//			// Location screen
					//			fw().stageQueueCommand(
					//			{ StageCmd::Command::PUSH,
					//				mksp<LocationScreen>(this->state, vehicle) });
					//			return;
					//		}
					//	}
					//}
					handleClickedOrganisation(
					    StateRef<Organisation>{state.get(), org->id},
					    Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Right),
					    CitySelectionState::Normal);
				});
				if (currentOrgIndex >= organisationInfoList.size())
				{
					organisationInfoList.push_back(info);
				}
				else
				{
					organisationInfoList[currentOrgIndex] = info;
				}
				orgList->replaceItem(control);
			}
		}
		for (auto &o : orgsMIA)
		{
			orgList->removeByData<Organisation>(o);
		}
		organisationInfoList.resize(currentOrgIndex + 1);
	}
	else
	{
		// Clear org selection on exit
		state->current_city->cityViewSelectedOrganisation.clear();
	}

	activeTab->update();
	baseForm->update();
	overlayTab->update();

	// If we have 'follow vehicle' enabled we clobber any other movement that may have occurred in
	// this frame
	if (this->followVehicle)
	{
		if (!state->current_city->cityViewSelectedVehicles.empty())
		{
			auto v = state->current_city->cityViewSelectedVehicles.front();
			if (v->city == state->current_city)
			{
				this->setScreenCenterTile(v->position);
			}
		}
		else if (!state->current_city->cityViewSelectedAgents.empty())
		{
			auto a = state->current_city->cityViewSelectedAgents.front();
			if (a->city == state->current_city)
			{
				if (a->currentVehicle)
				{
					this->setScreenCenterTile(a->currentVehicle->position);
				}
				else
				{
					this->setScreenCenterTile(a->position);
				}
			}
		}
	}
	// Store screen center for serialisation
	state->current_city->cityViewScreenCenter = centerPos;
}

void CityView::initiateUfoMission(StateRef<Vehicle> ufo, StateRef<Vehicle> playerCraft)
{
	bool isBuilding = false;
	bool isRaid = false;
	fw().stageQueueCommand({StageCmd::Command::REPLACEALL,
	                        mksp<BattleBriefing>(state, ufo->owner, ufo.id, isBuilding, isRaid,
	                                             loadBattleVehicle(state, ufo, playerCraft))});
}

void CityView::eventOccurred(Event *e)
{
	this->drawCity = true;
	activeTab->eventOccured(e);
	baseForm->eventOccured(e);
	overlayTab->eventOccured(e);
	// Exclude mouse down events that are over the form
	if (activeTab->eventIsWithin(e) || baseForm->eventIsWithin(e) || overlayTab->eventIsWithin(e))
	{
		// We pass this event so that scroll can work
		if (e->type() != EVENT_MOUSE_MOVE)
		{
			return;
		}
	}

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (handleKeyDown(e))
		{
			return;
		}
	}
	if (e->type() == EVENT_KEY_UP)
	{
		if (handleKeyUp(e))
		{
			return;
		}
	}
	if (e->type() == EVENT_MOUSE_DOWN)
	{
		if (handleMouseDown(e))
		{
			return;
		}
	}
	if (e->type() == EVENT_GAME_STATE)
	{
		if (handleGameStateEvent(e))
		{
			return;
		}
	}
	CityTileView::eventOccurred(e);
}

bool CityView::handleKeyDown(Event *e)
{
	// Common keys active in both debug and normal mode
	switch (e->keyboard().KeyCode)
	{
		case SDLK_RSHIFT:
			modifierRShift = true;
			return true;
		case SDLK_LSHIFT:
			modifierLShift = true;
			return true;
		case SDLK_RALT:
			modifierRAlt = true;
			return true;
		case SDLK_LALT:
			modifierLAlt = true;
			return true;
		case SDLK_RCTRL:
			modifierRCtrl = true;
			return true;
		case SDLK_LCTRL:
			modifierLCtrl = true;
			return true;
	}
	// Cheat codes
	if (e->type() == EVENT_KEY_DOWN)
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_r:
			{
				LogInfo("Repairing...");
				std::set<sp<Scenery>> stuffToRepair;
				for (auto &s : state->current_city->scenery)
				{
					if (s->canRepair())
					{
						stuffToRepair.insert(s);
					}
				}
				LogInfo("Repairing %u tiles out of %u", static_cast<unsigned>(stuffToRepair.size()),
				        static_cast<unsigned>(state->current_city->scenery.size()));

				for (auto &s : stuffToRepair)
				{
					s->repair(*state);
				}
				return true;
			}
			case SDLK_x:
			{
				LogWarning("Crashing!");
				for (auto &v : state->vehicles)
				{
					if (v.second->currentBuilding || v.second->city != state->current_city ||
					    v.second->crashed || v.second->falling)
					{
						continue;
					}
					v.second->health = v.second->type->crash_health > 0
					                       ? v.second->type->crash_health
					                       : v.second->getMaxHealth() / 4;
					v.second->startFalling(*state);
				}
				return true;
			}
			case SDLK_u:
			{
				LogWarning("Spawning crashed UFOs...");

				bool nothing = false;
				auto pos = centerPos;
				pos.z = 9;
				auto ufo = state->current_city->placeVehicle(
				    *state, {state.get(), "VEHICLETYPE_ALIEN_PROBE"}, state->getAliens(), pos);
				ufo->crash(*state, nullptr);
				pos.z++;
				ufo = state->current_city->placeVehicle(
				    *state, {state.get(), "VEHICLETYPE_ALIEN_BATTLESHIP"}, state->getAliens(), pos);
				ufo->crash(*state, nullptr);
				ufo->applyDamage(*state, 1, 0, nothing);
				pos.z++;
				ufo = state->current_city->placeVehicle(
				    *state, {state.get(), "VEHICLETYPE_ALIEN_TRANSPORTER"}, state->getAliens(),
				    pos);
				ufo->crash(*state, nullptr);
				ufo->applyDamage(*state, 1, 0, nothing);

				return true;
			}
			case SDLK_b:
			{
				LogWarning("Spawning base defense mission");
				Vec3<float> pos = {state->current_base->building->bounds.p0.x - 1,
				                   state->current_base->building->bounds.p0.y - 1, 10};
				auto v = state->cities["CITYMAP_HUMAN"]->placeVehicle(
				    *state, StateRef<VehicleType>{state.get(), "VEHICLETYPE_ALIEN_TRANSPORTER"},
				    state->getAliens(), pos);
				v->setMission(*state, VehicleMission::infiltrateOrSubvertBuilding(
				                          *state, *v, state->current_base->building));
				return true;
			}
			case SDLK_p:
			{
				LogWarning("Spawning hired agents");
				StateRef<Organisation> marsec = {state.get(), "ORG_MARSEC"};
				for (auto &b : state->cities["CITYMAP_HUMAN"]->buildings)
				{
					if (b.second->owner == marsec)
					{
						auto a = state->agent_generator.createAgent(
						    *state, marsec,
						    StateRef<AgentType>{state.get(), "AGENTTYPE_X-COM_AGENT_HUMAN"});
						a->homeBuilding = {state.get(), b.first};
						a->enterBuilding(*state, a->homeBuilding);
						a->city = a->homeBuilding->city;
						a->hire(*state, state->current_base->building);
						a = state->agent_generator.createAgent(
						    *state, marsec,
						    StateRef<AgentType>{state.get(), "AGENTTYPE_X-COM_BIOCHEMIST"});
						a->homeBuilding = {state.get(), b.first};
						a->enterBuilding(*state, a->homeBuilding);
						a->city = a->homeBuilding->city;
						a->hire(*state, state->current_base->building);
						break;
					}
				}
				return true;
			}
		}
	}
	// Keyboard commands
	if (e->type() == EVENT_KEY_DOWN)
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_ESCAPE:
				if (selectionState == CitySelectionState::Normal)
				{
					fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<InGameOptions>(state)});
				}
				else
				{
					setSelectionState(CitySelectionState::Normal);
				}
				return true;
			case SDLK_TAB:
				this->baseForm->findControlTyped<CheckBox>("BUTTON_TOGGLE_STRATMAP")
				    ->setChecked(
				        !this->baseForm->findControlTyped<CheckBox>("BUTTON_TOGGLE_STRATMAP")
				             ->isChecked());
				return true;
			case SDLK_SPACE:
				if (this->updateSpeed != CityUpdateSpeed::Pause)
					setUpdateSpeed(CityUpdateSpeed::Pause);
				else
					setUpdateSpeed(this->lastSpeed);
				return true;
			case SDLK_m:
				if (vanillaControls)
				{
					setSelectionState(CitySelectionState::ManualControl);
					return true;
				}
				baseForm->findControl("BUTTON_SHOW_LOG")->click();
				return true;
			case SDLK_n:
				if (!vanillaControls)
				{
					setSelectionState(CitySelectionState::ManualControl);
					return true;
				}
				break;
			case SDLK_HOME:
				baseForm->findControl("BUTTON_ZOOM_EVENT")->click();
				return true;
			case SDLK_c:
				baseForm->findControl("BUTTON_FOLLOW_VEHICLE")->click();
				return true;
			case SDLK_0:
				setUpdateSpeed(CityUpdateSpeed::Pause);
				return true;
			case SDLK_1:
				setUpdateSpeed(CityUpdateSpeed::Speed1);
				return true;
			case SDLK_2:
				setUpdateSpeed(CityUpdateSpeed::Speed2);
				return true;
			case SDLK_3:
				setUpdateSpeed(CityUpdateSpeed::Speed3);
				return true;
			case SDLK_4:
				setUpdateSpeed(CityUpdateSpeed::Speed4);
				return true;
			case SDLK_5:
				setUpdateSpeed(CityUpdateSpeed::Speed5);
				return true;
		}
	}
	return false;
}

bool CityView::handleKeyUp(Event *e)
{
	switch (e->keyboard().KeyCode)
	{
		case SDLK_RSHIFT:
			modifierRShift = false;
			return true;
		case SDLK_LSHIFT:
			modifierLShift = false;
			return true;
		case SDLK_RALT:
			modifierRAlt = false;
			return true;
		case SDLK_LALT:
			modifierLAlt = false;
			return true;
		case SDLK_RCTRL:
			modifierRCtrl = false;
			return true;
		case SDLK_LCTRL:
			modifierLCtrl = false;
			return true;
	}
	return false;
}

bool CityView::handleMouseDown(Event *e)
{
	if (Event::isPressed(e->mouse().Button, Event::MouseButton::Middle) ||
	    (Event::isPressed(e->mouse().Button, Event::MouseButton::Right) && vanillaControls))
	{
		Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
		auto clickTile =
		    this->screenToTileCoords(Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 0.0f);
		this->setScreenCenterTile(Vec2<float>{clickTile.x, clickTile.y});
		return true;
	}
	if (Event::isPressed(e->mouse().Button, Event::MouseButton::Left) ||
	    Event::isPressed(e->mouse().Button, Event::MouseButton::Right))
	{
		auto buttonPressed = Event::isPressed(e->mouse().Button, Event::MouseButton::Left)
		                         ? Event::MouseButton::Left
		                         : Event::MouseButton::Right;

		if (vanillaControls)
		{
			if (buttonPressed == Event::MouseButton::Left)
			{
				if (modifierLAlt || modifierRAlt)
				{
					setSelectionState(CitySelectionState::AttackVehicle);
				}
				else if (modifierLShift || modifierRShift)
				{
					setSelectionState(CitySelectionState::GotoBuilding);
				}
			}
		}

		// If a click has not been handled by a form it's in the map. See if we intersect with
		// anything
		Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
		auto clickTop = this->screenToTileCoords(
		    Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 12.99f);
		auto clickBottom =
		    this->screenToTileCoords(Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 0.0f);
		auto collision =
		    state->current_city->map->findCollision(clickTop, clickBottom, {}, nullptr, true);
		if (collision)
		{
			auto position = collision.position;
			sp<Scenery> scenery;
			StateRef<Building> building;
			sp<Vehicle> vehicle;
			sp<Projectile> projectile;
			switch (collision.obj->getType())
			{
				case TileObject::Type::Scenery:
				{
					scenery =
					    std::dynamic_pointer_cast<TileObjectScenery>(collision.obj)->getOwner();
					building = scenery->building;
					if (true)
					{
						Vec3<int> t = scenery->currentPosition;
						UString debug = "";
						debug +=
						    format("\nCLICKED %s SCENERY %s at %s BUILDING %s",
						           scenery->falling || scenery->willCollapse() ? "FALLING" : "OK",
						           scenery->type.id, t, building.id);
						// debug += format("\n LOS BLOCK %d", battle.getLosBlockID(t.x, t.y, t.z));

						debug += format(
						    "\nHt [%d] Con [%d] Type [%d|%d|%d] Road [%d%d%d%d] Hill [%d%d%d%d] "
						    "Tube "
						    "[%d%d%d%d%d%d]",
						    scenery->type->height, scenery->type->constitution,
						    (int)scenery->type->tile_type, (int)scenery->type->road_type,
						    (int)scenery->type->walk_mode, (int)scenery->type->connection[0],
						    (int)scenery->type->connection[1], (int)scenery->type->connection[2],
						    (int)scenery->type->connection[3], (int)scenery->type->hill[0],
						    (int)scenery->type->hill[1], (int)scenery->type->hill[2],
						    (int)scenery->type->hill[3], (int)scenery->type->tube[0],
						    (int)scenery->type->tube[1], (int)scenery->type->tube[2],
						    (int)scenery->type->tube[3], (int)scenery->type->tube[4],
						    (int)scenery->type->tube[5]);
						auto &map = *state->current_city->map;
						for (auto &p : scenery->supportedBy)
						{
							debug += format("\nCan be supported by %s", p);
						}
						for (auto &p : scenery->supportedParts)
						{
							debug += format("\nSupports %s", p);
						}
						for (int x = t.x - 1; x <= t.x + 1; x++)
						{
							for (int y = t.y - 1; y <= t.y + 1; y++)
							{
								for (int z = t.z - 1; z <= t.z + 1; z++)
								{
									if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y ||
									    z < 0 || z >= map.size.z)
									{
										continue;
									}
									auto tile2 = map.getTile(x, y, z);
									for (auto &o2 : tile2->ownedObjects)
									{
										if (o2->getType() == TileObject::Type::Scenery)
										{
											auto mp2 =
											    std::static_pointer_cast<TileObjectScenery>(o2)
											        ->getOwner();
											for (auto &p : mp2->supportedParts)
											{
												if (p == t)
												{
													debug += format(
													    "\nActually supported by %s at %d %d %d",
													    mp2->type.id, x - t.x, y - t.y, z - t.z);
												}
											}
										}
									}
								}
							}
							LogWarning("%s", debug);
						}
					}

					if (modifierLAlt && modifierLCtrl && modifierLShift)
					{
						if (building && buttonPressed == Event::MouseButton::Right)
						{
							building->collapse(*state);
						}
						else
						{
							scenery->die(*state);
						}
						return true;
					}
					break;
				}
				case TileObject::Type::Vehicle:
				{
					vehicle =
					    std::dynamic_pointer_cast<TileObjectVehicle>(collision.obj)->getVehicle();
					LogWarning("CLICKED VEHICLE %s at %s", vehicle->name, vehicle->position);
					for (auto &m : vehicle->missions)
					{
						LogWarning("Mission %s", m->getName());
					}
					for (auto &c : vehicle->cargo)
					{
						LogWarning("Cargo %dx%s", c.id, c.count);
					}
					if (modifierLAlt && modifierLCtrl && modifierLShift)
					{
						if (buttonPressed == Event::MouseButton::Right)
						{
							vehicle->die(*state);
						}
						else if (!vehicle->falling && !vehicle->crashed)
						{
							vehicle->health = vehicle->type->crash_health > 0
							                      ? vehicle->type->crash_health
							                      : vehicle->getMaxHealth() / 4;
							vehicle->startFalling(*state);
						}
						return true;
					}
					break;
				}
				case TileObject::Type::Projectile:
				{
					projectile = std::dynamic_pointer_cast<TileObjectProjectile>(collision.obj)
					                 ->getProjectile();
					LogInfo("CLICKED PROJECTILE %s at %s", projectile->damage,
					        projectile->position);
					break;
				}
				default:
				{
					LogError("Clicked on some object we didn't care to process?");
					break;
				}
			}
			// Try to handle clicks on objects
			// Click on vehicle
			if (vehicle && handleClickedVehicle(
			                   StateRef<Vehicle>{state.get(), Vehicle::getId(*state, vehicle)},
			                   buttonPressed == Event::MouseButton::Right, selectionState))
			{
				return true;
			}
			// Click on building
			if (building &&
			    handleClickedBuilding(building, buttonPressed == Event::MouseButton::Right,
			                          selectionState))
			{
				return true;
			}
			// Click on projectile
			if (projectile &&
			    handleClickedProjectile(projectile, buttonPressed == Event::MouseButton::Right,
			                            selectionState))
			{
				return true;
			}
			// Otherwise proceed as normal
			switch (selectionState)
			{
				case CitySelectionState::ManualControl:
				{
					if (buttonPressed == Event::MouseButton::Right)
					{
						orderMove(position, modifierRCtrl || modifierLCtrl);
					}
					else
					{
						orderFire(position);
					}
					break;
				}
				case CitySelectionState::GotoLocation:
				{
					// We always have a position if we came this far
					{
						orderMove(position, modifierRCtrl || modifierLCtrl);
						setSelectionState(CitySelectionState::Normal);
					}
					break;
				}
				case CitySelectionState::Normal:
				{
					if (buttonPressed == Event::MouseButton::Right &&
					    (modifierRShift || modifierLShift))
					{
						orderMove(position, modifierRCtrl || modifierLCtrl);
					}
					break;
				}
			}
		}
		return true;
	}
	return true;
}

bool CityView::handleGameStateEvent(Event *e)
{
	auto gameEvent = dynamic_cast<GameEvent *>(e);
	if (!gameEvent)
	{
		LogError("Invalid game state event");
		return true;
	}
	if (!gameEvent->message().empty())
	{
		state->logEvent(gameEvent);
		baseForm->findControlTyped<Ticker>("NEWS_TICKER")->addMessage(gameEvent->message());
		switch (gameEvent->type)
		{
			case GameEventType::AlienSpotted:
			case GameEventType::AlienTakeover:
			case GameEventType::UfoCrashed:
			case GameEventType::UfoRecoveryUnmanned:
			case GameEventType::MissionCompletedBase:
			case GameEventType::MissionCompletedBuildingAlien:
			case GameEventType::MissionCompletedBuildingNormal:
			case GameEventType::MissionCompletedBuildingRaid:
			case GameEventType::MissionCompletedVehicle:
			{
				// Never pause for these
				break;
			}
			default:
			{
				bool pause = false;
				if (GameEvent::optionsMap.find(gameEvent->type) != GameEvent::optionsMap.end())
				{
					pause = config().getBool(GameEvent::optionsMap.at(gameEvent->type));
				}
				if (pause)
				{
					fw().stageQueueCommand(
					    {StageCmd::Command::PUSH,
					     mksp<NotificationScreen>(state, *this, gameEvent->message(),
					                              gameEvent->type)});
				}
				break;
			}
		}
	}
	switch (gameEvent->type)
	{
		case GameEventType::AlienTakeover:
		{
			// FIXME: Proper takeover message
			auto gameOrgEvent = dynamic_cast<GameOrganisationEvent *>(e);
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH,
			     mksp<NotificationScreen>(state, *this, format("Aliens have taken over %s",
			                                                   gameOrgEvent->organisation->name),
			                              gameEvent->type)});
		}
		break;
		case GameEventType::DefendTheBase:
		{
			auto gameDefenseEvent = dynamic_cast<GameDefenseEvent *>(e);
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH, mksp<BaseDefenseScreen>(state, gameDefenseEvent->base,
			                                                      gameDefenseEvent->organisation)});
			break;
		}
		case GameEventType::UfoRecoveryBegin:
		{
			auto gameRecoveryEvent = dynamic_cast<GameVehicleEvent *>(e);
			if (gameRecoveryEvent->vehicle->type->battle_map)
			{
				setUpdateSpeed(CityUpdateSpeed::Pause);
				initiateUfoMission(gameRecoveryEvent->vehicle, gameRecoveryEvent->actor);
			}
			else
			{
				fw().pushEvent(new GameVehicleEvent(GameEventType::UfoRecoveryUnmanned,
				                                    gameRecoveryEvent->vehicle,
				                                    gameRecoveryEvent->actor));
				for (auto &u : gameRecoveryEvent->vehicle->type->researchUnlock)
				{
					u->forceComplete();
				}
			}
			break;
		}
		case GameEventType::UfoRecoveryUnmanned:
		{
			auto gameRecoveryEvent = dynamic_cast<GameVehicleEvent *>(e);
			LogWarning("Load unmanned ufo loot on craft!");
			// Remove ufo
			gameRecoveryEvent->vehicle->die(*state, true);
			// Return to base
			gameRecoveryEvent->actor->setMission(
			    *state, VehicleMission::gotoBuilding(*state, *gameRecoveryEvent->actor));
			break;
		}
		case GameEventType::AlienSpotted:
		{
			auto ev = dynamic_cast<GameBuildingEvent *>(e);
			if (!ev)
			{
				LogError("Invalid spotted event");
			}
			fw().soundBackend->playSample(
			    listRandomiser(state->rng, state->city_common_sample_list->alertSounds));
			zoomLastEvent();
			setUpdateSpeed(CityUpdateSpeed::Speed1);
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH, mksp<AlertScreen>(state, ev->building)});
			break;
		}
		case GameEventType::ResearchCompleted:
		{
			auto ev = dynamic_cast<GameResearchEvent *>(e);
			if (!ev)
			{
				LogError("Invalid research event");
			}
			this->state->research.resortTopicList();
			sp<Facility> lab_facility;
			for (auto &base : state->player_bases)
			{
				for (auto &facility : base.second->facilities)
				{
					if (ev->lab == facility->lab)
					{
						lab_facility = facility;
						break;
					}
				}
				if (lab_facility)
					break;
			}
			if (!lab_facility)
			{
				LogError("No facilities matching lab");
			}
			auto game_state = this->state;
			auto ufopaedia_entry = ev->topic->ufopaedia_entry;
			sp<UfopaediaCategory> ufopaedia_category;
			if (ufopaedia_entry)
			{
				for (auto &cat : this->state->ufopaedia)
				{
					for (auto &entry : cat.second->entries)
					{
						if (ufopaedia_entry == entry.second)
						{
							ufopaedia_category = cat.second;
							break;
						}
					}
					if (ufopaedia_category)
						break;
				}
				if (!ufopaedia_category)
				{
					LogError("No UFOPaedia category found for entry %s", ufopaedia_entry->title);
				}
			}
			auto message_box = mksp<MessageBox>(
			    tr("RESEARCH COMPLETE"),
			    format("%s\n%s\n%s", tr("Research project completed:"), ev->topic->name,
			           tr("Do you wish to view the UFOpaedia report?")),
			    MessageBox::ButtonOptions::YesNo,
			    // Yes callback
			    [game_state, lab_facility, ufopaedia_category, ufopaedia_entry]() {
				    fw().stageQueueCommand(
				        {StageCmd::Command::PUSH, mksp<ResearchScreen>(game_state, lab_facility)});
				    if (ufopaedia_entry)
				    {
					    fw().stageQueueCommand(
					        {StageCmd::Command::PUSH,
					         mksp<UfopaediaCategoryView>(game_state, ufopaedia_category,
					                                     ufopaedia_entry)});
				    }
				},
			    // No callback
			    [game_state, lab_facility]() {
				    fw().stageQueueCommand(
				        {StageCmd::Command::PUSH, mksp<ResearchScreen>(game_state, lab_facility)});
				});
			fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
		}
		break;
		case GameEventType::ManufactureCompleted:
		{
			auto ev = dynamic_cast<GameManufactureEvent *>(e);
			if (!ev)
			{
				LogError("Invalid manufacture event");
			}
			sp<Facility> lab_facility;
			sp<Base> lab_base;
			for (auto &base : state->player_bases)
			{
				for (auto &facility : base.second->facilities)
				{
					if (ev->lab == facility->lab)
					{
						lab_facility = facility;
						lab_base = base.second;
						break;
					}
				}
				if (lab_facility)
					break;
			}
			if (!lab_facility)
			{
				LogError("No facilities matching lab");
			}
			auto game_state = this->state;

			UString item_name;
			switch (ev->topic->item_type)
			{
				case ResearchTopic::ItemType::VehicleEquipment:
					item_name = game_state->vehicle_equipment[ev->topic->itemId]->name;
					break;
				case ResearchTopic::ItemType::VehicleEquipmentAmmo:
					item_name = game_state->vehicle_ammo[ev->topic->itemId]->name;
					break;
				case ResearchTopic::ItemType::AgentEquipment:
					item_name = game_state->agent_equipment[ev->topic->itemId]->name;
					break;
				case ResearchTopic::ItemType::Craft:
					item_name = game_state->vehicle_types[ev->topic->itemId]->name;
					break;
			}
			auto message_box = mksp<MessageBox>(
			    tr("MANUFACTURE COMPLETED"),
			    format("%s\n%s\n%s %d\n%d", lab_base->name, tr(item_name), tr("Quantity:"),
			           ev->goal, tr("Do you wish to reasign the Workshop?")),
			    MessageBox::ButtonOptions::YesNo,
			    // Yes callback
			    [game_state, lab_facility]() {
				    fw().stageQueueCommand(
				        {StageCmd::Command::PUSH, mksp<ResearchScreen>(game_state, lab_facility)});
				});
			fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
		}
		break;
		case GameEventType::ManufactureHalted:
		{
			auto ev = dynamic_cast<GameManufactureEvent *>(e);
			if (!ev)
			{
				LogError("Invalid manufacture event");
			}
			sp<Facility> lab_facility;
			sp<Base> lab_base;
			for (auto &base : state->player_bases)
			{
				for (auto &facility : base.second->facilities)
				{
					if (ev->lab == facility->lab)
					{
						lab_facility = facility;
						lab_base = base.second;
						break;
					}
				}
				if (lab_facility)
					break;
			}
			if (!lab_facility)
			{
				LogError("No facilities matching lab");
			}
			auto game_state = this->state;

			UString item_name;
			switch (ev->topic->item_type)
			{
				case ResearchTopic::ItemType::VehicleEquipment:
					item_name = game_state->vehicle_equipment[ev->topic->itemId]->name;
					break;
				case ResearchTopic::ItemType::VehicleEquipmentAmmo:
					item_name = game_state->vehicle_ammo[ev->topic->itemId]->name;
					break;
				case ResearchTopic::ItemType::AgentEquipment:
					item_name = game_state->agent_equipment[ev->topic->itemId]->name;
					break;
				case ResearchTopic::ItemType::Craft:
					item_name = game_state->vehicles[ev->topic->itemId]->name;
					break;
			}
			auto message_box =
			    mksp<MessageBox>(tr("MANUFACTURING HALTED"),
			                     format("%s\n%s\n%s %d/%d\n%d", lab_base->name, tr(item_name),
			                            tr("Completion status:"), ev->done, ev->goal,
			                            tr("Production costs exceed your available funds.")),
			                     MessageBox::ButtonOptions::Ok);
			fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
		}
		break;
		case GameEventType::FacilityCompleted:
		{
			auto ev = dynamic_cast<GameFacilityEvent *>(e);
			if (!ev)
			{
				LogError("Invalid facility event");
				return true;
			}
			auto message_box =
			    mksp<MessageBox>(tr("FACILITY COMPLETED"),
			                     format("%s\n%s", ev->base->name, tr(ev->facility->type->name)),
			                     MessageBox::ButtonOptions::Ok);
			fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
		}
		break;
		default:
			break;
	}
	return true;
}

void CityView::updateSelectedUnits()
{
	auto o = state->getPlayer();
	bool foundOwned = false;
	bool foundOwnedVehicle = false;
	// Vehicles
	{
		auto it = state->current_city->cityViewSelectedVehicles.begin();
		while (it != state->current_city->cityViewSelectedVehicles.end())
		{
			auto v = *it;
			if (!v || v->isDead())
			{
				it = state->current_city->cityViewSelectedVehicles.erase(it);
			}
			else
			{
				if (v->owner == o)
				{
					foundOwned = true;
					foundOwnedVehicle = true;
				}
				it++;
			}
		}
	}
	// Agents
	{
		auto it = state->current_city->cityViewSelectedAgents.begin();
		while (it != state->current_city->cityViewSelectedAgents.end())
		{
			auto a = *it;
			if (!a || a->isDead())
			{
				it = state->current_city->cityViewSelectedAgents.erase(it);
			}
			else
			{
				if (a->owner == o)
				{
					foundOwned = true;
				}
				it++;
			}
		}
	}
	if (!foundOwned && selectionState != CitySelectionState::Normal)
	{
		setSelectionState(CitySelectionState::Normal);
	}
	if (activeTab != uiTabs[1])
	{
		foundOwnedVehicle = false;
	}
	if (!foundOwnedVehicle && selectionState == CitySelectionState::ManualControl)
	{
		setSelectionState(CitySelectionState::Normal);
	}
}

void CityView::setUpdateSpeed(CityUpdateSpeed updateSpeed)
{
	if (this->updateSpeed == updateSpeed)
	{
		return;
	}
	this->lastSpeed = this->updateSpeed;
	switch (updateSpeed)
	{
		case CityUpdateSpeed::Pause:
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED0")->setChecked(true);
			break;
		case CityUpdateSpeed::Speed1:
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED1")->setChecked(true);
			break;
		case CityUpdateSpeed::Speed2:
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED2")->setChecked(true);
			break;
		case CityUpdateSpeed::Speed3:
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED3")->setChecked(true);
			break;
		case CityUpdateSpeed::Speed4:
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED4")->setChecked(true);
			break;
		case CityUpdateSpeed::Speed5:
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED5")->setChecked(true);
			break;
	}
}

void CityView::zoomLastEvent()
{
	if (!state->messages.empty())
	{
		auto message = state->messages.back();
		if (message.location != EventMessage::NO_LOCATION)
		{
			setScreenCenterTile(message.location);
		}
	}
}
void CityView::setSelectionState(CitySelectionState selectionState)
{
	this->selectionState = selectionState;
	switch (selectionState)
	{
		case CitySelectionState::Normal:
		{
			overlayTab->setVisible(false);
			break;
		}
		case CitySelectionState::AttackBuilding:
		{
			overlayTab->findControlTyped<Label>("TEXT")->setText(
			    tr("Click on building for selected vehicle to attack"));
			overlayTab->setVisible(true);
			break;
		}
		case CitySelectionState::AttackVehicle:
		{
			overlayTab->findControlTyped<Label>("TEXT")->setText(
			    tr("Click on target vehicle for selected vehicle"));
			overlayTab->setVisible(true);
			break;
		}
		case CitySelectionState::GotoBuilding:
		{
			UString message;
			if (activeTab == uiTabs[1])
			{
				if (state->current_city->cityViewSelectedVehicles.size() > 1)
				{
					message = tr("Click on destination building for selected vehicles");
				}
				else
				{
					message = tr("Click on destination building for selected vehicle");
				}
			}
			else
			{
				if (state->current_city->cityViewSelectedAgents.size() > 1)
				{
					message = tr("Click on destination building for selected people");
				}
				else
				{
					message = tr("Click on destination building for selected person");
				}
			}
			overlayTab->findControlTyped<Label>("TEXT")->setText(message);
			overlayTab->setVisible(true);
			break;
		}
		case CitySelectionState::GotoLocation:
		{
			overlayTab->findControlTyped<Label>("TEXT")->setText(
			    tr("Click on destination map point for selected vehicle"));
			overlayTab->setVisible(true);
			break;
		}
		case CitySelectionState::ManualControl:
		{
			overlayTab->findControlTyped<Label>("TEXT")->setText(tr("Manual control"));
			overlayTab->setVisible(true);
			break;
		}
	}
}
}; // namespace OpenApoc
