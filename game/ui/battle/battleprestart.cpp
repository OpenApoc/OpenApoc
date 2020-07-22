#include "game/ui/battle/battleprestart.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/apocresources/cursor.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlecommonimagelist.h"
#include "game/state/shared/aequipment.h"
#include "game/state/shared/agent.h"
#include "game/ui/components/controlgenerator.h"
#include "game/ui/general/aequipscreen.h"
#include "game/ui/general/agentsheet.h"
#include "game/ui/general/loadingscreen.h"
#include "game/ui/tileview/battleview.h"
#include <cmath>

namespace OpenApoc
{

std::shared_future<void> enterBattle(sp<GameState> state)
{
	auto loadTask =
	    fw().threadPoolEnqueue([state]() -> void { Battle::enterBattle(*state.get()); });

	return loadTask;
}

void BattlePreStart::displayAgent(sp<Agent> agent)
{
	if (!agent)
	{
		return;
	}

	AgentSheet(formAgentStats)
	    .display(*agent, bigUnitRanks, state->current_battle->mode == Battle::Mode::TurnBased);
	formAgentStats->setVisible(true);

	auto rHand = agent->getFirstItemInSlot(EquipmentSlotType::RightHand);
	auto lHand = agent->getFirstItemInSlot(EquipmentSlotType::LeftHand);
	menuform->findControlTyped<Graphic>("RIGHT_HAND")
	    ->setImage(rHand ? rHand->type->equipscreen_sprite : nullptr);
	menuform->findControlTyped<Graphic>("LEFT_HAND")
	    ->setImage(lHand ? lHand->type->equipscreen_sprite : nullptr);
}
BattlePreStart::BattlePreStart(sp<GameState> state)
    : Stage(), TOP_LEFT({302, 80}), menuform(ui().getForm("battle/prestart")), state(state)
{

	menuform->findControlTyped<GraphicButton>("BUTTON_EQUIP")
	    ->addCallback(FormEventType::ButtonClick, [state](Event *) {
		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<AEquipScreen>(state)});
	    });
	formAgentStats = menuform->findControlTyped<Form>("AGENT_STATS_VIEW");
	formAgentStats->setVisible(false);
	menuform->findControlTyped<GraphicButton>("BUTTON_OK")
	    ->addCallback(FormEventType::ButtonClick, [this, state](Event *) {
		    auto gameState = this->state;

		    fw().stageQueueCommand({StageCmd::Command::PUSH,
		                            mksp<LoadingScreen>(
		                                gameState, enterBattle(gameState),
		                                [gameState]() { return mksp<BattleView>(gameState); },
		                                this->state->battle_common_image_list->loadingImage, 1)});
	    });

	for (int i = 12; i <= 18; i++)
	{
		bigUnitRanks.push_back(
		    fw().data->loadImage(format("PCK:xcom3/tacdata/tacbut.pck:xcom3/tacdata/"
		                                "tacbut.tab:%d:xcom3/tacdata/tactical.pal",
		                                i)));
	}
}

void BattlePreStart::updateAgents()
{
	selectedAgent = nullptr;
	agents.clear();
	// Create agent controls
	for (auto &u : state->current_battle->units)
	{
		if (u.second->owner != state->current_battle->currentPlayer)
		{
			continue;
		}
		if (!u.second->agent->type->allowsDirectControl)
		{
			continue;
		}
		if (u.second->squadNumber == -1)
		{
			continue;
		}
		agents.insert(
		    mksp<AgentIcon>(u.second->agent,
		                    ControlGenerator::createAgentControl(*state, u.second->agent,
		                                                         UnitSelectionState::Unselected),
		                    ControlGenerator::createAgentControl(
		                        *state, u.second->agent, UnitSelectionState::FirstSelected)));
	}

	// Position agent controls
	for (auto &a : agents)
	{
		a->setLocation(menuform->Location + TOP_LEFT +
		               Vec2<int>{a->agent->unit->squadPosition * SHIFT_X,
		                         a->agent->unit->squadNumber * SHIFT_Y});
		a->update();
	}

	if (lastSelectedAgent)
	{
		displayAgent(lastSelectedAgent);
	}
}

void BattlePreStart::begin()
{
	updateAgents();
	displayAgent(nullptr);
}

void BattlePreStart::pause()
{
	if (draggedAgent)
	{
		draggedAgent->agent->unit->assignToSquad(*state->current_battle, draggedOrigin);
		draggedAgent = nullptr;
	}
}

