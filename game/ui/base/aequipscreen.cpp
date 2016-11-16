#include "game/ui/base/aequipscreen.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

static sp<Image> createStatsBar(int initialValue, int currentValue, int modifiedValue, int maxValue,
                                Colour &initialColour, Colour &currentColour, Vec2<int> imageSize)
{
	// Some agent types (e.g. Android's Psi-attack) have zero values. Break out here to avoid
	// dividing by zero.
	if (initialValue == 0 && currentValue == 0 && modifiedValue == 0 && maxValue == 0)
	{
		return mksp<RGBImage>(imageSize);
	}
	LogAssert(initialValue >= 0);
	LogAssert(currentValue >= 0);
	LogAssert(currentValue >= initialValue);
	LogAssert(modifiedValue >= 0);
	LogAssert(maxValue > 0);
	// Need at least 3 y pixels for the 'hollow' bar
	LogAssert(imageSize.x > 0 && imageSize.y > 2);

	int initialPixels = (float)imageSize.x * ((float)initialValue / (float)maxValue);
	int currentPixels = (float)imageSize.x * ((float)currentValue / (float)maxValue);
	int modifiedPixels = (float)imageSize.x * ((float)modifiedValue / (float)maxValue);

	sp<RGBImage> img = mksp<RGBImage>(imageSize);

	int x = 0;

	RGBImageLock l(img);

	for (; x < currentPixels; x++)
	{
		if (x <= initialPixels)
		{
			// Draw a 'hollow' line if we're > modifiedPixels (and not the end)
			if (x > modifiedPixels && x < currentPixels - 1)
			{
				l.set({x, 0}, initialColour);
				l.set({x, imageSize.y - 1}, initialColour);
			}
			else
			{
				for (int y = 0; y < imageSize.y; y++)
					l.set({x, y}, initialColour);
			}
		}
		else
		{
			// Draw a 'hollow' line if we're > modifiedPixels (and not the end)
			if (x > modifiedPixels && x < currentPixels - 1)
			{
				l.set({x, 0}, initialColour);
				l.set({x, imageSize.y - 1}, currentColour);
			}
			else
			{
				for (int y = 0; y < imageSize.y; y++)
					l.set({x, y}, currentColour);
			}
		}
	}

	return img;
}

AEquipScreen::AEquipScreen(sp<GameState> state)
    : Stage(), form(ui().getForm("aequipscreen")),
      pal(fw().data->loadPalette("xcom3/ufodata/agenteqp.pcx")), state(state)

{
}

AEquipScreen::~AEquipScreen() = default;

void AEquipScreen::begin()
{
	form->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	for (auto &agent : state->agents)
	{
		if (agent.second->type->role == AgentType::Role::Soldier)
		{
			this->setSelectedAgent(agent.second);
			break;
		}
	}
}

void AEquipScreen::pause() {}

void AEquipScreen::resume() {}

void AEquipScreen::finish() {}

void AEquipScreen::eventOccurred(Event *e)
{
	form->eventOccured(e);

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

		if (e->forms().RaisedBy->Name == "BUTTON_OK")
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}
}

void AEquipScreen::update() { form->update(); }

void AEquipScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	fw().renderer->setPalette(this->pal);
	form->render();
}

bool AEquipScreen::isTransition() { return false; }

