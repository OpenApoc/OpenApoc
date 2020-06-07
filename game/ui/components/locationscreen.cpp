#include "game/ui/components/locationscreen.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/city/building.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/shared/agent.h"
#include "game/state/shared/organisation.h"
#include "game/ui/base/vequipscreen.h"
#include "game/ui/components/agentassignment.h"
#include "game/ui/general/aequipscreen.h"
#include "library/strings_format.h"

namespace OpenApoc
{

LocationScreen::LocationScreen(sp<GameState> state, sp<Agent> agent)
    : Stage(), menuform(ui().getForm("city/location")), state(state), agent(agent)
{
	if (agent->currentBuilding)
	{
		building = agent->currentBuilding;
	}
	menuform->findControlTyped<Label>("CAPTION")->setText(tr("AGENT LOCATION"));
	menuform->findControlTyped<Graphic>("BG")->setImage(
	    fw().data->loadImage("xcom3/ufodata/location.pcx"));
}

LocationScreen::LocationScreen(sp<GameState> state, sp<Vehicle> vehicle)
    : Stage(), menuform(ui().getForm("city/location")), state(state), vehicle(vehicle)
{
	if (vehicle->currentBuilding)
	{
		building = vehicle->currentBuilding;
	}
	menuform->findControlTyped<Label>("CAPTION")->setText(tr("VEHICLE LOCATION"));
	menuform->findControlTyped<Graphic>("BG")->setImage(
	    fw().data->loadImage("xcom3/ufodata/locatn2.pcx"));
}

LocationScreen::~LocationScreen() = default;

void LocationScreen::begin()
{
	auto agentAssignmentPlaceholder = menuform->findControlTyped<Graphic>("AGENT_ASSIGNMENT");
	agentAssignment = menuform->createChild<AgentAssignment>(state);
	agentAssignment->init(ui().getForm("city/agentassignment"),
	                      agentAssignmentPlaceholder->Location, agentAssignmentPlaceholder->Size);
	if (building)
	{
		menuform->findControlTyped<Label>("TEXT_OWNER_NAME")->setText(tr(building->owner->name));
		agentAssignment->setLocation(building);
	}
	else if (agent)
	{
		menuform->findControlTyped<Label>("TEXT_OWNER_NAME")->setText(tr(agent->owner->name));
		agentAssignment->setLocation(agent);
	}
	else if (vehicle)
	{
		menuform->findControlTyped<Label>("TEXT_OWNER_NAME")->setText(tr(vehicle->owner->name));
		agentAssignment->setLocation(vehicle);
	}
	else
	{
		LogError("Nothing set as owner in LocationScreen?");
	}
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
}

void LocationScreen::pause() {}

void LocationScreen::resume() {}

void LocationScreen::finish() {}

void LocationScreen::eventOccurred(Event *e)
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

void LocationScreen::update() { menuform->update(); }

void LocationScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->preRender();
	menuform->render();
}

bool LocationScreen::isTransition() { return false; }

}; // namespace OpenApoc
