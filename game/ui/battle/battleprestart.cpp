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
#include "game/state/aequipment.h"
#include "game/state/agent.h"
#include "game/state/battle/battlecommonimagelist.h"
#include "game/state/gamestate.h"
#include "game/ui/base/aequipscreen.h"
#include "game/ui/battle/battleview.h"
#include "game/ui/general/loadingscreen.h"
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
	bool visible = agent ? true : false;
	menuform->findControlTyped<Label>("LABEL_HEALTH")->setVisible(visible);
	menuform->findControlTyped<Label>("LABEL_ACCURACY")->setVisible(visible);
	menuform->findControlTyped<Label>("LABEL_REACTIONS")->setVisible(visible);
	menuform->findControlTyped<Label>("LABEL_SPEED")->setVisible(visible);
	menuform->findControlTyped<Label>("LABEL_STAMINA")->setVisible(visible);
	menuform->findControlTyped<Label>("LABEL_BRAVERY")->setVisible(visible);
	menuform->findControlTyped<Label>("LABEL_STRENGTH")->setVisible(visible);
	menuform->findControlTyped<Label>("LABEL_PSI-ENERGY")->setVisible(visible);
	menuform->findControlTyped<Label>("LABEL_PSI-ATTACK")->setVisible(visible);
	menuform->findControlTyped<Label>("LABEL_PSI-DEFENCE")->setVisible(visible);

	if (!visible)
	{
		return;
	}

	AEquipScreen::outputAgent(agent, menuform, bigUnitRanks,
	                          state->current_battle->mode == Battle::Mode::TurnBased);

	auto rHand = agent->getFirstItemInSlot(EquipmentSlotType::RightHand);
	auto lHand = agent->getFirstItemInSlot(EquipmentSlotType::LeftHand);
	menuform->findControlTyped<Graphic>("RIGHT_HAND")
	    ->setImage(rHand ? rHand->type->equipscreen_sprite : nullptr);
	menuform->findControlTyped<Graphic>("LEFT_HAND")
	    ->setImage(lHand ? lHand->type->equipscreen_sprite : nullptr);
}

sp<Control> BattlePreStart::createAgentControl(StateRef<Agent> agent, bool selected)
{
	auto baseControl = mksp<Graphic>();
	baseControl->Name = "AGENT_PORTRAIT";
	baseControl->Size = unitSelect[0]->size;
	baseControl->setImage(unitSelect[selected ? 2 : 0]);

	auto icon = agent->getPortrait().icon;
	auto photoGraphic = baseControl->createChild<Graphic>(icon);
	photoGraphic->Size = icon->size;
	photoGraphic->Location = {1, 1};

	auto rankIcon = baseControl->createChild<Graphic>(unitRanks[(int)agent->rank]);
	rankIcon->AutoSize = true;
	rankIcon->Location = {0, 0};

	bool shield = agent->getMaxShield() > 0;

	float maxHealth;
	float currentHealth;
	if (shield)
	{
		currentHealth = agent->getShield();
		maxHealth = agent->getMaxShield();
	}
	else
	{
		currentHealth = agent->getHealth();
		maxHealth = agent->getMaxHealth();
	}
	float healthProportion = maxHealth == 0.0f ? 0.0f : currentHealth / maxHealth;

	if (healthProportion > 0.0f)
	{
		// FIXME: Put these somewhere slightly less magic?
		Vec2<int> healthBarOffset = {27, 2};
		Vec2<int> healthBarSize = {3, 20};

		auto healthImg = shield ? this->shieldImage : this->healthImage;
		auto healthGraphic = baseControl->createChild<Graphic>(healthImg);
		// This is a bit annoying as the health bar starts at the bottom, but the coord origin is
		// top-left, so fix that up a bit
		int healthBarHeight = (int)((float)healthBarSize.y * healthProportion);
		healthBarOffset.y = healthBarOffset.y + (healthBarSize.y - healthBarHeight);
		healthBarSize.y = healthBarHeight;
		healthGraphic->Location = healthBarOffset;
		healthGraphic->Size = healthBarSize;
		healthGraphic->ImagePosition = FillMethod::Stretch;
	}

	return baseControl;
}

BattlePreStart::BattlePreStart(sp<GameState> state)
    : Stage(), menuform(ui().getForm("battle/prestart")), TOP_LEFT({302, 80}), state(state)
{

	menuform->findControlTyped<GraphicButton>("BUTTON_EQUIP")
	    ->addCallback(FormEventType::ButtonClick, [this, state](Event *) {

		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<AEquipScreen>(state)});
		});
	menuform->findControlTyped<GraphicButton>("BUTTON_OK")
	    ->addCallback(FormEventType::ButtonClick, [this, state](Event *) {

		    auto gameState = this->state;

		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH,
		         mksp<LoadingScreen>(gameState, enterBattle(gameState),
		                             [gameState]() { return mksp<BattleView>(gameState); },
		                             this->state->battle_common_image_list->loadingImage, 1)});
		});

	auto img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({0, 0}, Colour{255, 255, 219});
		l.set({0, 1}, Colour{215, 0, 0});
	}
	this->healthImage = img;
	img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({0, 0}, Colour{160, 236, 252});
		l.set({0, 1}, Colour{4, 100, 252});
	}
	this->shieldImage = img;
	for (int i = 28; i <= 34; i++)
	{
		unitRanks.push_back(
		    fw().data->loadImage(format("PCK:xcom3/tacdata/tacbut.pck:xcom3/tacdata/"
		                                "tacbut.tab:%d:xcom3/tacdata/tactical.pal",
		                                i)));
	}
	for (int i = 12; i <= 18; i++)
	{
		bigUnitRanks.push_back(
		    fw().data->loadImage(format("PCK:xcom3/tacdata/tacbut.pck:xcom3/tacdata/"
		                                "tacbut.tab:%d:xcom3/tacdata/tactical.pal",
		                                i)));
	}
	unitSelect.push_back(fw().data->loadImage(
	    "PCK:xcom3/ufodata/vs_icon.pck:xcom3/ufodata/vs_icon.tab:37:xcom3/ufodata/pal_01.dat"));
	unitSelect.push_back(fw().data->loadImage("battle/battle-icon-38.png"));
	unitSelect.push_back(fw().data->loadImage("battle/battle-icon-39.png"));
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
		agents.insert(mksp<AgentControl>(u.second->agent,
		                                 createAgentControl(u.second->agent, false),
		                                 createAgentControl(u.second->agent, true)));
	}

	// Position agnet controls
	for (auto &a : agents)
	{
		a->setLocation(menuform->Location + TOP_LEFT +
		               Vec2<int>{a->agent->unit->squadPosition * SHIFT_X,
		                         a->agent->unit->squadNumber * SHIFT_Y});
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
		if (e->keyboard().KeyCode == SDLK_RETURN || e->keyboard().KeyCode == SDLK_ESCAPE)
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
			for (int i = 0; i < 5; i++)
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

void BattlePreStart::AgentControl::setLocation(Vec2<int> pos)
{
	normalControl->Location = pos;
	selectedControl->Location = pos;
}

BattlePreStart::AgentControl::AgentControl(sp<Agent> agent, sp<Control> normalControl,
                                           sp<Control> selectedControl)
    : agent(agent), normalControl(normalControl), selectedControl(selectedControl)
{
}

}; // namespace OpenApoc
