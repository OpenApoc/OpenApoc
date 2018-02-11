#include "game/ui/general/agentsheet.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "forms/textedit.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/damage.h"

namespace OpenApoc
{
// FIXME: Make stats colours part of GameState
// FIXME: 'initial' colours taken from screenshot, 'current' guessed
// first element is initial and second is current
const std::pair<Colour, Colour> healthColour{{155, 4, 4}, {220, 68, 68}};
const std::pair<Colour, Colour> accuracyColour{{252, 176, 0}, {255, 240, 64}};
const std::pair<Colour, Colour> reactionsColour{{252, 176, 0}, {255, 240, 64}};
const std::pair<Colour, Colour> speedColour{{12, 156, 56}, {76, 220, 120}};
const std::pair<Colour, Colour> staminaColour{{12, 156, 56}, {76, 220, 120}};
const std::pair<Colour, Colour> braveryColour{{0, 128, 164}, {100, 192, 228}};
const std::pair<Colour, Colour> strengthColour{{140, 136, 136}, {204, 200, 200}};
const std::pair<Colour, Colour> psiEnergyColour{{192, 56, 144}, {255, 120, 208}};
const std::pair<Colour, Colour> psiAttackColour{{192, 56, 144}, {255, 120, 208}};
const std::pair<Colour, Colour> psiDefenceColour{{192, 56, 144}, {255, 120, 208}};

AgentSheet::AgentSheet(sp<Form> dstForm) : form(dstForm) {}

void AgentSheet::display(sp<Agent> item, std::vector<sp<Image>> &ranks, bool turnBased)
{
	clear();
	form->findControlTyped<TextEdit>("AGENT_NAME")->setText(item->name);
	form->findControlTyped<Graphic>("SELECTED_PORTRAIT")->setImage(item->getPortrait().photo);

	form->findControlTyped<Graphic>("SELECTED_RANK")
	    ->setImage(item->type->displayRank ? ranks[(int)item->rank] : nullptr);

	form->findControlTyped<Label>("LABEL_1")->setText(tr("Health"));
	form->findControlTyped<Graphic>("VALUE_1")->setImage(
	    createStatsBar(item->initial_stats.health, item->current_stats.health,
	                   item->modified_stats.health, 100, healthColour, {100, 4}));

	form->findControlTyped<Label>("LABEL_2")->setText(tr("Accuracy"));
	form->findControlTyped<Graphic>("VALUE_2")->setImage(
	    createStatsBar(item->initial_stats.accuracy, item->current_stats.accuracy,
	                   item->modified_stats.accuracy, 100, accuracyColour, {100, 4}));

	form->findControlTyped<Label>("LABEL_3")->setText(tr("Reactions"));
	form->findControlTyped<Graphic>("VALUE_3")->setImage(
	    createStatsBar(item->initial_stats.reactions, item->current_stats.reactions,
	                   item->modified_stats.reactions, 100, reactionsColour, {100, 4}));

	form->findControlTyped<Label>("LABEL_4")->setText(turnBased ? tr("Time Units") : tr("Speed"));
	if (turnBased)
	{
		form->findControlTyped<Graphic>("VALUE_4")->setImage(
		    createStatsBar(item->initial_stats.time_units, item->current_stats.time_units,
		                   item->modified_stats.time_units, 100, speedColour, {100, 4}));
	}
	else
	{
		form->findControlTyped<Graphic>("VALUE_4")->setImage(createStatsBar(
		    item->initial_stats.getDisplaySpeedValue(), item->current_stats.getDisplaySpeedValue(),
		    item->modified_stats.getDisplaySpeedValue(), 100, speedColour, {100, 4}));
	}

	form->findControlTyped<Label>("LABEL_5")->setText(tr("Stamina"));
	form->findControlTyped<Graphic>("VALUE_5")->setImage(createStatsBar(
	    item->initial_stats.getDisplayStaminaValue(), item->current_stats.getDisplayStaminaValue(),
	    item->modified_stats.getDisplayStaminaValue(), 100, staminaColour, {100, 4}));

	form->findControlTyped<Label>("LABEL_6")->setText(tr("Bravery"));
	form->findControlTyped<Graphic>("VALUE_6")->setImage(
	    createStatsBar(item->initial_stats.bravery, item->current_stats.bravery,
	                   item->modified_stats.bravery, 100, braveryColour, {100, 4}));

	form->findControlTyped<Label>("LABEL_7")->setText(tr("Strength"));
	form->findControlTyped<Graphic>("VALUE_7")->setImage(
	    createStatsBar(item->initial_stats.strength, item->current_stats.strength,
	                   item->modified_stats.strength, 100, strengthColour, {100, 4}));

	form->findControlTyped<Label>("LABEL_8")->setText(tr("Psi-energy"));
	form->findControlTyped<Graphic>("VALUE_8")->setImage(
	    createStatsBar(item->initial_stats.psi_energy, item->current_stats.psi_energy,
	                   item->modified_stats.psi_energy, 100, psiEnergyColour, {100, 4}));

	form->findControlTyped<Label>("LABEL_9")->setText(tr("Psi-attack"));
	form->findControlTyped<Graphic>("VALUE_9")->setImage(
	    createStatsBar(item->initial_stats.psi_attack, item->current_stats.psi_attack,
	                   item->modified_stats.psi_attack, 100, psiAttackColour, {100, 4}));

	form->findControlTyped<Label>("LABEL_10")->setText(tr("Psi-defence"));
	form->findControlTyped<Graphic>("VALUE_10")
	    ->setImage(createStatsBar(item->initial_stats.psi_defence, item->current_stats.psi_defence,
	                              item->modified_stats.psi_defence, 100, psiDefenceColour,
	                              {100, 4}));
}

void AgentSheet::clear()
{
	for (int i = 0; i < 10; i++)
	{
		auto labelName = format("LABEL_%d", i + 1);
		auto label = form->findControlTyped<Label>(labelName);
		if (!label)
		{
			LogError("Failed to find UI control matching \"%s\"", labelName);
		}
		else
		{
			label->setText("");
		}
	}
}

sp<Image> AgentSheet::createStatsBar(int initialValue, int currentValue, int modifiedValue,
                                     int maxValue, const std::pair<Colour, Colour> &colours,
                                     Vec2<int> imageSize)
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
	initialPixels = clamp(initialPixels, 0, imageSize.x - 1);
	currentPixels = clamp(currentPixels, 0, imageSize.x - 1);
	modifiedPixels = clamp(modifiedPixels, 0, imageSize.x - 1);

	sp<RGBImage> img = mksp<RGBImage>(imageSize);
	RGBImageLock l(img);

	for (int x = 0; x < currentPixels; x++)
	{
		for (int y = 0; y < imageSize.y; y++)
		{
			const Colour &col = x <= initialPixels ? colours.first : colours.second;
			// draw pixel at the top/bottom border of the bar or if filling the bar or if at the
			// right border
			if (y == 0 || y == imageSize.y - 1 || x <= modifiedPixels || x == currentPixels - 1)
			{
				l.set({x, y}, col);
			}
		}
	}

	return img;
}

}; // namespace OpenApoc
