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
#include <sstream>

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
const Colour bkgColour{36, 36, 36};

AgentSheet::AgentSheet(sp<Form> profileForm, sp<Form> statsForm)
    : profileForm(profileForm), statsForm(statsForm)
{
}

AgentSheet::AgentSheet(sp<GameState> state, sp<Form> profileForm, sp<Form> statsForm,
                       sp<Form> historyForm)
    : profileForm(profileForm), statsForm(statsForm), historyForm(historyForm), state(state)
{
}

void AgentSheet::display(const Agent &item, std::vector<sp<Image>> &ranks, bool turnBased)
{
	clear();
	displayProfile(item, ranks);
	displayStats(item, ranks, turnBased);
	if (historyForm)
		displayHistory(item);
}

void AgentSheet::displayProfile(const Agent &item, std::vector<sp<Image>> &ranks)
{
	profileForm->findControlTyped<TextEdit>("AGENT_NAME")->setText(item.name);
	profileForm->findControlTyped<Graphic>("SELECTED_PORTRAIT")->setImage(item.getPortrait().photo);

	profileForm->findControlTyped<Graphic>("SELECTED_RANK")
	    ->setImage(item.type->displayRank ? ranks[(int)item.rank] : nullptr);
	profileForm->findControlTyped<Graphic>("SELECTED_RANK")->ToolTipText = item.getRankName();
}

void AgentSheet::displayHistory(const Agent &item)
{

	historyForm->findControlTyped<Label>("LABEL_DAYS_IN_SERVICE")->setText(tr("Days In Service"));
	std::stringstream daysInService;
	daysInService << item.getDaysInService(*state);
	historyForm->findControlTyped<Label>("VALUE_DAYS_IN_SERVICE")->setText(daysInService.str());

	historyForm->findControlTyped<Label>("LABEL_MISSION_COUNT")->setText(tr("Missions"));
	std::stringstream missionsCount;
	missionsCount << item.getMissions();
	historyForm->findControlTyped<Label>("VALUE_MISSION_COUNT")->setText(missionsCount.str());

	historyForm->findControlTyped<Label>("LABEL_KILL_COUNT")->setText(tr("Kills"));
	std::stringstream killCount;
	killCount << item.getKills();
	historyForm->findControlTyped<Label>("VALUE_KILL_COUNT")->setText(killCount.str());

	for (unsigned int i = 5; i > item.getMedalTier(); i--)
	{
		auto formLabel = format("MEDAL_%d", i);
		auto medalFormElement = historyForm->findControlTyped<Graphic>(formLabel);
		medalFormElement->setVisible(false);
	}
}

