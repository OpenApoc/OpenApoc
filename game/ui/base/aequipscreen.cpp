#include "game/ui/base/aequipscreen.h"
#include "forms/form.h"
#include "framework/font.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "game/state/battle/battleunit.h"
#include "forms/list.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "game/state/gamestate.h"
#include "game/ui/equipscreen.h"

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
const Vec2<int> AEquipScreen::EQUIP_GRID_SLOT_SIZE{16, 16};
const Vec2<int> AEquipScreen::EQUIP_GRID_SLOTS{16, 16};

AEquipScreen::AEquipScreen(sp<GameState> state)
    : Stage(), form(ui().getForm("aequipscreen")),
      pal(fw().data->loadPalette("xcom3/ufodata/agenteqp.pcx")), state(state)

{
	this->state = state;

	auto paperDollPlaceholder = form->findControlTyped<Graphic>("PAPER_DOLL");

	this->paperDoll = form->createChild<EquipmentPaperDoll>(
	    paperDollPlaceholder->Location, paperDollPlaceholder->Size, EQUIP_GRID_SLOT_SIZE);

	auto img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({ 0, 0 }, Colour{ 255, 255, 219 });
		l.set({ 0, 1 }, Colour{ 215, 0, 0 });
	}
	this->healthImage = img;
	img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({ 0, 0 }, Colour{ 160, 236, 252 });
		l.set({ 0, 1 }, Colour{ 4, 100, 252 });
	}
	this->shieldImage = img;
	img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({ 0, 0 }, Colour{ 150, 150, 150 });
		l.set({ 0, 1 }, Colour{ 97, 101, 105 });
	}
	this->stunImage = img;
	this->iconShade = fw().data->loadImage("battle/battle-icon-shade.png");
	for (int i = 28; i <= 34; i++)
	{
		unitRanks.push_back(fw().data->loadImage(format("PCK:xcom3/tacdata/tacbut.pck:xcom3/tacdata/"
			"tacbut.tab:%d:xcom3/tacdata/tactical.pal",
			i)));
	}
	for (int i = 12; i <= 18; i++)
	{
		bigUnitRanks.push_back(fw().data->loadImage(format("PCK:xcom3/tacdata/tacbut.pck:xcom3/tacdata/"
			"tacbut.tab:%d:xcom3/tacdata/tactical.pal",
			i)));
	}
}

AEquipScreen::~AEquipScreen() = default;

