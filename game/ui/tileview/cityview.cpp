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
#include "framework/jukebox.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "framework/sound.h"
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
#include "game/state/city/vequipment.h"
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
#include "game/state/rules/city/vequipmenttype.h"
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
#include "game/ui/city/bribescreen.h"
#include "game/ui/city/buildingscreen.h"
#include "game/ui/city/diplomatictreatyscreen.h"
#include "game/ui/city/infiltrationscreen.h"
#include "game/ui/city/scorescreen.h"
#include "game/ui/city/weeklyfundingscreen.h"
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

std::shared_future<void> loadBattleBuilding(sp<GameState> state, sp<Building> building,
                                            bool hotseat, bool raid,
                                            std::list<StateRef<Agent>> playerAgents,
                                            StateRef<Vehicle> playerVehicle)
{
	auto loadTask = fw().threadPoolEnqueue(
	    [hotseat, building, state, raid, playerAgents, playerVehicle]() mutable -> void
	    {
		    StateRef<Organisation> org = raid ? building->owner : state->getAliens();
		    StateRef<Building> bld = {state.get(), building};

		    const std::map<StateRef<AgentType>, int> *aliens = nullptr;
		    const int *guards = nullptr;
		    const int *civilians = nullptr;

		    Battle::beginBattle(*state, hotseat, org, playerAgents, aliens, guards, civilians,
		                        playerVehicle, bld);
	    });
	return loadTask;
}

std::shared_future<void> loadBattleVehicle(sp<GameState> state, StateRef<Vehicle> ufo,
                                           StateRef<Vehicle> playerVehicle)
{

	auto loadTask = fw().threadPoolEnqueue(
	    [state, ufo, playerVehicle]() -> void
	    {
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

sp<Facility> findCurrentResearchFacility(sp<GameState> state, AgentType::Role role,
                                         FacilityType::Capacity capacity)
{
	sp<Facility> lab;
	if (!state->current_city->cityViewSelectedCivilians.empty())
	{
		for (auto &a : state->current_city->cityViewSelectedCivilians)
		{
			if (a && a->type->role == role)
			{
				state->current_base = a->homeBuilding->base;
				if (a->assigned_to_lab)
				{
					auto thisRef = StateRef<Agent>{state.get(), a};
					for (auto &fac : state->current_base->facilities)
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
					for (auto &f : state->current_base->facilities)
					{
						if (f->type->capacityType == capacity)
						{
							lab = f;
							break;
						}
					}
				}
				break;
			}
		}
		if (lab == nullptr)
		{
			for (auto &base : state->player_bases)
			{
				for (auto &facility : base.second->facilities)
				{
					if (facility->type->capacityType == capacity)
					{
						lab = facility;
						break;
					}
				}
				if (lab)
					break;
			}
		}
	}
	else
	{
		for (auto &f : state->current_base->facilities)
		{
			if (f->type->capacityType == capacity)
			{
				lab = f;
				break;
			}
			if (lab)
				break;
		}
	}
	return lab;
}

constexpr size_t NUM_TABS = 8;

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

	// [Alt] + [Shift] gives building orders (left = attack, right = move)
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
				// Base / location screen
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
				else
				{
					// Location (building) screen
					fw().stageQueueCommand(
					    {StageCmd::Command::PUSH, mksp<BuildingScreen>(this->state, building)});
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
                                    CitySelectionState selState, bool passThrough)
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
			default:
			{
				// Other selection states don't affect vehicles
				break;
			}
		}
		return true;
	}

	// [Alt] + [Shift] gives building orders (therefore do nothing)
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
			if (!passThrough)
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
			break;
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
		default:
		{
			// Other selection states don't affect vehicles
			break;
		}
	}
	return false;
}

bool CityView::handleClickedAgent(StateRef<Agent> agent, bool rightClick,
                                  CitySelectionState selState [[maybe_unused]])
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
                                         CitySelectionState selState [[maybe_unused]])
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

void CityView::showDailyReport()
{
	fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<ScoreScreen>(this->state)});
}

