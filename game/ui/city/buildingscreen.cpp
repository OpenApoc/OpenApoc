#include "game/ui/city/buildingscreen.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/battle/battle.h"
#include "game/state/city/building.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/shared/organisation.h"
#include "game/ui/base/vequipscreen.h"
#include "game/ui/battle/battlebriefing.h"
#include "game/ui/components/agentassignment.h"
#include "game/ui/general/aequipscreen.h"
#include "game/ui/general/messagebox.h"
#include "library/strings_format.h"

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
	    [hotseat, building, state, raid, playerAgents, playerVehicle]() mutable -> void {
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
} // namespace
BuildingScreen::BuildingScreen(sp<GameState> state, sp<Building> building)
    : Stage(), menuform(ui().getForm("city/building")), state(state), building(building)
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	menuform->findControlTyped<Label>("TEXT_BUILDING_NAME")->setText(tr(building->name));
	menuform->findControlTyped<Label>("TEXT_OWNER_NAME")->setText(tr(building->owner->name));
	menuform->findControlTyped<Label>("TEXT_BUILDING_FUNCTION")
	    ->setText(tr(building->function->name));
}

BuildingScreen::~BuildingScreen() = default;

void BuildingScreen::begin()
{
	auto agentAssignmentPlaceholder = menuform->findControlTyped<Graphic>("AGENT_ASSIGNMENT");
	agentAssignment = menuform->createChild<AgentAssignment>(state);
	agentAssignment->init(ui().getForm("city/agentassignment"),
	                      agentAssignmentPlaceholder->Location, agentAssignmentPlaceholder->Size);
	agentAssignment->setLocation(building);
}

void BuildingScreen::pause() {}

void BuildingScreen::resume() {}

void BuildingScreen::finish() {}

void BuildingScreen::eventOccurred(Event *e)
{
	menuform->eventOccured(e);
	if (menuform->eventIsWithin(e))
	{
		return;
	}

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_RETURN ||
		    e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			menuform->findControl("BUTTON_QUIT")->click();
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_EXTERMINATE" ||
		    e->forms().RaisedBy->Name == "BUTTON_RAID")
		{
			if (!building->isAlive())
			{
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(tr("No Entrance"), tr("Cannot raid as building destroyed"),
				                      MessageBox::ButtonOptions::Ok)});
				return;
			}

			if (building->accessTopic && !building->accessTopic->isComplete())
			{
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(tr("No Entrance"),
				                      tr("Our Agents are unable to find an entrance to this "
				                         "building. Our Scientists "
				                         "back at HQ must complete their research."),
				                      MessageBox::ButtonOptions::Ok)});
				return;
			}

			std::list<StateRef<Agent>> agents(agentAssignment->getSelectedAgents());
			StateRef<Vehicle> vehicle;
			if (agentAssignment->currentVehicle)
			{
				vehicle = {state.get(), Vehicle::getId(*state, agentAssignment->currentVehicle)};
			}

			if (agents.empty())
			{
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(tr("No Agents Selected"),
				                      tr("You need to select the agents you want to become active "
				                         "within the building."),
				                      MessageBox::ButtonOptions::Ok)});
			}
			else
			{
				if (e->forms().RaisedBy->Name == "BUTTON_EXTERMINATE" &&
				    building->owner != state->getAliens())
				{
					bool foundAlien = false;
					for (auto &e : building->current_crew)
					{
						if (e.second > 0)
						{
							foundAlien = true;
							break;
						}
					}
					if (!foundAlien)
					{
						UString message = "You have not found any Aliens in this building.";
						if (building->owner != state->getPlayer())
						{
							auto priorRelationship =
							    building->owner->isRelatedTo(state->getPlayer());
							building->owner->adjustRelationTo(*state, state->getPlayer(),
							                                  -5 - state->difficulty);
							auto newRelationship = building->owner->isRelatedTo(state->getPlayer());
							if (newRelationship != priorRelationship &&
							    newRelationship == Organisation::Relation::Unfriendly)
							{
								message = "You have not found any Aliens in this building. As "
								          "a consequence of your "
								          "unwelcome intrusion the owner of the building has "
								          "now become unfriendly "
								          "towards X-Com.";
							}
							else if (newRelationship != priorRelationship &&
							         newRelationship == Organisation::Relation::Hostile)
							{
								message = "You have not found any Aliens in this building. As "
								          "a consequence of your "
								          "unwelcome intrusion the owner of the building has "
								          "now become hostile towards"
								          " X-Com.";
							}
							else
							{
								message = "You have not found any Aliens in this building. "
								          "As a consequence of your "
								          "unwelcome intrusion the owner of the building is "
								          "less favorably disposed "
								          "towards X-Com.";
							}
						}
						fw().stageQueueCommand(
						    {StageCmd::Command::PUSH,
						     mksp<MessageBox>(tr("No Hostile Forces Discovered"), tr(message),
						                      MessageBox::ButtonOptions::Ok)});
					}
					else
					{
						bool inBuilding = true;
						bool raid = false;
						bool hotseat = false;
						fw().stageQueueCommand(
						    {StageCmd::Command::REPLACEALL,
						     mksp<BattleBriefing>(state, building->owner,
						                          Building::getId(*state, building), inBuilding,
						                          raid,
						                          loadBattleBuilding(state, building, hotseat, raid,
						                                             agents, vehicle))});
						return;
					}
				}
				else
				{
					if (building->owner == state->getPlayer())
					{
						fw().stageQueueCommand(
						    {StageCmd::Command::PUSH,
						     mksp<MessageBox>(
						         tr("No Hostile Forces Discovered"),
						         tr("You have not found any hostile forces in this building."),
						         MessageBox::ButtonOptions::Ok)});
						return;
					}
					if (config().getBool("OpenApoc.Mod.RaidHostileAction"))
					{
						building->owner->adjustRelationTo(*state, state->getPlayer(), -200.0f);
					}
					bool inBuilding = true;
					bool raid = true;
					bool hotseat = false;
					fw().stageQueueCommand(
					    {StageCmd::Command::REPLACEALL,
					     mksp<BattleBriefing>(
					         state, building->owner, Building::getId(*state, building), inBuilding,
					         raid,
					         loadBattleBuilding(state, building, hotseat, raid, agents, vehicle))});
					return;
				}
			}
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_EQUIPAGENT")
		{
			if (agentAssignment->currentAgent)
			{
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<AEquipScreen>(this->state, agentAssignment->currentAgent)});
			}
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_EQUIPVEHICLE")
		{
			if (agentAssignment->currentVehicle)
			{
				auto equipScreen = mksp<VEquipScreen>(this->state);
				equipScreen->setSelectedVehicle(agentAssignment->currentVehicle);
				fw().stageQueueCommand({StageCmd::Command::PUSH, equipScreen});
			}
			return;
		}
	}
}

void BuildingScreen::update() { menuform->update(); }

void BuildingScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->preRender();
	menuform->render();
}

bool BuildingScreen::isTransition() { return false; }

}; // namespace OpenApoc