void BattlePreStart::resume() { updateAgents(); }

void BattlePreStart::finish() {}

void BattlePreStart::eventOccurred(Event *e)
{
	menuform->eventOccured(e);
	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_RETURN || e->keyboard().KeyCode == SDLK_ESCAPE ||
		    e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			menuform->findControl("BUTTON_OK")->click();
			return;
		}
		if (e->keyboard().KeyCode == SDLK_SPACE)
		{
			menuform->findControl("BUTTON_EQUIP")->click();
			return;
		}
	}
	if (e->type() == EVENT_MOUSE_MOVE)
	{
		Vec2<int> mousePos{e->mouse().X, e->mouse().Y};

		selectedAgent = nullptr;
		draggedMoved = true;
		for (auto &a : agents)
		{
			auto &c = *a->normalControl;
			if (mousePos.x >= c.Location.x && mousePos.y >= c.Location.y &&
			    mousePos.x < c.Location.x + c.Size.x && mousePos.y < c.Location.y + c.Size.y)
			{
				selectedAgent = a;
				lastSelectedAgent = selectedAgent->agent;
				displayAgent(lastSelectedAgent);
				break;
			}
		}
	}
	if (e->type() == EVENT_MOUSE_DOWN && !draggedAgent)
	{
		if (selectedAgent)
		{
			draggedAgent = selectedAgent;
			draggedOrigin = draggedAgent->agent->unit->squadNumber;
			draggedAgent->agent->unit->removeFromSquad(*state->current_battle);
			draggedAgentOffset =
			    draggedAgent->normalControl->Location - Vec2<int>{e->mouse().X, e->mouse().Y};
			draggedMoved = false;
			updateAgents();
		}
	}
	if (e->type() == EVENT_MOUSE_UP && draggedAgent)
	{
		Vec2<int> mousePos{e->mouse().X, e->mouse().Y};
		mousePos -= menuform->Location;

		if (selectedAgent)
		{
			int num = selectedAgent->agent->unit->squadNumber;
			int pos = selectedAgent->agent->unit->squadPosition;
			selectedAgent->agent->unit->removeFromSquad(*state->current_battle);
			draggedAgent->agent->unit->assignToSquad(*state->current_battle, num, pos);
			selectedAgent->agent->unit->assignToSquad(*state->current_battle, draggedOrigin);
		}
		else
		{
			int newSquad = -1;
			for (int i = 0; i < 6; i++)
			{
				if (mousePos.x >= TOP_LEFT.x - ROW_HEADER && mousePos.x < TOP_LEFT.x + ROW_WIDTH &&
				    mousePos.y >= TOP_LEFT.y + i * SHIFT_Y &&
				    mousePos.y < TOP_LEFT.y + i * SHIFT_Y + ROW_HEIGHT)
				{
					newSquad = i;
					break;
				}
			}
			if (newSquad == -1 ||
			    !draggedAgent->agent->unit->assignToSquad(*state->current_battle, newSquad))
			{
				draggedAgent->agent->unit->assignToSquad(*state->current_battle, draggedOrigin);
			}
		}

		updateAgents();
		if (!draggedMoved)
		{
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH, mksp<AEquipScreen>(state, draggedAgent->agent)});
		}
		draggedAgent = nullptr;
	}
}

void BattlePreStart::update() { menuform->update(); }

void BattlePreStart::render()
{
	menuform->render();

	for (auto &c : agents)
	{
		if (c == selectedAgent)
		{
			c->selectedControl->render();
		}
		else
		{
			c->normalControl->render();
		}
	}
	if (draggedAgent)
	{
		Vec2<int> agentPos = fw().getCursor().getPosition() + draggedAgentOffset;
		draggedAgent->setLocation(agentPos);
		draggedAgent->selectedControl->render();
	}
}

bool BattlePreStart::isTransition() { return false; }

void BattlePreStart::AgentIcon::setLocation(Vec2<int> pos)
{
	normalControl->Location = pos;
	selectedControl->Location = pos;
}

void BattlePreStart::AgentIcon::update()
{
	normalControl->update();
	selectedControl->update();
}

BattlePreStart::AgentIcon::AgentIcon(sp<Agent> agent, sp<Control> normalControl,
                                     sp<Control> selectedControl)
    : agent(agent), normalControl(normalControl), selectedControl(selectedControl)
{
}

}; // namespace OpenApoc