void AEquipScreen::begin()
{
	form->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	// TODO: Implement agent selection based on who the screen is called on
	for (auto &agent : state->agents)
	{
		if (agent.second->type->role != AgentType::Role::Soldier)
		{
			continue;
		}
		if (agent.second->unit && !agent.second->unit->isConscious())
		{
			continue;
		}
		this->setSelectedAgent(agent.second);
		break;
	}

	// Agent list functionality
	auto agentList = form->findControlTyped<ListBox>("AGENT_SELECT_BOX");
	agentList->addCallback(FormEventType::ListBoxChangeSelected, [this](Event *e) {
		LogWarning("agent selected");
		auto list = std::static_pointer_cast<ListBox>(e->forms().RaisedBy);
		auto agent = list->getSelectedData<Agent>();
		if (!agent)
		{
			LogError("No agent in selected data");
			return;
		}
		if (agent->unit && !agent->unit->isConscious())
		{
			LogWarning("Cannot select unconscious agent");
			return;
		}
		this->setSelectedAgent(agent);
	});

	// Populate agent list
	auto font = ui().getFont("smalfont");
	auto agentEntryHeight = font->getFontHeight() * 2;
	agentList->clear();
	auto owner = state->getPlayer();
	if (state->current_battle)
	{
		owner = state->current_battle->currentPlayer;
	}
	for (auto &agent : state->agents)
	{
		if (agent.second->owner != owner)
		{
			continue;
		}
		if (agent.second->type->role != AgentType::Role::Soldier)
		{
			continue;
		}
		// Unit is not participating in battle
		if (state->current_battle && !agent.second->unit)
		{
			continue;
		}

		auto agentControl =
			this->createAgentControl({ 130, agentEntryHeight }, { state.get(), agent.second });
		agentList->addItem(agentControl);
	}
	agentList->ItemSize = agentEntryHeight;

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
	this->paperDoll->setObject(agent);

	this->form->findControlTyped<Label>("AGENT_NAME")->setText(agent->name);
	this->form->findControlTyped<Graphic>("SELECTED_PORTRAIT")
	    ->setImage(agent->getPortrait().photo);
	this->form->findControlTyped<Graphic>("SELECTED_RANK")
		->setImage(bigUnitRanks[(int)agent->rank]);
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
	    agent->initial_stats.getDisplayStaminaValue(), agent->current_stats.getDisplayStaminaValue(), agent->modified_stats.getDisplayStaminaValue(),
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

// FIXME: Put this in the rules somewhere?
// FIXME: This could be shared with the citview ICON_RESOURCES?
static const UString agentFramePath =
"PCK:xcom3/ufodata/vs_icon.pck:xcom3/ufodata/vs_icon.tab:37:xcom3/ufodata/pal_01.dat";

sp<Control> AEquipScreen::createAgentControl(Vec2<int> size, StateRef<Agent> agent)
{
	auto baseControl = mksp<Control>();
	baseControl->setData(agent.getSp());
	baseControl->Name = "AGENT_PORTRAIT";
	baseControl->Size = size;

	auto frameGraphic = baseControl->createChild<Graphic>(fw().data->loadImage(agentFramePath));
	frameGraphic->AutoSize = true;
	frameGraphic->Location = { 0, 0 };
	auto photoGraphic = frameGraphic->createChild<Graphic>(agent->getPortrait().icon);
	photoGraphic->AutoSize = true;
	photoGraphic->Location = { 1, 1 };

	// TODO: Fade portraits
	bool faded = false;

	if (faded)
	{
		auto fadeIcon = baseControl->createChild<Graphic>(iconShade);
		fadeIcon->AutoSize = true;
		fadeIcon->Location = { 2, 1 };
	}

	auto rankIcon = baseControl->createChild<Graphic>(unitRanks[(int)agent->rank]);
	rankIcon->AutoSize = true;
	rankIcon->Location = { 0, 0 };

	bool shield = agent->getMaxShield() > 0;
	
	float maxHealth;
	float currentHealth;
	float stunProportion;
	if (shield)
	{
		currentHealth = agent->getShield();
		maxHealth = agent->getMaxShield();
	}
	else
	{
		currentHealth = agent->getHealth();
		maxHealth = agent->getMaxHealth();
		if (agent->unit)
		{ 
			float stunHealth = agent->unit->stunDamage;
			stunProportion = stunHealth / maxHealth;
		}
	}
	float healthProportion = maxHealth == 0.0f ? 0.0f : currentHealth / maxHealth;
	stunProportion = clamp(stunProportion, 0.0f, healthProportion);
	
	if (healthProportion > 0.0f)
	{
		// FIXME: Put these somewhere slightly less magic?
		Vec2<int> healthBarOffset = { 27, 2 };
		Vec2<int> healthBarSize = { 3, 20 };

		auto healthImg = shield ? this->shieldImage : this->healthImage;
		auto healthGraphic = frameGraphic->createChild<Graphic>(healthImg);
		// This is a bit annoying as the health bar starts at the bottom, but the coord origin is
		// top-left, so fix that up a bit
		int healthBarHeight = (int)((float)healthBarSize.y * healthProportion);
		healthBarOffset.y = healthBarOffset.y + (healthBarSize.y - healthBarHeight);
		healthBarSize.y = healthBarHeight;
		healthGraphic->Location = healthBarOffset;
		healthGraphic->Size = healthBarSize;
		healthGraphic->ImagePosition = FillMethod::Stretch;
	}
	if (stunProportion > 0.0f)
	{
		// FIXME: Put these somewhere slightly less magic?
		Vec2<int> healthBarOffset = { 27, 2 };
		Vec2<int> healthBarSize = { 3, 20 };

		auto healthImg = this->stunImage;
		auto healthGraphic = frameGraphic->createChild<Graphic>(healthImg);
		// This is a bit annoying as the health bar starts at the bottom, but the coord origin is
		// top-left, so fix that up a bit
		int healthBarHeight = (int)((float)healthBarSize.y * stunProportion);
		healthBarOffset.y = healthBarOffset.y + (healthBarSize.y - healthBarHeight);
		healthBarSize.y = healthBarHeight;
		healthGraphic->Location = healthBarOffset;
		healthGraphic->Size = healthBarSize;
		healthGraphic->ImagePosition = FillMethod::Stretch;
	}
	
	auto font = ui().getFont("smalfont");

	auto nameLabel = baseControl->createChild<Label>(agent->name, font);
	nameLabel->Location = { 40, 0 };
	nameLabel->Size = { 100, font->getFontHeight() * 2 };

	return baseControl;
}

} // namespace OpenApoc
