#include "game/ui/general/agenthistorysheet.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include <sstream>

namespace OpenApoc
{

AgentHistorySheet::AgentHistorySheet(sp<Form> historyForm, sp<GameState> state)
    : historyForm(historyForm), state(state)
{
}

void AgentHistorySheet::display(const Agent &item)
{
	historyForm->findControlTyped<Label>("LABEL_DAYS_IN_SERVICE")->setText(tr("Days service"));
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

	for (int i = 5; i > (int)item.getMedalTier(); i--)
	{
		auto formLabel = format("MEDAL_%d", i);
		auto medalFromElement = historyForm->findControlTyped<Graphic>(formLabel);
		medalFromElement->setVisible(false);
	}
}

} // namespace OpenApoc