void AEquipScreen::setSelectedAgent(sp<Agent> agent)
{
	this->currentAgent = agent;

	this->form->findControlTyped<Label>("AGENT_NAME")->setText(agent->name);
	this->form->findControlTyped<Graphic>("SELECTED_PORTRAIT")
	    ->setImage(agent->getPortrait().photo);
	// FIXME: Make stats colours part of GameState
	// FIXME: 'initial' colours taken from screenshot, 'current' guessed
	Colour healthInitialColour{156, 4, 4};
	Colour healthCurrentColour{220, 68, 68};
	this->form->findControlTyped<Graphic>("VALUE_1")->setImage(createStatsBar(
	    agent->initial_stats.health, agent->current_stats.health, agent->modified_stats.health,
	    agent->type->max_stats.health, healthInitialColour, healthCurrentColour, {64, 4}));
	Colour accuracyInitialColour{252, 176, 0};
	Colour accuracyCurrentColour{255, 240, 64};
	this->form->findControlTyped<Graphic>("VALUE_2")->setImage(
	    createStatsBar(agent->initial_stats.accuracy, agent->current_stats.accuracy,
	                   agent->modified_stats.accuracy, agent->type->max_stats.accuracy,
	                   accuracyInitialColour, accuracyCurrentColour, {64, 4}));
	Colour reactionsInitialColour{252, 176, 0};
	Colour reactionsCurrentColour{255, 240, 64};
	this->form->findControlTyped<Graphic>("VALUE_3")->setImage(
	    createStatsBar(agent->initial_stats.reactions, agent->current_stats.reactions,
	                   agent->modified_stats.reactions, agent->type->max_stats.reactions,
	                   reactionsInitialColour, reactionsCurrentColour, {64, 4}));
	Colour speedInitialColour{12, 156, 56};
	Colour speedCurrentColour{76, 220, 120};
	this->form->findControlTyped<Graphic>("VALUE_4")->setImage(createStatsBar(
	    agent->initial_stats.speed, agent->current_stats.speed, agent->modified_stats.speed,
	    agent->type->max_stats.speed, speedInitialColour, speedCurrentColour, {64, 4}));
	Colour staminaInitialColour{12, 156, 56};
	Colour staminaCurrentColour{76, 220, 120};
	this->form->findControlTyped<Graphic>("VALUE_5")->setImage(createStatsBar(
	    agent->initial_stats.stamina, agent->current_stats.stamina, agent->modified_stats.stamina,
	    agent->type->max_stats.stamina, staminaInitialColour, staminaCurrentColour, {64, 4}));
	Colour braveryInitialColour{0, 128, 164};
	Colour braveryCurrentColour{64, 192, 228};
	this->form->findControlTyped<Graphic>("VALUE_6")->setImage(createStatsBar(
	    agent->initial_stats.bravery, agent->current_stats.bravery, agent->modified_stats.bravery,
	    agent->type->max_stats.bravery, braveryInitialColour, braveryCurrentColour, {64, 4}));
	Colour strengthInitialColour{140, 136, 136};
	Colour strengthCurrentColour{204, 200, 200};
	this->form->findControlTyped<Graphic>("VALUE_7")->setImage(
	    createStatsBar(agent->initial_stats.strength, agent->current_stats.strength,
	                   agent->modified_stats.strength, agent->type->max_stats.strength,
	                   strengthInitialColour, strengthCurrentColour, {64, 4}));
	Colour psi_energyInitialColour{192, 56, 144};
	Colour psi_energyCurrentColour{255, 120, 208};
	this->form->findControlTyped<Graphic>("VALUE_8")->setImage(
	    createStatsBar(agent->initial_stats.psi_energy, agent->current_stats.psi_energy,
	                   agent->modified_stats.psi_energy, agent->type->max_stats.psi_energy,
	                   psi_energyInitialColour, psi_energyCurrentColour, {64, 4}));
	Colour psi_attackInitialColour{192, 56, 144};
	Colour psi_attackCurrentColour{255, 120, 208};
	this->form->findControlTyped<Graphic>("VALUE_9")->setImage(
	    createStatsBar(agent->initial_stats.psi_attack, agent->current_stats.psi_attack,
	                   agent->modified_stats.psi_attack, agent->type->max_stats.psi_attack,
	                   psi_attackInitialColour, psi_attackCurrentColour, {64, 4}));
	Colour psi_defenceInitialColour{192, 56, 144};
	Colour psi_defenceCurrentColour{255, 120, 208};
	this->form->findControlTyped<Graphic>("VALUE_10")
	    ->setImage(
	        createStatsBar(agent->initial_stats.psi_defence, agent->current_stats.psi_defence,
	                       agent->modified_stats.psi_defence, agent->type->max_stats.psi_defence,
	                       psi_defenceInitialColour, psi_defenceCurrentColour, {64, 4}));
}

} // namespace OpenApoc