void CityView::showWeeklyFundingReport()
{
	fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<WeeklyFundingScreen>(this->state)});
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
		for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
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
				if (v->city.id == "CITYMAP_HUMAN")
				{
					v->setMission(*this->state,
					              VehicleMission::gotoBuilding(*this->state, *v, bld));
				}
				else
				{
					v->setMission(*this->state, VehicleMission::gotoPortal(*this->state, *v));
				}
			}
		}
		return;
	}
	if (activeTab == uiTabs[2])
	{
		for (auto &a : this->state->current_city->cityViewSelectedSoldiers)
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

void CityView::orderMove(Vec3<float> position, bool alternative, bool portal)
{
	bool useTeleporter =
	    alternative && config().getBool("OpenApoc.NewFeature.AllowManualCityTeleporters");
	if (activeTab == uiTabs[1])
	{
		if (portal)
		{
			for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
			{
				if (v && v->owner == this->state->getPlayer())
				{
					v->setMission(*state, VehicleMission::gotoPortal(*state, *v, position));
				}
			}
		}
		else
		{
			state->current_city->groupMove(*state,
			                               state->current_city->cityViewSelectedOwnedVehicles,
			                               position, useTeleporter);
		}
		return;
	}
}

void CityView::orderMove(StateRef<Building> building, bool alternative)
{
	bool useTeleporter =
	    alternative && config().getBool("OpenApoc.NewFeature.AllowManualCityTeleporters");
	if (activeTab == uiTabs[1])
	{
		for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
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
		bool useTaxi = !alternative && config().getBool("OpenApoc.NewFeature.AllowSoldierTaxiUse");
		for (auto &a : this->state->current_city->cityViewSelectedSoldiers)
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
/* if (agent->type->role == AgentType::Role::Soldier)
{
    auto pos = std::find(state->current_city->cityViewSelectedSoldiers.begin(),
                         state->current_city->cityViewSelectedSoldiers.end(), agent);
    if (inverse)
    {
        // Agent in selection => remove
        if (pos != state->current_city->cityViewSelectedSoldiers.end())
        {
            state->current_city->cityViewSelectedSoldiers.erase(pos);
        }
    }
    else
    {
        // Agent not selected
        if (pos == state->current_city->cityViewSelectedSoldiers.end())
        {
            // If additive add
            if (additive)
            {
                state->current_city->cityViewSelectedSoldiers.push_front(agent);
            }
            else
            {
                // Agent not in selection => replace selection with agent
                state->current_city->cityViewSelectedSoldiers.clear();
                state->current_city->cityViewSelectedSoldiers.push_back(agent);
            }
        }
        // Agent is selected
        else
        {
            // First move vehicle to front
            state->current_city->cityViewSelectedSoldiers.erase(pos);
            state->current_city->cityViewSelectedSoldiers.push_front(agent);
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
    }*/
void CityView::orderSelect(StateRef<Vehicle> vehicle, bool inverse, bool additive)
{
	if (vehicle->owner == state->getPlayer())
	{
		auto pos = std::find(state->current_city->cityViewSelectedOwnedVehicles.begin(),
		                     state->current_city->cityViewSelectedOwnedVehicles.end(), vehicle);

		if (vehicle->city != state->current_city)
		{
			return;
		}
		if (inverse)
		{
			// Vehicle in selection => remove
			if (pos != state->current_city->cityViewSelectedOwnedVehicles.end())
			{
				state->current_city->cityViewSelectedOwnedVehicles.erase(pos);
			}
		}
		else
		{
			// Vehicle not selected
			if (pos == state->current_city->cityViewSelectedOwnedVehicles.end())
			{
				// Selecting non-owned vehicles is always additive to current selection
				if (additive)
				{
					state->current_city->cityViewSelectedOwnedVehicles.push_front(vehicle);
				}
				else
				{
					// Vehicle not in selection => replace selection with vehicle
					state->current_city->cityViewSelectedOwnedVehicles.clear();
					state->current_city->cityViewSelectedOwnedVehicles.push_back(vehicle);
				}
			}
			// Vehicle is selected
			else
			{
				// First move vehicle to front
				state->current_city->cityViewSelectedOwnedVehicles.erase(pos);
				state->current_city->cityViewSelectedOwnedVehicles.push_front(vehicle);

				// Then if not additive then zoom to vehicle
				if (!additive)
				{
					this->setScreenCenterTile(vehicle->position);
					setSelectedTab(1);
				}
			}
		}
	}
	else
	{
		auto pos = std::find(state->current_city->cityViewSelectedOtherVehicles.begin(),
		                     state->current_city->cityViewSelectedOtherVehicles.end(), vehicle);

		if (vehicle->city != state->current_city)
		{
			return;
		}
		if (inverse)
		{
			// Vehicle in selection => remove
			if (pos != state->current_city->cityViewSelectedOtherVehicles.end())
			{
				state->current_city->cityViewSelectedOtherVehicles.erase(pos);
			}
		}
		else
		{
			// Vehicle not selected
			if (pos == state->current_city->cityViewSelectedOtherVehicles.end())
			{
				// Selecting non-owned vehicles is always additive to current selection
				if (additive)
				{
					state->current_city->cityViewSelectedOtherVehicles.push_front(vehicle);
				}
				else
				{
					// Vehicle not in selection => replace selection with vehicle
					state->current_city->cityViewSelectedOtherVehicles.clear();
					state->current_city->cityViewSelectedOtherVehicles.push_back(vehicle);
				}
			}
			// Vehicle is selected
			else
			{
				// First move vehicle to front
				state->current_city->cityViewSelectedOtherVehicles.erase(pos);
				state->current_city->cityViewSelectedOtherVehicles.push_front(vehicle);

				// Then if not additive then zoom to vehicle
				if (!additive)
				{
					this->setScreenCenterTile(vehicle->position);
					setSelectedTab(6);
				}
			}
		}
	}
	if (state->current_city->cityViewSelectedOwnedVehicles.empty())
	{
		return;
	}
}

void CityView::orderSelect(StateRef<Agent> agent, bool inverse, bool additive)
{
	if (agent->type->role == AgentType::Role::Soldier)
	{
		auto pos = std::find(state->current_city->cityViewSelectedSoldiers.begin(),
		                     state->current_city->cityViewSelectedSoldiers.end(), agent);
		if (inverse)
		{
			// Agent in selection => remove
			if (pos != state->current_city->cityViewSelectedSoldiers.end())
			{
				state->current_city->cityViewSelectedSoldiers.erase(pos);
			}
		}
		else
		{
			// Agent not selected
			if (pos == state->current_city->cityViewSelectedSoldiers.end())
			{
				// If additive add
				if (additive)
				{
					state->current_city->cityViewSelectedSoldiers.push_front(agent);
				}
				else
				{
					// Agent not in selection => replace selection with agent
					state->current_city->cityViewSelectedSoldiers.clear();
					state->current_city->cityViewSelectedSoldiers.push_back(agent);
				}
			}
			// Agent is selected
			else
			{
				// First move vehicle to front
				state->current_city->cityViewSelectedSoldiers.erase(pos);
				state->current_city->cityViewSelectedSoldiers.push_front(agent);
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
		auto agentForm = this->uiTabs[2];
		if (state->current_city->cityViewSelectedSoldiers.empty())
		{
			agentForm->findControlTyped<CheckBox>("BUTTON_AGENT_PHYSICAL")->setChecked(false);
			agentForm->findControlTyped<CheckBox>("BUTTON_AGENT_PSI")->setChecked(false);
			return;
		}
		agent = state->current_city->cityViewSelectedSoldiers.front();
		switch (agent->trainingAssignment)
		{
			case TrainingAssignment::None:
				agentForm->findControlTyped<CheckBox>("BUTTON_AGENT_PHYSICAL")->setChecked(false);
				agentForm->findControlTyped<CheckBox>("BUTTON_AGENT_PSI")->setChecked(false);
				break;
			case TrainingAssignment::Physical:
			{
				agentForm->findControlTyped<CheckBox>("BUTTON_AGENT_PHYSICAL")->setChecked(true);
				agentForm->findControlTyped<CheckBox>("BUTTON_AGENT_PSI")->setChecked(false);
				break;
			}
			case TrainingAssignment::Psi:
			{
				agentForm->findControlTyped<CheckBox>("BUTTON_AGENT_PHYSICAL")->setChecked(false);
				agentForm->findControlTyped<CheckBox>("BUTTON_AGENT_PSI")->setChecked(true);
				break;
			}
		}
	}
	// Biochem
	if (agent->type->role == AgentType::Role::BioChemist)
	{
		auto pos = std::find(state->current_city->cityViewSelectedBios.begin(),
		                     state->current_city->cityViewSelectedBios.end(), agent);
		if (inverse)
		{
			// Agent in selection => remove
			if (pos != state->current_city->cityViewSelectedBios.end())
			{
				state->current_city->cityViewSelectedBios.erase(pos);
			}
		}
		else
		{
			// Agent not selected
			if (pos == state->current_city->cityViewSelectedBios.end())
			{
				// If additive add
				if (additive)
				{
					state->current_city->cityViewSelectedBios.push_front(agent);
				}
				else
				{
					// Agent not in selection => replace selection with agent
					state->current_city->cityViewSelectedBios.clear();
					state->current_city->cityViewSelectedBios.push_back(agent);
				}
			}
			// Agent is selected
			else
			{
				// First move agent to front
				state->current_city->cityViewSelectedBios.erase(pos);
				state->current_city->cityViewSelectedBios.push_front(agent);
				// Then if not additive then zoom to agent
				if (!additive)
				{
					this->setScreenCenterTile(agent->position);
				}
			}
		}
	}
	// Physics
	if (agent->type->role == AgentType::Role::Physicist)
	{
		auto pos = std::find(state->current_city->cityViewSelectedPhysics.begin(),
		                     state->current_city->cityViewSelectedPhysics.end(), agent);
		if (inverse)
		{
			// Agent in selection => remove
			if (pos != state->current_city->cityViewSelectedPhysics.end())
			{
				state->current_city->cityViewSelectedPhysics.erase(pos);
			}
		}
		else
		{
			// Agent not selected
			if (pos == state->current_city->cityViewSelectedPhysics.end())
			{
				// If additive add
				if (additive)
				{
					state->current_city->cityViewSelectedPhysics.push_front(agent);
				}
				else
				{
					// Agent not in selection => replace selection with agent
					state->current_city->cityViewSelectedPhysics.clear();
					state->current_city->cityViewSelectedPhysics.push_back(agent);
				}
			}
			// Agent is selected
			else
			{
				// First move agent to front
				state->current_city->cityViewSelectedPhysics.erase(pos);
				state->current_city->cityViewSelectedPhysics.push_front(agent);
				// Then if not additive then zoom to agent
				if (!additive)
				{
					this->setScreenCenterTile(agent->position);
				}
			}
		}
	}
	// Engineers
	if (agent->type->role == AgentType::Role::Engineer)
	{
		auto pos = std::find(state->current_city->cityViewSelectedEngineers.begin(),
		                     state->current_city->cityViewSelectedEngineers.end(), agent);
		if (inverse)
		{
			// Agent in selection => remove
			if (pos != state->current_city->cityViewSelectedEngineers.end())
			{
				state->current_city->cityViewSelectedEngineers.erase(pos);
			}
		}
		else
		{
			// Agent not selected
			if (pos == state->current_city->cityViewSelectedEngineers.end())
			{
				// If additive add
				if (additive)
				{
					state->current_city->cityViewSelectedEngineers.push_front(agent);
				}
				else
				{
					// Agent not in selection => replace selection with agent
					state->current_city->cityViewSelectedEngineers.clear();
					state->current_city->cityViewSelectedEngineers.push_back(agent);
				}
			}
			// Agent is selected
			else
			{
				// First move agent to front
				state->current_city->cityViewSelectedEngineers.erase(pos);
				state->current_city->cityViewSelectedEngineers.push_front(agent);
				// Then if not additive then zoom to agent
				if (!additive)
				{
					this->setScreenCenterTile(agent->position);
				}
			}
		}
	}
	// Build civilian list
	state->current_city->cityViewSelectedCivilians.clear();

	for (auto &a : state->current_city->cityViewSelectedBios)
	{
		state->current_city->cityViewSelectedCivilians.push_back(a);
	}
	for (auto &a : state->current_city->cityViewSelectedPhysics)
	{
		state->current_city->cityViewSelectedCivilians.push_back(a);
	}
	for (auto &a : state->current_city->cityViewSelectedEngineers)
	{
		state->current_city->cityViewSelectedCivilians.push_back(a);
	}
}

void CityView::orderFire(Vec3<float> position)
{
	if (activeTab == uiTabs[1])
	{
		for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
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
		for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
		{
			if (v && v->owner == this->state->getPlayer() && v != vehicle)
			{
				if (forced)
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
				else if (vehicle->crashed || vehicle->falling || vehicle->sliding)
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
				else
				{
					if (vehicle->owner == state->getPlayer())
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
			}
		}
		return;
	}
}

void CityView::orderFollow(StateRef<Vehicle> vehicle)
{
	if (activeTab == uiTabs[1])
	{
		for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
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
		for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
		{
			if (v && v->owner == this->state->getPlayer())
			{
				v->setMission(*state, VehicleMission::attackBuilding(*this->state, *v, building));
			}
		}
	}
}

void CityView::orderDisableWeapon(int index, bool disable)
{
	weaponDisabled[index] = disable;

	std::vector<sp<VEquipment>> currentWeapons;
	if (!state->current_city->cityViewSelectedOwnedVehicles.empty())
	{
		auto vehicle = state->current_city->cityViewSelectedOwnedVehicles.front();
		if (vehicle->owner != state->getPlayer())
		{
			if (state->current_city->cityViewSelectedOwnedVehicles.size() > 1)
			{
				vehicle = *++state->current_city->cityViewSelectedOwnedVehicles.begin();
			}
			else
			{
				vehicle = nullptr;
			}
		}
		if (vehicle)
		{
			for (auto &e : vehicle->equipment)
			{
				if (e->type->type == EquipmentSlotType::VehicleWeapon)
				{
					currentWeapons.push_back(e);
				}
			}
		}
	}
	if (currentWeapons.size() > index)
	{
		currentWeapons[index]->disabled = disable;
	}
}

void CityView::setSelectedTab(int tabIndex)
{
	if (tabIndex < 0 || tabIndex > uiTabs.size())
	{
		LogError("Trying to select invalid tab: %d", tabIndex);
		return;
	}
	for (auto tab : uiTabs)
	{
		tab->setVisible(false);
	}
	uiTabs[tabIndex]->setVisible(true);
	this->activeTab = uiTabs[tabIndex];
	this->state->current_city->cityViewPageIndex = tabIndex;
}

CityView::CityView(sp<GameState> state)
    : CityTileView(*state->current_city->map, Vec3<int>{TILE_X_CITY, TILE_Y_CITY, TILE_Z_CITY},
                   Vec2<int>{STRAT_TILE_X, STRAT_TILE_Y}, TileViewMode::Isometric,
                   state->current_city->cityViewScreenCenter, *state),
      baseForm(ui().getForm("city/city")), overlayTab(ui().getForm("city/overlay")),
      debugOverlay(ui().getForm("city/debugoverlay_city")), updateSpeed(CityUpdateSpeed::Speed1),
      lastSpeed(CityUpdateSpeed::Pause), state(state), followVehicle(false),
      selectionState(CitySelectionState::Normal)
{
	weaponType.resize(3);
	weaponDisabled.resize(3, false);
	weaponAmmo.resize(3, -1);

	overlayTab->setVisible(false);
	overlayTab->findControlTyped<GraphicButton>("BUTTON_CLOSE")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { setSelectionState(CitySelectionState::Normal); });
	baseForm->findControlTyped<RadioButton>("BUTTON_SPEED1")->setChecked(true);
	for (size_t i = 0; i < NUM_TABS; ++i)
	{
		sp<Form> f = baseForm->findControlTyped<Form>(format("SUBFORM_TAB_%d", i + 1));
		f->takesFocus = false;
		this->uiTabs.push_back(f);
	}
	setSelectedTab(this->state->current_city->cityViewPageIndex);

	// Refresh base views
	resume();

	for (size_t i = 0; i < this->uiTabs.size(); ++i)
	{
		this->baseForm->findControl(format("BUTTON_TAB_%d", i + 1))
		    ->addCallback(FormEventType::ButtonClick,
		                  [this, i](Event *)
		                  {
			                  this->setSelectedTab(i);
			                  this->update();
		                  });
	}
	this->baseForm->findControl("BUTTON_FOLLOW_VEHICLE")
	    ->addCallback(FormEventType::CheckBoxChange,
	                  [this](FormsEvent *e) {
		                  this->followVehicle =
		                      std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy)->isChecked();
	                  });
	this->baseForm->findControl("BUTTON_TOGGLE_STRATMAP")
	    ->addCallback(FormEventType::CheckBoxChange,
	                  [this](FormsEvent *e)
	                  {
		                  bool strategy =
		                      std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy)->isChecked();
		                  this->setViewMode(strategy ? TileViewMode::Strategy
		                                             : TileViewMode::Isometric);
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
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) {
		                  fw().stageQueueCommand(
		                      {StageCmd::Command::PUSH, mksp<InfiltrationScreen>(this->state)});
	                  });
	this->baseForm->findControl("BUTTON_SHOW_SCORE")
	    ->addCallback(
	        FormEventType::ButtonClick,
	        [this](Event *) {
		        fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<ScoreScreen>(this->state)});
	        });
	this->baseForm->findControl("BUTTON_SHOW_UFOPAEDIA")
	    ->addCallback(
	        FormEventType::ButtonClick,
	        [this](Event *) {
		        fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<UfopaediaView>(this->state)});
	        });
	this->baseForm->findControl("BUTTON_SHOW_OPTIONS")
	    ->addCallback(
	        FormEventType::ButtonClick,
	        [this](Event *) {
		        fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<InGameOptions>(this->state)});
	        });
	this->baseForm->findControl("BUTTON_SHOW_LOG")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) {
		                  fw().stageQueueCommand({StageCmd::Command::PUSH,
		                                          mksp<MessageLogScreen>(this->state, *this)});
	                  });
	this->baseForm->findControl("BUTTON_ZOOM_EVENT")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *)
	                  {
		                  if (baseForm->findControlTyped<Ticker>("NEWS_TICKER")->hasMessages())
		                  {
			                  this->zoomLastEvent();
		                  }
	                  });

	auto baseManagementForm = this->uiTabs[0];
	baseManagementForm->findControl("BUTTON_SHOW_BASE")
	    ->addCallback(
	        FormEventType::ButtonClick,
	        [this](Event *) {
		        fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<BaseScreen>(this->state)});
	        });
	baseManagementForm->findControl("BUTTON_BUILD_BASE")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *)
	                  {
		                  fw().stageQueueCommand(
		                      {StageCmd::Command::PUSH,
		                       mksp<BaseSelectScreen>(this->state, this->centerPos)});
	                  });
	auto vehicleForm = this->uiTabs[1];
	for (int i = 0; i < weaponDisabled.size(); i++)
	{
		vehicleForm->findControlTyped<CheckBox>(format("VEHICLE_WEAPON_%d_DISABLED", i + 1))
		    ->addCallback(FormEventType::CheckBoxSelected, [this, i](FormsEvent *e [[maybe_unused]])
		                  { orderDisableWeapon(i, true); });
		vehicleForm->findControlTyped<CheckBox>(format("VEHICLE_WEAPON_%d_DISABLED", i + 1))
		    ->addCallback(FormEventType::CheckBoxDeSelected,
		                  [this, i](FormsEvent *e [[maybe_unused]])
		                  { orderDisableWeapon(i, false); });
	}
	vehicleForm->findControl("BUTTON_EQUIP_VEHICLE")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *)
	                  {
		                  bool playerHasVehicles = false;
		                  for (auto &v : this->state->vehicles)
		                  {
			                  auto vehicle = v.second;
			                  if (vehicle->owner == this->state->getPlayer())
			                  {
				                  playerHasVehicles = true;
				                  break;
			                  }
		                  }
		                  // avoid attempting to open vehicle equip screen if player has no vehicles
		                  if (playerHasVehicles)
		                  {
			                  auto equipScreen = mksp<VEquipScreen>(this->state);
			                  for (auto &v :
			                       this->state->current_city->cityViewSelectedOwnedVehicles)
			                  {
				                  if (v && v->owner == this->state->getPlayer())
				                  {
					                  equipScreen->setSelectedVehicle(v);
					                  break;
				                  }
			                  }
			                  fw().stageQueueCommand({StageCmd::Command::PUSH, equipScreen});
		                  }
	                  });
	vehicleForm->findControl("BUTTON_VEHICLE_BUILDING")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *)
	                  {
		                  for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
		                  {
			                  if (v && v->owner == this->state->getPlayer())
			                  {
				                  fw().stageQueueCommand({StageCmd::Command::PUSH,
				                                          mksp<LocationScreen>(this->state, v)});
				                  break;
			                  }
		                  }
	                  });
	vehicleForm->findControl("BUTTON_GOTO_BUILDING")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *)
	                  {
		                  for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
		                  {
			                  if (v && v->owner == this->state->getPlayer())
			                  {
				                  setSelectionState(CitySelectionState::GotoBuilding);
				                  break;
			                  }
		                  }
	                  });
	vehicleForm->findControl("BUTTON_GOTO_LOCATION")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *)
	                  {
		                  for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
		                  {
			                  if (v && v->owner == this->state->getPlayer())
			                  {
				                  setSelectionState(CitySelectionState::GotoLocation);
				                  break;
			                  }
		                  }
	                  });
	vehicleForm->findControl("BUTTON_VEHICLE_ATTACK")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *)
	                  {
		                  for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
		                  {
			                  if (v && v->owner == this->state->getPlayer())
			                  {
				                  setSelectionState(CitySelectionState::AttackVehicle);
				                  break;
			                  }
		                  }
	                  });
	vehicleForm->findControl("BUTTON_VEHICLE_ATTACK_BUILDING")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *)
	                  {
		                  for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
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
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *)
	                  {
		                  for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
		                  {
			                  if (v && v->owner == this->state->getPlayer())
			                  {
				                  v->attackMode = Vehicle::AttackMode::Aggressive;
			                  }
		                  }
	                  });
	vehicleForm->findControl("BUTTON_ATTACK_MODE_STANDARD")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *)
	                  {
		                  for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
		                  {
			                  if (v && v->owner == this->state->getPlayer())
			                  {
				                  v->attackMode = Vehicle::AttackMode::Standard;
			                  }
		                  }
	                  });
	vehicleForm->findControl("BUTTON_ATTACK_MODE_DEFENSIVE")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *)
	                  {
		                  for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
		                  {
			                  if (v && v->owner == this->state->getPlayer())
			                  {
				                  v->attackMode = Vehicle::AttackMode::Defensive;
			                  }
		                  }
	                  });
	vehicleForm->findControl("BUTTON_ATTACK_MODE_EVASIVE")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *)
	                  {
		                  for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
		                  {
			                  if (v && v->owner == this->state->getPlayer())
			                  {
				                  v->attackMode = Vehicle::AttackMode::Evasive;
			                  }
		                  }
	                  });
	vehicleForm->findControl("BUTTON_ALTITUDE_HIGHEST")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *)
	                  {
		                  for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
		                  {
			                  if (v && v->owner == this->state->getPlayer())
			                  {
				                  v->altitude = Vehicle::Altitude::Highest;
			                  }
		                  }
	                  });
	vehicleForm->findControl("BUTTON_ALTITUDE_HIGH")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *)
	                  {
		                  for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
		                  {
			                  if (v && v->owner == this->state->getPlayer())
			                  {
				                  v->altitude = Vehicle::Altitude::High;
			                  }
		                  }
	                  });
	vehicleForm->findControl("BUTTON_ALTITUDE_STANDARD")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *)
	                  {
		                  for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
		                  {
			                  if (v && v->owner == this->state->getPlayer())
			                  {
				                  v->altitude = Vehicle::Altitude::Standard;
			                  }
		                  }
	                  });
	vehicleForm->findControl("BUTTON_ALTITUDE_LOW")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *)
	                  {
		                  for (auto &v : this->state->current_city->cityViewSelectedOwnedVehicles)
		                  {
			                  if (v && v->owner == this->state->getPlayer())
			                  {
				                  v->altitude = Vehicle::Altitude::Low;
			                  }
		                  }
	                  });
	auto agentForm = this->uiTabs[2];
	agentForm->findControl("BUTTON_AGENT_BUILDING")
	    ->addCallback(
	        FormEventType::ButtonClick,
	        [this](Event *)
	        {
		        if (!this->state->current_city->cityViewSelectedSoldiers.empty())
		        {
			        fw().stageQueueCommand(
			            {StageCmd::Command::PUSH,
			             mksp<LocationScreen>(
			                 this->state,
			                 this->state->current_city->cityViewSelectedSoldiers.front())});
		        }
	        });
	agentForm->findControl("BUTTON_EQUIP_AGENT")
	    ->addCallback(
	        FormEventType::ButtonClick,
	        [this](Event *)
	        {
		        bool playerHasSoldiers = false;
		        for (auto &a : this->state->agents)
		        {
			        auto agent = a.second;
			        if (agent->owner == this->state->getPlayer() &&
			            agent->type->role == AgentType::Role::Soldier)
			        {
				        playerHasSoldiers = true;
				        break;
			        }
		        }
		        // avoid attempting to open agent equip screen if player has no agents
		        if (playerHasSoldiers)
		        {
			        fw().stageQueueCommand(
			            {StageCmd::Command::PUSH,
			             mksp<AEquipScreen>(
			                 this->state,
			                 !this->state->current_city->cityViewSelectedSoldiers.empty()
			                     ? this->state->current_city->cityViewSelectedSoldiers.front()
			                     : nullptr)});
		        }
	        });

	agentForm->findControl("BUTTON_GOTO_BUILDING")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *)
	                  {
		                  if (!this->state->current_city->cityViewSelectedSoldiers.empty())
		                  {
			                  setSelectionState(CitySelectionState::GotoBuilding);
		                  }
	                  });
	agentForm->findControl("BUTTON_GOTO_BASE")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *)
	                  {
		                  if (!this->state->current_city->cityViewSelectedSoldiers.empty())
		                  {

			                  auto a = this->state->current_city->cityViewSelectedSoldiers.front();
			                  // Don't return to base if in a vehicle
			                  if (!a->currentVehicle)
			                  {
				                  orderGoToBase();
			                  }
		                  }
	                  });
	agentForm->findControl("BUTTON_AGENT_PHYSICAL")
	    ->addCallback(FormEventType::MouseClick,
	                  [this](FormsEvent *e)
	                  {
		                  auto checkBox = std::static_pointer_cast<CheckBox>(e->forms().RaisedBy);
		                  bool checked = !checkBox->isChecked();
		                  if (checked)
		                  {
			                  this->uiTabs[2]
			                      ->findControlTyped<CheckBox>("BUTTON_AGENT_PSI")
			                      ->setChecked(false);
		                  }
		                  for (auto &a : this->state->current_city->cityViewSelectedSoldiers)
		                  {
			                  if (checked)
			                  {
				                  a->assignTraining(TrainingAssignment::Physical);
			                  }
			                  else if (a->trainingAssignment == TrainingAssignment::Physical)
			                  {
				                  a->assignTraining(TrainingAssignment::None);
			                  }
		                  }
		                  // Uncheck button if need to
		                  if (checked)
		                  {
			                  checked = false;
			                  for (auto &a : this->state->current_city->cityViewSelectedSoldiers)
			                  {
				                  if (a->trainingAssignment != TrainingAssignment::None)
				                  {
					                  checked = true;
					                  break;
				                  }
			                  }
			                  if (!checked)
			                  {
				                  checkBox->setChecked(!checkBox->isChecked());
			                  }
		                  }
		                  // Check other button if need to
		                  else
		                  {
			                  for (auto &a : this->state->current_city->cityViewSelectedSoldiers)
			                  {
				                  if (a->trainingAssignment == TrainingAssignment::Psi)
				                  {
					                  checked = true;
					                  break;
				                  }
			                  }
			                  if (checked)
			                  {
				                  this->uiTabs[2]
				                      ->findControlTyped<CheckBox>("BUTTON_AGENT_PSI")
				                      ->setChecked(true);
			                  }
		                  }
	                  });
	agentForm->findControl("BUTTON_AGENT_PSI")
	    ->addCallback(FormEventType::MouseClick,
	                  [this](FormsEvent *e)
	                  {
		                  auto checkBox = std::static_pointer_cast<CheckBox>(e->forms().RaisedBy);
		                  bool checked = !checkBox->isChecked();
		                  if (checked)
		                  {
			                  this->uiTabs[2]
			                      ->findControlTyped<CheckBox>("BUTTON_AGENT_PHYSICAL")
			                      ->setChecked(false);
		                  }
		                  for (auto &a : this->state->current_city->cityViewSelectedSoldiers)
		                  {
			                  if (checked)
			                  {
				                  a->assignTraining(TrainingAssignment::Psi);
			                  }
			                  else if (a->trainingAssignment == TrainingAssignment::Psi)
			                  {
				                  a->assignTraining(TrainingAssignment::None);
			                  }
		                  }
		                  // Uncheck button if need to
		                  if (checked)
		                  {
			                  checked = false;
			                  for (auto &a : this->state->current_city->cityViewSelectedSoldiers)
			                  {
				                  if (a->trainingAssignment != TrainingAssignment::None)
				                  {
					                  checked = true;
					                  break;
				                  }
			                  }
			                  if (!checked)
			                  {
				                  checkBox->setChecked(!checkBox->isChecked());
			                  }
		                  }
		                  // Check other button if need to
		                  else
		                  {
			                  for (auto &a : this->state->current_city->cityViewSelectedSoldiers)
			                  {
				                  if (a->trainingAssignment == TrainingAssignment::Physical)
				                  {
					                  checked = true;
					                  break;
				                  }
			                  }
			                  if (checked)
			                  {
				                  this->uiTabs[2]
				                      ->findControlTyped<CheckBox>("BUTTON_AGENT_PHYSICAL")
				                      ->setChecked(true);
			                  }
		                  }
	                  });

	this->uiTabs[3]
	    ->findControl("BUTTON_RESEARCH")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *)
	                  {
		                  sp<Facility> lab =
		                      findCurrentResearchFacility(this->state, AgentType::Role::BioChemist,
		                                                  FacilityType::Capacity::Chemistry);
		                  fw().stageQueueCommand(
		                      {StageCmd::Command::PUSH, mksp<ResearchScreen>(this->state, lab)});
	                  });
	this->uiTabs[4]
	    ->findControl("BUTTON_RESEARCH")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *)
	                  {
		                  sp<Facility> lab =
		                      findCurrentResearchFacility(this->state, AgentType::Role::Engineer,
		                                                  FacilityType::Capacity::Workshop);
		                  fw().stageQueueCommand(
		                      {StageCmd::Command::PUSH, mksp<ResearchScreen>(this->state, lab)});
	                  });
	this->uiTabs[5]
	    ->findControl("BUTTON_RESEARCH")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *)
	                  {
		                  sp<Facility> lab =
		                      findCurrentResearchFacility(this->state, AgentType::Role::Physicist,
		                                                  FacilityType::Capacity::Physics);
		                  fw().stageQueueCommand(
		                      {StageCmd::Command::PUSH, mksp<ResearchScreen>(this->state, lab)});
	                  });

	this->uiTabs[7]
	    ->findControl("BUTTON_SHOW_ALL")
	    ->addCallback(
	        FormEventType::CheckBoxSelected,
	        [this](Event *)
	        {
		        this->state->current_city->cityViewOrgButtonIndex = 0;
		        uiTabs[7]->findControlTyped<ListBox>("ORGANISATION_LIST")->scroller->setValue(0);
		        this->update();
	        });
	this->uiTabs[7]
	    ->findControl("BUTTON_SHOW_ALLIED")
	    ->addCallback(
	        FormEventType::CheckBoxSelected,
	        [this](Event *)
	        {
		        this->state->current_city->cityViewOrgButtonIndex = 1;
		        uiTabs[7]->findControlTyped<ListBox>("ORGANISATION_LIST")->scroller->setValue(0);
		        this->update();
	        });
	this->uiTabs[7]
	    ->findControl("BUTTON_SHOW_FRIENDLY")
	    ->addCallback(
	        FormEventType::CheckBoxSelected,
	        [this](Event *)
	        {
		        this->state->current_city->cityViewOrgButtonIndex = 2;
		        uiTabs[7]->findControlTyped<ListBox>("ORGANISATION_LIST")->scroller->setValue(0);
		        this->update();
	        });
	this->uiTabs[7]
	    ->findControl("BUTTON_SHOW_NEUTRAL")
	    ->addCallback(
	        FormEventType::CheckBoxSelected,
	        [this](Event *)
	        {
		        this->state->current_city->cityViewOrgButtonIndex = 3;
		        uiTabs[7]->findControlTyped<ListBox>("ORGANISATION_LIST")->scroller->setValue(0);
		        this->update();
	        });
	this->uiTabs[7]
	    ->findControl("BUTTON_SHOW_UNFRIENDLY")
	    ->addCallback(
	        FormEventType::CheckBoxSelected,
	        [this](Event *)
	        {
		        this->state->current_city->cityViewOrgButtonIndex = 4;
		        uiTabs[7]->findControlTyped<ListBox>("ORGANISATION_LIST")->scroller->setValue(0);
		        this->update();
	        });
	this->uiTabs[7]
	    ->findControl("BUTTON_SHOW_HOSTILE")
	    ->addCallback(
	        FormEventType::CheckBoxSelected,
	        [this](Event *)
	        {
		        this->state->current_city->cityViewOrgButtonIndex = 5;
		        uiTabs[7]->findControlTyped<ListBox>("ORGANISATION_LIST")->scroller->setValue(0);
		        this->update();
	        });
	this->uiTabs[7]
	    ->findControl("BUTTON_BRIBE")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *)
	                  {
		                  if (this->state->current_city->cityViewSelectedOrganisation)
		                  {
			                  fw().stageQueueCommand(
			                      {StageCmd::Command::PUSH, mksp<BribeScreen>(this->state)});
		                  }
	                  });

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
	if (state->current_city.id == "CITYMAP_ALIEN")
	{

		fw().jukebox->play(JukeBox::PlayList::Alien);
	}
	else
	{
		fw().jukebox->play(JukeBox::PlayList::City);
	}
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
	state->skipTurboCalculations = config().getBool("OpenApoc.NewFeature.SkipTurboMovement");
	CityTileView::resume();
	modifierLAlt = false;
	modifierLCtrl = false;
	modifierLShift = false;
	modifierRAlt = false;
	modifierRCtrl = false;
	modifierRShift = false;

	this->uiTabs[0]->findControlTyped<Label>("TEXT_BASE_NAME")->setText(state->current_base->name);

	refreshBaseView();
}

