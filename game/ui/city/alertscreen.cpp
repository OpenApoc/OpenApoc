#include "game/ui/city/alertscreen.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/city/agentmission.h"
#include "game/state/city/building.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gamestate.h"
#include "game/state/shared/organisation.h"
#include "game/ui/base/vequipscreen.h"
#include "game/ui/components/agentassignment.h"
#include "game/ui/general/aequipscreen.h"
#include "game/ui/general/messagebox.h"
#include "library/strings_format.h"

namespace OpenApoc
{

AlertScreen::AlertScreen(sp<GameState> state, sp<Building> building)
    : Stage(), menuform(ui().getForm("city/alert")), state(state), building(building)
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	menuform->findControlTyped<Label>("TEXT_OWNER_NAME")->setText(tr(building->owner->name));
	menuform->findControlTyped<Label>("TEXT_BUILDING_FUNCTION")
	    ->setText(tr(building->function->name));
}

AlertScreen::~AlertScreen() = default;

void AlertScreen::begin()
{
	auto agentAssignmentPlaceholder = menuform->findControlTyped<Graphic>("AGENT_ASSIGNMENT");
	agentAssignment = menuform->createChild<AgentAssignment>(state);
	agentAssignment->init(ui().getForm("city/agentassignment"),
	                      agentAssignmentPlaceholder->Location, agentAssignmentPlaceholder->Size);
	agentAssignment->setLocation();
}

void AlertScreen::pause() {}

void AlertScreen::resume() {}

void AlertScreen::finish() {}

void AlertScreen::eventOccurred(Event *e)
{
	menuform->eventOccured(e);
	if (menuform->eventIsWithin(e))
	{
		return;
	}

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_EXTERMINATE")
		{
			// send a vehicles fleet
			std::list<StateRef<Vehicle>> selectedVehicles(agentAssignment->getSelectedVehicles());
			if (!selectedVehicles.empty())
			{
				for (auto &vehicle : selectedVehicles)
				{
					vehicle->setMission(*state, VehicleMission::gotoBuilding(
					                                *state, *vehicle, {state.get(), building}));
				}
				fw().stageQueueCommand({StageCmd::Command::POP});
				return;
			}

			// send an angents group on foot
			std::list<StateRef<Agent>> selectedAgents(agentAssignment->getSelectedAgents());
			if (!selectedAgents.empty())
			{
				for (auto &agent : selectedAgents)
				{
					agent->setMission(*state, AgentMission::gotoBuilding(*state, *agent,
					                                                     {state.get(), building}));
				}
				fw().stageQueueCommand({StageCmd::Command::POP});
				return;
			}

			fw().stageQueueCommand({StageCmd::Command::PUSH,
			                        mksp<MessageBox>(tr("No Agents Selected"),
			                                         tr("You need to select the agents you want to "
			                                            "become active within the building."),
			                                         MessageBox::ButtonOptions::Ok)});
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
		if (e->forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}
}

void AlertScreen::update() { menuform->update(); }

void AlertScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->preRender();
	menuform->render();
}

bool AlertScreen::isTransition() { return false; }

}; // namespace OpenApoc