void AgentSheet::displayStats(const Agent &item, std::vector<sp<Image>> &ranks, bool turnBased)
{

	statsForm->findControlTyped<Label>("LABEL_1")->setText(tr("Health"));
	statsForm->findControlTyped<Graphic>("VALUE_1")->setImage(
	    createStatsBar(item.initial_stats.health, item.current_stats.health,
	                   item.modified_stats.health, 100, healthColour, {88, 7}));
	statsForm->findControlTyped<Graphic>("VALUE_1")->ToolTipText =
	    statsForm->findControlTyped<Label>("LABEL_1")->getText() +
	    format(": %d/%d", item.modified_stats.health, item.current_stats.health);

	statsForm->findControlTyped<Label>("LABEL_2")->setText(tr("Accuracy"));
	statsForm->findControlTyped<Graphic>("VALUE_2")->setImage(
	    createStatsBar(item.initial_stats.accuracy, item.current_stats.accuracy,
	                   item.modified_stats.accuracy, 100, accuracyColour, {88, 7}));
	statsForm->findControlTyped<Graphic>("VALUE_2")->ToolTipText =
	    statsForm->findControlTyped<Label>("LABEL_2")->getText() +
	    format(": %d/%d", item.modified_stats.accuracy, item.current_stats.accuracy);

	statsForm->findControlTyped<Label>("LABEL_3")->setText(tr("Reactions"));
	statsForm->findControlTyped<Graphic>("VALUE_3")->setImage(
	    createStatsBar(item.initial_stats.reactions, item.current_stats.reactions,
	                   item.modified_stats.reactions, 100, reactionsColour, {88, 7}));
	statsForm->findControlTyped<Graphic>("VALUE_3")->ToolTipText =
	    statsForm->findControlTyped<Label>("LABEL_3")->getText() +
	    format(": %d/%d", item.modified_stats.reactions, item.current_stats.reactions);

	statsForm->findControlTyped<Label>("LABEL_4")->setText(turnBased ? tr("Time Units")
	                                                                 : tr("Speed"));
	statsForm->findControlTyped<Graphic>("VALUE_4")->ToolTipText =
	    statsForm->findControlTyped<Label>("LABEL_4")->getText();
	if (turnBased)
	{
		statsForm->findControlTyped<Graphic>("VALUE_4")->setImage(
		    createStatsBar(item.initial_stats.time_units, item.current_stats.time_units,
		                   item.modified_stats.time_units, 100, speedColour, {88, 7}));
		statsForm->findControlTyped<Graphic>("VALUE_4")->ToolTipText +=
		    format(": %d/%d", item.modified_stats.time_units, item.current_stats.time_units);
	}
	else
	{
		statsForm->findControlTyped<Graphic>("VALUE_4")->setImage(createStatsBar(
		    item.initial_stats.getDisplaySpeedValue(), item.current_stats.getDisplaySpeedValue(),
		    item.modified_stats.getDisplaySpeedValue(), 100, speedColour, {88, 7}));
		statsForm->findControlTyped<Graphic>("VALUE_4")->ToolTipText +=
		    format("^ %d/%d", item.modified_stats.getDisplaySpeedValue(),
		           item.current_stats.getDisplaySpeedValue());
	}

	statsForm->findControlTyped<Label>("LABEL_5")->setText(tr("Stamina"));
	statsForm->findControlTyped<Graphic>("VALUE_5")->setImage(createStatsBar(
	    item.initial_stats.getDisplayStaminaValue(), item.current_stats.getDisplayStaminaValue(),
	    item.modified_stats.getDisplayStaminaValue(), 100, staminaColour, {88, 7}));
	statsForm->findControlTyped<Graphic>("VALUE_5")->ToolTipText =
	    statsForm->findControlTyped<Label>("LABEL_5")->getText() +
	    format(": %d/%d", item.modified_stats.getDisplayStaminaValue(),
	           item.current_stats.getDisplayStaminaValue());

	statsForm->findControlTyped<Label>("LABEL_6")->setText(tr("Bravery"));
	statsForm->findControlTyped<Graphic>("VALUE_6")->setImage(
	    createStatsBar(item.initial_stats.bravery, item.current_stats.bravery,
	                   item.modified_stats.bravery, 100, braveryColour, {88, 7}));
	statsForm->findControlTyped<Graphic>("VALUE_6")->ToolTipText =
	    statsForm->findControlTyped<Label>("LABEL_6")->getText() +
	    format(": %d/%d", item.modified_stats.bravery, item.current_stats.bravery);

	statsForm->findControlTyped<Label>("LABEL_7")->setText(tr("Strength"));
	statsForm->findControlTyped<Graphic>("VALUE_7")->setImage(
	    createStatsBar(item.initial_stats.strength, item.current_stats.strength,
	                   item.modified_stats.strength, 100, strengthColour, {88, 7}));
	statsForm->findControlTyped<Graphic>("VALUE_7")->ToolTipText =
	    statsForm->findControlTyped<Label>("LABEL_7")->getText() +
	    format(": %d/%d", item.modified_stats.strength, item.current_stats.strength);

	statsForm->findControlTyped<Label>("LABEL_8")->setText(tr("Psi-energy"));
	statsForm->findControlTyped<Graphic>("VALUE_8")->setImage(
	    createStatsBar(item.initial_stats.psi_energy, item.current_stats.psi_energy,
	                   item.modified_stats.psi_energy, 100, psiEnergyColour, {88, 7}));
	statsForm->findControlTyped<Graphic>("VALUE_8")->ToolTipText =
	    statsForm->findControlTyped<Label>("LABEL_8")->getText() +
	    format(": %d/%d", item.modified_stats.psi_energy, item.current_stats.psi_energy);

	statsForm->findControlTyped<Label>("LABEL_9")->setText(tr("Psi-attack"));
	statsForm->findControlTyped<Graphic>("VALUE_9")->setImage(
	    createStatsBar(item.initial_stats.psi_attack, item.current_stats.psi_attack,
	                   item.modified_stats.psi_attack, 100, psiAttackColour, {88, 7}));
	statsForm->findControlTyped<Graphic>("VALUE_9")->ToolTipText =
	    statsForm->findControlTyped<Label>("LABEL_9")->getText() +
	    format(": %d/%d", item.modified_stats.psi_attack, item.current_stats.psi_attack);

	statsForm->findControlTyped<Label>("LABEL_10")->setText(tr("Psi-defence"));
	statsForm->findControlTyped<Graphic>("VALUE_10")
	    ->setImage(createStatsBar(item.initial_stats.psi_defence, item.current_stats.psi_defence,
	                              item.modified_stats.psi_defence, 100, psiDefenceColour, {88, 7}));
	statsForm->findControlTyped<Graphic>("VALUE_10")->ToolTipText =
	    statsForm->findControlTyped<Label>("LABEL_10")->getText() +
	    format(": %d/%d", item.modified_stats.psi_defence, item.current_stats.psi_defence);
}

void AgentSheet::clear()
{
	for (int i = 0; i < 10; i++)
	{
		auto labelName = format("LABEL_%d", i + 1);
		auto label = statsForm->findControlTyped<Label>(labelName);
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

	for (int x = 0; x < imageSize.x; x++)
	{
		for (int y = 0; y < imageSize.y; y++)
		{
			{
				const Colour &col = x <= initialPixels ? colours.first : colours.second;
				if (x < currentPixels && (y == 0 || y == imageSize.y - 1 || x <= modifiedPixels ||
				                          x == currentPixels - 1))
				{
					l.set({x, y}, col);
				}
				else
					l.set({x, y}, bkgColour);
			}
		}
	}
	return img;
}

}; // namespace OpenApoc