void CityView::refreshBaseView()
{
	if (miniViews.size() != state->player_bases.size())
	{
		this->uiTabs[0]
		    ->findControlTyped<Label>("TEXT_BASE_NAME")
		    ->setText(state->current_base->name);
		for (auto view : miniViews)
		{
			view->setData(nullptr);
			view->setImage(nullptr);
			view->setDepressedImage(nullptr);
			view->ToolTipText = "";
			view->setVisible(false);
		}

		miniViews.clear();
	}

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
		view->setVisible(true);
		view->setData(viewBase);
		auto viewImage = BaseGraphics::drawMiniBase(*viewBase);
		view->setImage(viewImage);
		view->setDepressedImage(viewImage);
		view->ToolTipText = viewBase->name;
		view->addCallback(FormEventType::ButtonClick,
		                  [this](FormsEvent *e)
		                  {
			                  auto clickedBase = StateRef<Base>(
			                      this->state.get(), e->forms().RaisedBy->getData<Base>());
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
		if (DEBUG_SHOW_VEHICLE_PATH || DEBUG_SHOW_VEHICLE_TARGETS)
		{
			static const Colour groundColor = {255, 255, 64, 255};
			static const Colour flyingColor = {64, 255, 255, 255};
			static const Colour targetXCOMColor = {0, 255, 0, 255};
			static const Colour targetOtherColor = {255, 0, 0, 255};
			static const Colour targetMutualColor = {255, 240, 0, 255};
			for (auto &pair : state->vehicles)
			{
				auto v = pair.second;
				if (DEBUG_SHOW_VEHICLE_PATH)
				{
					auto vTile = v->tileObject;
					if (v->city != state->current_city)
						continue;
					if (!vTile)
						continue;
					if (v->missions.empty())
					{
						continue;
					}
					auto &path = v->missions.front().currentPlannedPath;
					Vec3<float> prevPos = vTile->getPosition();
					for (auto &pos : path)
					{
						Vec3<float> posf = pos;
						posf += Vec3<float>{0.5f, 0.5f, 0.5f};
						Vec2<float> screenPosA = this->tileToOffsetScreenCoords(prevPos);
						Vec2<float> screenPosB = this->tileToOffsetScreenCoords(posf);

						fw().renderer->drawLine(screenPosA, screenPosB,
						                        v->type->isGround() ? groundColor : flyingColor);

						prevPos = posf;
					}
				}
				if (DEBUG_SHOW_VEHICLE_TARGETS)
				{
					if (v->city != state->current_city)
						continue;
					if (v->missions.empty())
					{
						continue;
					}
					StateRef<Vehicle> targetVehicle = v->missions.front().targetVehicle;
					if (targetVehicle)
					{
						auto &targetPos = targetVehicle->position;
						auto &attackerPos = v->position;
						Vec2<float> targetScreenPos = this->tileToOffsetScreenCoords(targetPos);
						Vec2<float> attackerScreenPos = this->tileToOffsetScreenCoords(attackerPos);

						fw().renderer->drawLine(
						    attackerScreenPos, targetScreenPos,
						    targetVehicle->missions.front().targetVehicle == v ? targetMutualColor
						    : v->owner == state->getPlayer()                   ? targetXCOMColor
						                                                       : targetOtherColor);
					}
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
						fw().renderer->draw(img, this->tileToOffsetScreenCoords(thisPos) +
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

		baseForm->render();
		overlayTab->render();
		debugOverlay->render();
		if (activeTab == uiTabs[0])
		{
			// Highlight selected base
			for (auto &view : miniViews)
			{
				auto viewBase = view->getData<Base>();
				if (state->current_base == viewBase)
				{
					Vec2<int> pos = view->getLocationOnScreen() - 1;
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
	unsigned int ticks = 0;
	int day = state->gameTime.getDay();
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

	// Switch dimensions
	bool switchDimension = false;
	// Switch dimension if new day in city and vehicles in alien dimension
	if (day != state->gameTime.getDay())
	{
		for (auto &v : state->vehicles)
		{
			if (state->current_city != v.second->city && v.second->owner == state->getPlayer())
			{
				switchDimension = true;
				break;
			}
		}
	}
	// Switch dimension if no owned vehicle in alien dimension
	if (state->current_city.id == "CITYMAP_ALIEN")
	{
		switchDimension = true;
		for (auto &v : state->vehicles)
		{
			if (state->current_city == v.second->city && v.second->owner == state->getPlayer())
			{
				switchDimension = false;
				break;
			}
			// Don't switch if UFOs crashing
			if (v.second->owner == state->getAliens() && v.second->falling)
			{
				switchDimension = false;
				break;
			}
		}
		// Don't switch if any projectiles exist
		if (!state->current_city->projectiles.empty())
		{
			switchDimension = false;
		}
	}
	if (DEBUG_SHOW_ALIEN ? state->current_city.id != "CITYMAP_ALIEN" : switchDimension)
	{
		setUpdateSpeed(CityUpdateSpeed::Speed1);
		for (auto &newCity : state->cities)
		{
			if (state->current_city != newCity.second)
			{
				if (switchDimension)
				{
					state->setCurrentCity({state.get(), newCity.first});
				}
				else // Debug warping should not trigger research unlocks
				{
					state->current_city = {state.get(), newCity.first};
				}
				auto cityView = mksp<CityView>(state);
				cityView->DEBUG_SHOW_ALIEN = DEBUG_SHOW_ALIEN;
				fw().stageQueueCommand({StageCmd::Command::REPLACEALL, cityView});
				return;
			}
		}
	}

	this->drawCity = true;
	CityTileView::update();

	// Update debug menu
	if (!config().getBool("OpenApoc.NewFeature.DebugCommandsVisible"))
	{
		debugVisible = false;
	}
	else if (debugHotkeyMode)
	{
		debugVisible = true;
	}
	debugOverlay->setVisible(debugVisible);

	updateSelectedUnits();

	// Update time display
	auto clockControl = baseForm->findControlTyped<Label>("CLOCK");
	clockControl->setText(state->gameTime.getLongTimeString());

	// Update base icons
	if (activeTab == uiTabs[0])
	{
		refreshBaseView();
	}

	// Update owned vehicle controls
	if (activeTab == uiTabs[1])
	{
		auto ownedVehicleList = uiTabs[1]->findControlTyped<ListBox>("OWNED_VEHICLE_LIST");

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
				control->addCallback(
				    FormEventType::MouseDown,
				    [this, vehicle](FormsEvent *e)
				    {
					    if (!this->vanillaControls)
					    {
						    if (Event::isPressed(e->forms().MouseInfo.Button,
						                         Event::MouseButton::Right))
						    {
							    // [Alt/Ctrl] + [Shift] opens equipment
							    if ((modifierLShift || modifierRShift) &&
							        (modifierLAlt || modifierRAlt || modifierLCtrl ||
							         modifierRCtrl))
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
					        Event::isPressed(e->forms().MouseInfo.Button,
					                         Event::MouseButton::Right),
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

		// Update weapon controls and vehicle stance
		std::vector<sp<VEquipment>> currentWeapons;
		if (!state->current_city->cityViewSelectedOwnedVehicles.empty())
		{
			auto vehicle = state->current_city->cityViewSelectedOwnedVehicles.front();
			auto vehicleForm = this->uiTabs[1];
			if (vehicle->owner != state->getPlayer())
			{
				if (state->current_city->cityViewSelectedOwnedVehicles.size() > 1)
				{
					vehicle = *++state->current_city->cityViewSelectedOwnedVehicles.begin();
				}
				else
				{
					vehicle = nullptr;
				}
			}
			if (vehicle)
			{
				switch (vehicle->altitude)
				{
					case Vehicle::Altitude::Highest:
						vehicleForm->findControlTyped<RadioButton>("BUTTON_ALTITUDE_HIGHEST")
						    ->setChecked(true);
						break;
					case Vehicle::Altitude::High:
						vehicleForm->findControlTyped<RadioButton>("BUTTON_ALTITUDE_HIGH")
						    ->setChecked(true);
						break;
					case Vehicle::Altitude::Standard:
						vehicleForm->findControlTyped<RadioButton>("BUTTON_ALTITUDE_STANDARD")
						    ->setChecked(true);
						break;
					case Vehicle::Altitude::Low:
						vehicleForm->findControlTyped<RadioButton>("BUTTON_ALTITUDE_LOW")
						    ->setChecked(true);
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
				for (auto &e : vehicle->equipment)
				{
					if (e->type->type == EquipmentSlotType::VehicleWeapon)
					{
						currentWeapons.push_back(e);
					}
				}
			}
		}
		else
		{
			auto vehicleForm = this->uiTabs[1];
			vehicleForm->findControlTyped<RadioButton>("BUTTON_ALTITUDE_HIGHEST")
			    ->setChecked(false);
			vehicleForm->findControlTyped<RadioButton>("BUTTON_ALTITUDE_HIGH")->setChecked(false);
			vehicleForm->findControlTyped<RadioButton>("BUTTON_ALTITUDE_STANDARD")
			    ->setChecked(false);
			vehicleForm->findControlTyped<RadioButton>("BUTTON_ALTITUDE_LOW")->setChecked(false);
			vehicleForm->findControlTyped<RadioButton>("BUTTON_ATTACK_MODE_AGGRESSIVE")
			    ->setChecked(false);
			vehicleForm->findControlTyped<RadioButton>("BUTTON_ATTACK_MODE_STANDARD")
			    ->setChecked(false);
			vehicleForm->findControlTyped<RadioButton>("BUTTON_ATTACK_MODE_DEFENSIVE")
			    ->setChecked(false);
			vehicleForm->findControlTyped<RadioButton>("BUTTON_ATTACK_MODE_EVASIVE")
			    ->setChecked(false);
		}
		for (int i = 0; i < weaponType.size(); i++)
		{
			auto currentWeaponType = currentWeapons.size() > i ? currentWeapons[i]->type : nullptr;
			int currentAmmo =
			    currentWeaponType
			        ? (currentWeaponType->max_ammo > 0
			               ? currentWeapons[i]->ammo * 32 / currentWeaponType->max_ammo
			               : 32)
			        : 0;
			bool currentDisabled = currentWeaponType && currentWeapons[i]->disabled;

			if (currentWeaponType != weaponType[i])
			{
				weaponType[i] = currentWeaponType;
				if (weaponType[i])
				{
					uiTabs[1]
					    ->findControlTyped<Graphic>(format("VEHICLE_WEAPON_%d", i + 1))
					    ->setImage(weaponType[i]->icon);
					uiTabs[1]
					    ->findControlTyped<Graphic>(format("VEHICLE_WEAPON_%d", i + 1))
					    ->ToolTipText = tr(weaponType[i]->name);
					uiTabs[1]
					    ->findControlTyped<CheckBox>(format("VEHICLE_WEAPON_%d_DISABLED", i + 1))
					    ->setVisible(true);
				}
				else
				{
					uiTabs[1]
					    ->findControlTyped<Graphic>(format("VEHICLE_WEAPON_%d", i + 1))
					    ->setImage(nullptr);
					uiTabs[1]
					    ->findControlTyped<Graphic>(format("VEHICLE_WEAPON_%d", i + 1))
					    ->ToolTipText = "";
					uiTabs[1]
					    ->findControlTyped<CheckBox>(format("VEHICLE_WEAPON_%d_DISABLED", i + 1))
					    ->setVisible(false);
				}
			}
			if (currentAmmo != weaponAmmo[i])
			{
				weaponAmmo[i] = currentAmmo;

				auto size = Vec2<int>{6, 32};
				auto bar = mksp<RGBImage>(size);

				int redHeight = currentAmmo;
				{
					static const Colour redColor = {215, 0, 0, 255};
					static const Colour orangeColor = {146, 89, 0, 255};
					RGBImageLock l(bar);
					for (int x = 0; x < size.x; x++)
					{
						for (int y = 1; y <= size.y; y++)
						{
							if (y <= redHeight)
							{
								l.set({x, size.y - y}, redColor);
							}
							else
							{
								l.set({x, size.y - y}, orangeColor);
							}
						}
					}
				}
				uiTabs[1]
				    ->findControlTyped<Graphic>(format("VEHICLE_WEAPON_%d_AMMO", i + 1))
				    ->setImage(bar);
			}
			if (currentDisabled != weaponDisabled[i])
			{
				weaponDisabled[i] = currentDisabled;
				uiTabs[1]
				    ->findControlTyped<CheckBox>(format("VEHICLE_WEAPON_%d_DISABLED", i + 1))
				    ->setChecked(currentDisabled);
			}
		}
	}

	// Update soldier agent controls
	if (activeTab == uiTabs[2])
	{
		auto ownedAgentList = uiTabs[2]->findControlTyped<ListBox>("OWNED_AGENT_LIST");

		auto agentForm = uiTabs[2];
		sp<Label> agentName = agentForm->findControlTyped<Label>("TEXT_AGENT_NAME");
		sp<Label> agentAssignment = agentForm->findControlTyped<Label>("TEXT_AGENT_ASSIGNMENT");
		if (state->current_city->cityViewSelectedSoldiers.empty())
		{
			agentName->setText("");
			agentAssignment->setText("");
		}
		else
		{
			StateRef<Agent> agent = state->current_city->cityViewSelectedSoldiers.front();
			StateRef<Base> base;
			if (agent->type->role == AgentType::Role::Soldier)
			{
				auto agentRName = tr(agent->getRankName()) + UString(" ") + agent->name;
				agentName->setText(agentRName);
				if (agent->currentBuilding == agent->homeBuilding)
				{
					base = agent->currentBuilding->base;
				}
				else if (agent->currentVehicle &&
				         agent->currentVehicle->currentBuilding == agent->homeBuilding)
				{
					base = agent->currentVehicle->currentBuilding->base;
				}
				else
				{
					if (!agent->recentlyHired)
					{
						if (agent->currentBuilding)
							agentAssignment->setText(tr("At") + UString(" ") +
							                         tr(agent->currentBuilding->name));
						else if (agent->currentVehicle && agent->currentVehicle->currentBuilding)
							agentAssignment->setText(
							    tr("At") + UString(" ") +
							    tr(agent->currentVehicle->currentBuilding->name));
						else if (!agent->missions.empty() && agent->missions.front().targetBuilding)
						{
							if (agent->missions.front().targetBuilding == agent->homeBuilding)
								agentAssignment->setText(tr("Returning to base"));
							else
								agentAssignment->setText(
								    tr("Traveling to:") + UString(" ") +
								    tr(agent->missions.front().targetBuilding->name));
						}
						else if (agent->currentVehicle &&
						         !agent->currentVehicle->missions.empty() &&
						         agent->currentVehicle->missions.front().targetBuilding)
						{
							if (agent->currentVehicle->missions.front().targetBuilding ==
							    agent->homeBuilding)
								agentAssignment->setText(tr("Returning to base"));
							else
								agentAssignment->setText(tr("Traveling to:") + UString(" ") +
								                         tr(agent->currentVehicle->missions.front()
								                                .targetBuilding->name));
						}
						else
							agentAssignment->setText(tr("Traveling to:") + UString(" ") +
							                         tr("map point"));
					}
					else
						agentAssignment->setText(tr("Reporting to base"));
				}
			}
			else
			{
				agentName->setText("");
				agentAssignment->setText("");
			}

			if (agent->type->role == AgentType::Role::Soldier && base == agent->homeBuilding->base)
			{
				if (agent->missions.empty() &&
				    agent->modified_stats.health < agent->current_stats.health)
					agentAssignment->setText(tr("Wounded"));
				else
					switch (agent->trainingAssignment)
					{
						case TrainingAssignment::None:
							if (agent->type->canTrain)
								agentAssignment->setText(tr("Not assigned to training"));
							else
								agentAssignment->setText(tr("(Android training not possible)"));
							break;
						case TrainingAssignment::Physical:
						{
							UString efficiency = tr("Combat training (efficiency=");
							int usage = base->getUsage(*state, FacilityType::Capacity::Training);
							usage = (100.0f / std::max(100, usage)) * 100;
							efficiency += format("%d%%", usage) + UString(")");
							agentAssignment->setText(efficiency);
							break;
						}
						case TrainingAssignment::Psi:
						{
							UString efficiency = tr("Psionic training (efficiency=");
							int usage = base->getUsage(*state, FacilityType::Capacity::Psi);
							usage = (100.0f / std::max(100, usage)) * 100;
							efficiency += format("%d%%", usage) + UString(")");
							agentAssignment->setText(efficiency);
							break;
						}
					}
			}
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
				control->addCallback(
				    FormEventType::MouseDown,
				    [this, agent](FormsEvent *e)
				    {
					    if (!this->vanillaControls)
					    {
						    if (Event::isPressed(e->forms().MouseInfo.Button,
						                         Event::MouseButton::Right))
						    {
							    // [Alt/Ctrl] + [Shift] opens equipment
							    if ((modifierLShift || modifierRShift) &&
							        (modifierLAlt || modifierRAlt || modifierLCtrl ||
							         modifierRCtrl))
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
								    fw().stageQueueCommand(
								        {StageCmd::Command::PUSH,
								         mksp<LocationScreen>(this->state, agent)});
								    return;
							    }
						    }
					    }
					    handleClickedAgent(
					        StateRef<Agent>{state.get(), Agent::getId(*state, agent)},
					        Event::isPressed(e->forms().MouseInfo.Button,
					                         Event::MouseButton::Right),
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

		auto agentForm = uiTabs[3];
		sp<Label> agentName = agentForm->findControlTyped<Label>("TEXT_AGENT_NAME");
		sp<Label> agentAssignment = agentForm->findControlTyped<Label>("TEXT_AGENT_ASSIGNMENT");
		if (state->current_city->cityViewSelectedBios.empty())
		{
			agentName->setText("");
			agentAssignment->setText("");
		}
		else
		{
			StateRef<Agent> agent = state->current_city->cityViewSelectedBios.front();
			if (agent->type->role == AgentType::Role::BioChemist)
			{
				agentName->setText(agent->name);
				if (agent->assigned_to_lab)
				{
					auto thisRef = StateRef<Agent>{state.get(), agent};
					for (auto &fac : agent->homeBuilding->base->facilities)
					{
						if (!fac->lab)
						{
							continue;
						}
						auto it = std::find(fac->lab->assigned_agents.begin(),
						                    fac->lab->assigned_agents.end(), thisRef);
						if (it != fac->lab->assigned_agents.end())
						{
							if (fac->lab->current_project)
							{
								UString pr = tr(fac->lab->current_project->name);
								int progress = (static_cast<float>(
								                    fac->lab->current_project->man_hours_progress) /
								                fac->lab->current_project->man_hours) *
								               100;
								agentAssignment->setText(pr + format(" (%d%%)", progress));
							}
							else
								agentAssignment->setText(tr("No project assigned"));
							break;
						}
					}
				}
				else
				{
					if (!agent->recentlyHired)
						agentAssignment->setText(tr("Not assigned to lab"));
					else
						agentAssignment->setText(tr("Reporting to base"));
				}
			}
			else
			{
				agentName->setText("");
				agentAssignment->setText("");
			}
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
				control->addCallback(
				    FormEventType::MouseDown,
				    [this, agent](FormsEvent *e)
				    {
					    if (!this->vanillaControls)
					    {
						    if (Event::isPressed(e->forms().MouseInfo.Button,
						                         Event::MouseButton::Right))
						    {
							    // [Shift] opens location
							    if (modifierLShift || modifierRShift)
							    {
								    // Location screen
								    fw().stageQueueCommand(
								        {StageCmd::Command::PUSH,
								         mksp<LocationScreen>(this->state, agent)});
								    return;
							    }
						    }
					    }
					    handleClickedAgent(
					        StateRef<Agent>{state.get(), Agent::getId(*state, agent)},
					        Event::isPressed(e->forms().MouseInfo.Button,
					                         Event::MouseButton::Right),
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

		auto agentForm = uiTabs[4];
		sp<Label> agentName = agentForm->findControlTyped<Label>("TEXT_AGENT_NAME");
		sp<Label> agentAssignment = agentForm->findControlTyped<Label>("TEXT_AGENT_ASSIGNMENT");
		if (state->current_city->cityViewSelectedEngineers.empty())
		{
			agentName->setText("");
			agentAssignment->setText("");
		}
		else
		{
			StateRef<Agent> agent = state->current_city->cityViewSelectedEngineers.front();
			if (agent->type->role == AgentType::Role::Engineer)
			{
				agentName->setText(agent->name);
				if (agent->assigned_to_lab)
				{
					auto thisRef = StateRef<Agent>{state.get(), agent};
					for (auto &fac : agent->homeBuilding->base->facilities)
					{
						if (!fac->lab)
						{
							continue;
						}
						auto it = std::find(fac->lab->assigned_agents.begin(),
						                    fac->lab->assigned_agents.end(), thisRef);
						if (it != fac->lab->assigned_agents.end())
						{
							if (fac->lab->current_project)
							{
								UString pr = tr(fac->lab->current_project->name);
								int progress =
								    (static_cast<float>(fac->lab->manufacture_man_hours_invested +
								                        fac->lab->current_project->man_hours *
								                            fac->lab->manufacture_done) /
								     (fac->lab->current_project->man_hours *
								      fac->lab->manufacture_goal)) *
								    100;
								agentAssignment->setText(pr + format(" (%d%%)", progress));
							}
							else
								agentAssignment->setText(tr("No project assigned"));
							break;
						}
					}
				}
				else
				{
					if (!agent->recentlyHired)
						agentAssignment->setText(tr("Not assigned to workshop"));
					else
						agentAssignment->setText(tr("Reporting to base"));
				}
			}
			else
			{
				agentName->setText("");
				agentAssignment->setText("");
			}
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
				control->addCallback(
				    FormEventType::MouseDown,
				    [this, agent](FormsEvent *e)
				    {
					    if (!this->vanillaControls)
					    {
						    if (Event::isPressed(e->forms().MouseInfo.Button,
						                         Event::MouseButton::Right))
						    {
							    // [Shift] opens location
							    if (modifierLShift || modifierRShift)
							    {
								    // Location screen
								    fw().stageQueueCommand(
								        {StageCmd::Command::PUSH,
								         mksp<LocationScreen>(this->state, agent)});
								    return;
							    }
						    }
					    }
					    handleClickedAgent(
					        StateRef<Agent>{state.get(), Agent::getId(*state, agent)},
					        Event::isPressed(e->forms().MouseInfo.Button,
					                         Event::MouseButton::Right),
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

		auto agentForm = uiTabs[5];
		sp<Label> agentName = agentForm->findControlTyped<Label>("TEXT_AGENT_NAME");
		sp<Label> agentAssignment = agentForm->findControlTyped<Label>("TEXT_AGENT_ASSIGNMENT");
		if (state->current_city->cityViewSelectedPhysics.empty())
		{
			agentName->setText("");
			agentAssignment->setText("");
		}
		else
		{
			StateRef<Agent> agent = state->current_city->cityViewSelectedPhysics.front();
			if (agent->type->role == AgentType::Role::Physicist)
			{
				agentName->setText(agent->name);
				if (agent->assigned_to_lab)
				{
					auto thisRef = StateRef<Agent>{state.get(), agent};
					for (auto &fac : agent->homeBuilding->base->facilities)
					{
						if (!fac->lab)
						{
							continue;
						}
						auto it = std::find(fac->lab->assigned_agents.begin(),
						                    fac->lab->assigned_agents.end(), thisRef);
						if (it != fac->lab->assigned_agents.end())
						{
							if (fac->lab->current_project)
							{
								UString pr = tr(fac->lab->current_project->name);
								int progress = (static_cast<float>(
								                    fac->lab->current_project->man_hours_progress) /
								                fac->lab->current_project->man_hours) *
								               100;
								agentAssignment->setText(pr + format(" (%d%%)", progress));
							}
							else
								agentAssignment->setText(tr("No project assigned"));
							break;
						}
					}
				}
				else
				{
					if (!agent->recentlyHired)
						agentAssignment->setText(tr("Not assigned to lab"));
					else
						agentAssignment->setText(tr("Reporting to base"));
				}
			}
			else
			{
				agentName->setText("");
				agentAssignment->setText("");
			}
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
				control->addCallback(
				    FormEventType::MouseDown,
				    [this, agent](FormsEvent *e)
				    {
					    if (!this->vanillaControls)
					    {
						    if (Event::isPressed(e->forms().MouseInfo.Button,
						                         Event::MouseButton::Right))
						    {
							    // [Shift] opens location
							    if (modifierLShift || modifierRShift)
							    {
								    // Location screen
								    fw().stageQueueCommand(
								        {StageCmd::Command::PUSH,
								         mksp<LocationScreen>(this->state, agent)});
								    return;
							    }
						    }
					    }
					    handleClickedAgent(
					        StateRef<Agent>{state.get(), Agent::getId(*state, agent)},
					        Event::isPressed(e->forms().MouseInfo.Button,
					                         Event::MouseButton::Right),
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

	// Update hostile vehicle controls
	if (activeTab == uiTabs[6])
	{
		auto hostileVehicleList = uiTabs[6]->findControlTyped<ListBox>("HOSTILE_VEHICLE_LIST");

		if (!state->current_city->cityViewSelectedOtherVehicles.empty())
		{
			auto selectedVehicle = state->current_city->cityViewSelectedOtherVehicles.front();
			if (selectedVehicle->owner == state->getPlayer())
			{
				uiTabs[6]->findControlTyped<Label>("TEXT_VEHICLE_NAME")->setText("");
				uiTabs[6]->findControlTyped<Label>("TEXT_VEHICLE_OWNER")->setText("");
			}
			else
			{
				uiTabs[6]
				    ->findControlTyped<Label>("TEXT_VEHICLE_NAME")
				    ->setText(selectedVehicle->name);
				uiTabs[6]
				    ->findControlTyped<Label>("TEXT_VEHICLE_OWNER")
				    ->setText(selectedVehicle->owner->name);
			}
		}
		else
		{
			uiTabs[6]->findControlTyped<Label>("TEXT_VEHICLE_NAME")->setText("");
			uiTabs[6]->findControlTyped<Label>("TEXT_VEHICLE_OWNER")->setText("");
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
			    v.second->isDead() || !v.second->tileObject)
			{
				continue;
			}

			// Show selected non-player vehicle in list of hostile vehicles
			if (state->getPlayer()->isRelatedTo(vehicle->owner) != Organisation::Relation::Hostile)
			{
				if (vehicle->owner == state->getPlayer() ||
				    state->current_city->cityViewSelectedOwnedVehicles.empty() ||
				    state->current_city->cityViewSelectedOwnedVehicles.front() != v.second)
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
				control->addCallback(
				    FormEventType::MouseDown,
				    [this, vehicle](FormsEvent *e)
				    {
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
					    //			fw().stageQueueCommand({ StageCmd::Command::PUSH, equipScreen
					    //}); 			return;
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
					        Event::isPressed(e->forms().MouseInfo.Button,
					                         Event::MouseButton::Right),
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

		auto selectedOrg = state->current_city->cityViewSelectedOrganisation;
		if (selectedOrg && (state->current_city->cityViewOrgButtonIndex == 0 ||
		                    static_cast<int>(selectedOrg->isRelatedTo(state->getPlayer())) ==
		                        state->current_city->cityViewOrgButtonIndex - 1))
		{
			uiTabs[7]->findControlTyped<Label>("TEXT_ORG_NAME")->setText(tr(selectedOrg->name));
			UString relation = "";
			switch (selectedOrg->isRelatedTo(state->getPlayer()))
			{
				case Organisation::Relation::Allied:
					relation += tr(": allied with:");
					break;
				case Organisation::Relation::Friendly:
					relation += tr(": friendly with:");
					break;
				case Organisation::Relation::Neutral:
					relation += tr(": neutral towards:");
					break;
				case Organisation::Relation::Unfriendly:
					relation += tr(": unfriendly towards:");
					break;
				case Organisation::Relation::Hostile:
					relation += tr(": hostile towards:");
					break;
			}
			relation += UString(" ") + tr(state->getPlayer()->name);
			uiTabs[7]->findControlTyped<Label>("TEXT_ORG_RELATION")->setText(relation);
		}
		else
		{
			uiTabs[7]->findControlTyped<Label>("TEXT_ORG_NAME")->setText("");
			uiTabs[7]->findControlTyped<Label>("TEXT_ORG_RELATION")->setText("");
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
				control->addCallback(FormEventType::MouseDown,
				                     [this, org](FormsEvent *e)
				                     {
					                     // if (!this->vanillaControls)
					                     //{
					                     //	if (Event::isPressed(e->forms().MouseInfo.Button,
					                     //		Event::MouseButton::Right))
					                     //	{
					                     //		// [Alt/Ctrl] + [Shift] opens equipment
					                     //		if ((modifierLShift || modifierRShift) &&
					                     //			(modifierLAlt || modifierRAlt || modifierLCtrl
					                     //|| 				modifierRCtrl))
					                     //		{
					                     //			// Equipscreen for owner vehicles
					                     //			auto equipScreen =
					                     // mksp<VEquipScreen>(this->state);
					                     //			equipScreen->setSelectedVehicle(vehicle);
					                     //			fw().stageQueueCommand({
					                     // StageCmd::Command::PUSH, equipScreen }); return;
					                     //		}
					                     //		// [Shift] opens location
					                     //		if (modifierLShift || modifierRShift)
					                     //		{
					                     //			// Location screen
					                     //			fw().stageQueueCommand(
					                     //			{ StageCmd::Command::PUSH,
					                     //				mksp<LocationScreen>(this->state, vehicle)
					                     //}); 			return;
					                     //		}
					                     //	}
					                     //}
					                     handleClickedOrganisation(
					                         StateRef<Organisation>{state.get(), org->id},
					                         Event::isPressed(e->forms().MouseInfo.Button,
					                                          Event::MouseButton::Right),
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

	baseForm->update();
	overlayTab->update();

	// If we have 'follow vehicle' enabled we clobber any other movement that may have occurred in
	// this frame
	if (this->followVehicle && this->updateSpeed != CityUpdateSpeed::Pause)
	{
		if (activeTab == uiTabs[1] && !state->current_city->cityViewSelectedOwnedVehicles.empty())
		{
			auto v = state->current_city->cityViewSelectedOwnedVehicles.front();
			if (v->city == state->current_city)
			{
				// Don't follow if vehicle is in building
				if (!v->currentBuilding)
				{
					this->setScreenCenterTile(v->position);
				}
			}
		}
		else if (activeTab == uiTabs[2] && !state->current_city->cityViewSelectedSoldiers.empty())
		{
			auto a = state->current_city->cityViewSelectedSoldiers.front();

			if (a->city == state->current_city)
			{
				if (a->currentVehicle)
				{
					// Don't follow if current vehicle is in building
					if (!a->currentVehicle->currentBuilding)
					{
						this->setScreenCenterTile(a->currentVehicle->position);
					}
				}
			}
		}
		else if (!state->current_city->cityViewSelectedOtherVehicles.empty() &&
		         activeTab == uiTabs[6])
		{
			auto v = state->current_city->cityViewSelectedOtherVehicles.front();
			if (v->city == state->current_city)
			{
				// Don't follow if vehicle is in building
				if (!v->currentBuilding)
				{
					this->setScreenCenterTile(v->position);
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

void CityView::initiateBuildingMission(sp<GameState> state, StateRef<Building> building,
                                       std::list<StateRef<Agent>> agents)
{
	bool inBuilding = true;
	bool raid = false;
	bool hotseat = false;
	fw().stageQueueCommand(
	    {StageCmd::Command::REPLACEALL,
	     mksp<BattleBriefing>(
	         state, building->owner, Building::getId(*state, building), inBuilding, raid,
	         loadBattleBuilding(state, building, hotseat, raid, agents, nullptr))});
}

void CityView::eventOccurred(Event *e)
{
	this->drawCity = true;
	if (overlayTab->isVisible())
	{
		overlayTab->eventOccured(e);
	}
	baseForm->eventOccured(e);

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
		case SDLK_F1:
			if (config().getBool("OpenApoc.NewFeature.DebugCommandsVisible"))
			{
				debugVisible = !debugVisible;
			}
	}

	if (e->type() == EVENT_KEY_DOWN)
	{
		// Cheat codes
		if (debugHotkeyMode)
		{
			switch (e->keyboard().KeyCode)
			{
				case SDLK_r:
				{
					if (modifierLShift)
					{
						LogInfo("Single Level Debug Repair");
						state->current_city->repairScenery(*state, true);
						return true;
					}
					else
					{
						LogInfo("Debug Repair");
						std::set<sp<Scenery>> stuffToRepair;
						for (auto &s : state->current_city->scenery)
						{
							if (s->damaged || !s->isAlive())
							{
								stuffToRepair.insert(s);
							}
						}
						LogInfo("Repairing %u tiles out of %u",
						        static_cast<unsigned>(stuffToRepair.size()),
						        static_cast<unsigned>(state->current_city->scenery.size()));

						for (auto &s : stuffToRepair)
						{
							s->repair(*state);
						}
						return true;
					}
				}
				case SDLK_x:
				{
					LogWarning("Crashing!");
					for (auto &v : state->vehicles)
					{
						if (v.second->currentBuilding || v.second->city != state->current_city ||
						    v.second->crashed || v.second->falling || !v.second->tileObject)
						{
							continue;
						}
						v.second->health = v.second->type->crash_health > 0
						                       ? v.second->type->crash_health
						                       : v.second->getMaxHealth() / 4;
						if (v.second->type->type == VehicleType::Type::UFO)
						{
							v.second->crash(*state, nullptr);
						}
						else
						{
							v.second->startFalling(*state);
						}
					}
					return true;
				}
				case SDLK_u:
				{
					LogWarning("Spawning crashed UFOs...");

					std::vector<StateRef<VehicleType>> validTypes;

					auto pos = this->centerPos;

					for (const auto &type : state->vehicle_types)
					{
						if (type.second->crashed_sprite)
						{
							validTypes.emplace_back(state.get(), type.second);
							LogWarning("Valid UFO type: %s", type.second->name);
						}
					}

					for (int i = 0; i < 3; i++)
					{
						auto type = pickRandom(state->rng, validTypes);
						LogWarning("Crashing %s", type->name);
						pos.z = 9 + i;
						auto ufo = state->current_city->placeVehicle(*state, {state.get(), type},
						                                             state->getAliens(), pos);
						ufo->crash(*state, nullptr);
						ufo->applyDamage(*state, 1, 0);
					}

					return true;
				}
				case SDLK_a:
				{
					LogWarning("All you ever want...");

					for (auto &e : state->vehicle_equipment)
					{
						if (e.second->store_space > 0)
						{
							state->current_base->inventoryVehicleEquipment[e.first]++;
						}
					}
					for (auto &e : state->vehicle_ammo)
					{
						state->current_base->inventoryVehicleAmmo[e.first] += 10;
					}

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
					                          *state, *v, false, state->current_base->building));
					return true;
				}
				case SDLK_MINUS:
				{
					LogWarning("Destroying selected vehicles...");
					for (auto &v : state->current_city->cityViewSelectedOwnedVehicles)
					{
						v->die(*state);
					}
					for (auto &o : state->current_city->cityViewSelectedOtherVehicles)
					{
						o->die(*state);
					}

					return true;
				}
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
	static const std::set<TileObject::Type> sceneryPortalVehicleSet = {
	    TileObject::Type::Scenery, TileObject::Type::Doodad, TileObject::Type::Vehicle};
	static const std::set<TileObject::Type> projectileSet = {TileObject::Type::Projectile};
	static const std::set<TileObject::Type> vehicleSet = {TileObject::Type::Vehicle};

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
		auto collision = state->current_city->map->findCollision(
		    clickTop, clickBottom, sceneryPortalVehicleSet, nullptr, true);

		auto position = collision.position;
		bool portal = false;
		sp<Scenery> scenery;
		StateRef<Building> building;
		sp<Vehicle> vehicle;
		sp<Projectile> projectile;
		if (collision)
		{
			switch (collision.obj->getType())
			{
				case TileObject::Type::Doodad:
				{
					// Clicked at portal
					portal = true;
					break;
				}
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
						LogWarning("Mission %s", m.getName());
					}
					for (auto &c : vehicle->cargo)
					{
						LogInfo("Cargo %sx%d", c.id, c.count);
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
							if (vehicle->type->type == VehicleType::Type::UFO)
							{
								vehicle->crash(*state, nullptr);
							}
							else
							{
								vehicle->startFalling(*state);
							}
						}
						return true;
					}
					break;
				}
				default:
				{
					LogError("Clicked on some object we didn't care to process?");
					break;
				}
			}
			// Find vehicle click if no vehicle
			if (!vehicle)
			{
				auto collisionVehicle = state->current_city->map->findCollision(
				    clickTop, clickBottom, vehicleSet, nullptr, true);
				if (collisionVehicle)
				{
					vehicle = std::dynamic_pointer_cast<TileObjectVehicle>(collisionVehicle.obj)
					              ->getVehicle();
					LogWarning("SECONDARY CLICK ON VEHICLE %s at %s", vehicle->name,
					           vehicle->position);
					for (auto &m : vehicle->missions)
					{
						LogWarning("Mission %s", m.getName());
					}
					for (auto &c : vehicle->cargo)
					{
						LogInfo("Cargo %sx%d", c.id, c.count);
					}
				}
			}
		}

		auto projCollision = state->current_city->map->findCollision(clickTop, clickBottom,
		                                                             projectileSet, nullptr, true);
		if (projCollision)
		{
			projectile =
			    std::dynamic_pointer_cast<TileObjectProjectile>(projCollision.obj)->getProjectile();
			LogInfo("CLICKED PROJECTILE %d at %s", projectile->damage, projectile->position);

			if (!vehicle && !scenery && !portal)
			{
				position = projectile->position;
			}
		}

		// Try to handle clicks on objects
		if (vehicle || scenery || portal || projectile)
		{
			// Click on projectile
			if (projectile &&
			    handleClickedProjectile(projectile, buttonPressed == Event::MouseButton::Right,
			                            selectionState))
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
			// Click on vehicle
			if (vehicle &&
			    handleClickedVehicle(
			        StateRef<Vehicle>{state.get(), Vehicle::getId(*state, vehicle)},
			        buttonPressed == Event::MouseButton::Right, selectionState, (bool)scenery))
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
						orderMove(position, modifierRCtrl || modifierLCtrl, portal);
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
				default:
				{
					// Other selection modes don't affect these objects
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
			     mksp<NotificationScreen>(
			         state, *this,
			         format("Aliens have taken over %s", gameOrgEvent->organisation->name),
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
			state->totalScore.alienIncidents += ALIEN_INCIDENT_SCORE;
			state->weekScore.alienIncidents += ALIEN_INCIDENT_SCORE;
			fw().soundBackend->playSample(
			    pickRandom(state->rng, state->city_common_sample_list->alertSounds));
			zoomLastEvent();
			setUpdateSpeed(CityUpdateSpeed::Speed1);
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH, mksp<AlertScreen>(state, ev->building)});
			break;
		}
		case GameEventType::CommenceInvestigation:
		{
			auto ev = dynamic_cast<GameBuildingEvent *>(e);
			if (!ev)
			{
				LogError("Invalid investigation event");
			}
			auto game_state = this->state;
			auto building = ev->building;

			// the player may have called an alien extermination before all units arrive
			// in that case we don't want to prompt for investigation on arrival
			if (!building->detected)
			{
				break;
			}

			std::list<StateRef<Agent>> agents;
			for (auto v : building->currentVehicles)
			{
				if (v->owner == state->getPlayer())
				{
					for (auto a : v->currentAgents)
					{
						if (a->owner == state->getPlayer())
						{
							agents.push_back(a);
						}
					}
				}
			}
			for (auto a : building->currentAgents)
			{
				if (a->owner == state->getPlayer())
				{
					agents.push_back(a);
				}
			}

			// the player may have recalled agents
			// in that case we don't want to prompt for investigation
			if (agents.empty())
			{
				break;
			}

			UString title = tr("Commence investigation");
			UString message = format(tr("All selected units and crafts have arrived at %s. "
			                            "Proceed with investigation? (%d units)"),
			                         building->name, agents.size());
			fw().stageQueueCommand({StageCmd::Command::PUSH,
			                        mksp<MessageBox>(
			                            title, message, MessageBox::ButtonOptions::YesNo,
			                            // "Yes" callback
			                            [this, game_state, building, agents]()
			                            { initiateBuildingMission(game_state, building, agents); },
			                            // "No" callback
			                            [this]() { setUpdateSpeed(CityUpdateSpeed::Pause); })});
			break;
		}
		case GameEventType::ResearchCompleted:
		{
			auto ev = dynamic_cast<GameResearchEvent *>(e);
			if (!ev)
			{
				LogError("Invalid research event");
			}
			state->totalScore.researchCompleted += ev->topic->score;
			state->weekScore.researchCompleted += ev->topic->score;
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
			setUpdateSpeed(CityUpdateSpeed::Pause);
			auto message_box = mksp<MessageBox>(
			    tr("RESEARCH COMPLETE"),
			    format("%s\n%s\n%s", tr("Research project completed:"), ev->topic->name,
			           tr("Do you wish to view the UFOpaedia report?")),
			    MessageBox::ButtonOptions::YesNo,
			    // "Yes" callback
			    [game_state, lab_facility, ufopaedia_category, ufopaedia_entry]()
			    {
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
			    // "No" callback
			    [game_state, lab_facility]()
			    {
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
			setUpdateSpeed(CityUpdateSpeed::Pause);
			auto message_box = mksp<MessageBox>(
			    tr("MANUFACTURE COMPLETED"),
			    format("%s\n%s\n%s %d\n%s", lab_base->name, tr(item_name), tr("Quantity:"),
			           ev->goal, tr("Do you wish to reasign the Workshop?")),
			    MessageBox::ButtonOptions::YesNo,
			    // Yes callback
			    [game_state, lab_facility]()
			    {
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
					item_name = game_state->vehicle_types[ev->topic->itemId]->name;
					break;
			}
			setUpdateSpeed(CityUpdateSpeed::Pause);
			auto message_box =
			    mksp<MessageBox>(tr("MANUFACTURING HALTED"),
			                     format("%s\n%s\n%s %d/%d\n%s", lab_base->name, tr(item_name),
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
			setUpdateSpeed(CityUpdateSpeed::Pause);
			auto message_box =
			    mksp<MessageBox>(tr("FACILITY COMPLETED"),
			                     format("%s\n%s", ev->base->name, tr(ev->facility->type->name)),
			                     MessageBox::ButtonOptions::Ok);
			fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
		}
		break;
		case GameEventType::OrganisationRequestBribe:
		{
			auto gameOrgEvent = dynamic_cast<GameOrganisationEvent *>(e);
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH,
			     mksp<DiplomaticTreatyScreen>(this->state, gameOrgEvent->organisation)});
		}
		break;
		case GameEventType::DailyReport:
		{
			setUpdateSpeed(CityUpdateSpeed::Pause);
			showDailyReport();
		}
		break;
		case GameEventType::WeeklyReport:
		{
			setUpdateSpeed(CityUpdateSpeed::Pause);
			showWeeklyFundingReport();
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
		auto it = state->current_city->cityViewSelectedOwnedVehicles.begin();
		while (it != state->current_city->cityViewSelectedOwnedVehicles.end())
		{
			auto v = *it;
			if (!v || v->isDead() || v->city != state->current_city)
			{
				it = state->current_city->cityViewSelectedOwnedVehicles.erase(it);
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
		auto itSold = state->current_city->cityViewSelectedSoldiers.begin();
		while (itSold != state->current_city->cityViewSelectedSoldiers.end())
		{
			auto a = *itSold;
			if (!a || a->isDead())
			{
				itSold = state->current_city->cityViewSelectedSoldiers.erase(itSold);
			}
			else
			{
				if (a->owner == o)
				{
					foundOwned = true;
				}
				itSold++;
			}
		}
		auto itBio = state->current_city->cityViewSelectedBios.begin();
		while (itBio != state->current_city->cityViewSelectedBios.end())
		{
			auto a = *itBio;
			if (!a || a->isDead())
			{
				itBio = state->current_city->cityViewSelectedBios.erase(itBio);
			}
			else
			{
				if (a->owner == o)
				{
					foundOwned = true;
				}
				itBio++;
			}
		}
		auto itPhy = state->current_city->cityViewSelectedPhysics.begin();
		while (itPhy != state->current_city->cityViewSelectedPhysics.end())
		{
			auto a = *itPhy;
			if (!a || a->isDead())
			{
				itPhy = state->current_city->cityViewSelectedPhysics.erase(itPhy);
			}
			else
			{
				if (a->owner == o)
				{
					foundOwned = true;
				}
				itPhy++;
			}
		}
		auto itEng = state->current_city->cityViewSelectedEngineers.begin();
		while (itEng != state->current_city->cityViewSelectedEngineers.end())
		{
			auto a = *itEng;
			if (!a || a->isDead())
			{
				itEng = state->current_city->cityViewSelectedEngineers.erase(itEng);
			}
			else
			{
				if (a->owner == o)
				{
					foundOwned = true;
				}
				itEng++;
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
				if (state->current_city->cityViewSelectedOwnedVehicles.size() > 1)
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
				if (state->current_city->cityViewSelectedSoldiers.size() > 1)
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
