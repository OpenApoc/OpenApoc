#include "game/ui/city/scorescreen.h"
#include "forms/form.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/radiobutton.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/city/base.h"
#include "game/state/city/facility.h"
#include "game/state/gamestate.h"
#include "game/state/playerstatesnapshot.h"
#include "game/state/shared/agent.h"
#include "game/state/shared/organisation.h"
#include "game/ui/base/recruitscreen.h"

namespace OpenApoc
{
ScoreScreen::ScoreScreen(PlayerStateSnapshot state, bool showWeeklyUpkeep)
    : Stage(), menuform(ui().getForm("city/score")), state(state), isWeeklyUpkeep(showWeeklyUpkeep)
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state.getPlayerBalance());
	menuform->findControlTyped<Label>("TEXT_DATE")->setText(state.gameTime.getLongDateString());
	menuform->findControlTyped<Label>("TEXT_WEEK")->setText(state.gameTime.getWeekString());

	formScore = menuform->findControlTyped<Form>("SCORE_VIEW");
	formFinance = menuform->findControlTyped<Form>("FINANCE_VIEW");
	title = menuform->findControlTyped<Label>("TITLE");

	auto buttonScore = menuform->findControlTyped<RadioButton>("BUTTON_SCORE");
	buttonScore->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { setScoreMode(); });

	auto buttonFinance = menuform->findControlTyped<RadioButton>("BUTTON_FINANCE");
	buttonFinance->addCallback(FormEventType::CheckBoxSelected,
	                           [this](Event *) { setFinanceMode(); });

	auto buttonOK = menuform->findControlTyped<GraphicButton>("BUTTON_OK");
	buttonOK->addCallback(FormEventType::ButtonClick,
	                      [](Event *) { fw().stageQueueCommand({StageCmd::Command::POP}); });

	if (isWeeklyUpkeep)
	{
		buttonFinance->setChecked(true);
	}
	else
	{
		buttonScore->setChecked(true);
	}
}

ScoreScreen::~ScoreScreen() = default;

/**
 * Setup the score mode.
 */
void ScoreScreen::setScoreMode()
{
	if (!formScoreFilled)
	{
		formScoreFilled = true;

		formScore->findControlTyped<Label>("TACTICAL_W")
		    ->setText(format("%d", state.weekScore.tacticalMissions));
		formScore->findControlTyped<Label>("RESEARCH_W")
		    ->setText(format("%d", state.weekScore.researchCompleted));
		formScore->findControlTyped<Label>("ALIEN_W")->setText(
		    format("%d", state.weekScore.alienIncidents));
		formScore->findControlTyped<Label>("UFO_SHOTDOWN_W")
		    ->setText(format("%d", state.weekScore.craftShotDownUFO));
		formScore->findControlTyped<Label>("CRAFT_SHOTDOWN_W")
		    ->setText(format("%d", state.weekScore.craftShotDownXCom));
		formScore->findControlTyped<Label>("INCURSIONS_W")
		    ->setText(format("%d", state.weekScore.incursions));
		formScore->findControlTyped<Label>("DAMAGE_W")
		    ->setText(format("%d", state.weekScore.cityDamage));
		formScore->findControlTyped<Label>("TOTAL_W")->setText(
		    format("%d", state.weekScore.getTotal()));

		formScore->findControlTyped<Label>("TACTICAL_T")
		    ->setText(format("%d", state.totalScore.tacticalMissions));
		formScore->findControlTyped<Label>("RESEARCH_T")
		    ->setText(format("%d", state.totalScore.researchCompleted));
		formScore->findControlTyped<Label>("ALIEN_T")->setText(
		    format("%d", state.totalScore.alienIncidents));
		formScore->findControlTyped<Label>("UFO_SHOTDOWN_T")
		    ->setText(format("%d", state.totalScore.craftShotDownUFO));
		formScore->findControlTyped<Label>("CRAFT_SHOTDOWN_T")
		    ->setText(format("%d", state.totalScore.craftShotDownXCom));
		formScore->findControlTyped<Label>("INCURSIONS_T")
		    ->setText(format("%d", state.totalScore.incursions));
		formScore->findControlTyped<Label>("DAMAGE_T")
		    ->setText(format("%d", state.totalScore.cityDamage));
		formScore->findControlTyped<Label>("TOTAL_T")->setText(
		    format("%d", state.totalScore.getTotal()));
	}

	title->setText(tr("SCORE"));
	formScore->setVisible(true);
	formFinance->setVisible(false);
}

/**
 * Setup the finance mode.
 */
void ScoreScreen::setFinanceMode()
{
	if (!formFinanceFilled)
	{
		formFinanceFilled = true;

		auto getSalary = [this](AgentType::Role role) {
			auto it = state.agent_salary.find(role);
			if (it != state.agent_salary.end())
			{
				return it->second;
			}
			return 0;
		};

		formFinance->findControlTyped<Label>("AGENTS_Q")
		    ->setText(format("%d", state.agent_qty[AgentType::Role::Soldier]));
		formFinance->findControlTyped<Label>("BIOCHEMISTS_Q")
		    ->setText(format("%d", state.agent_qty[AgentType::Role::BioChemist]));
		formFinance->findControlTyped<Label>("ENGINEERS_Q")
		    ->setText(format("%d", state.agent_qty[AgentType::Role::Engineer]));
		formFinance->findControlTyped<Label>("PHYSICISTS_Q")
		    ->setText(format("%d", state.agent_qty[AgentType::Role::Physicist]));
		formFinance->findControlTyped<Label>("TOTAL_Q")->setText(
		    format("%d", state.agent_qty[AgentType::Role::Soldier] +
		                     state.agent_qty[AgentType::Role::BioChemist] +
		                     state.agent_qty[AgentType::Role::Engineer] +
		                     state.agent_qty[AgentType::Role::Physicist]));
		formFinance->findControlTyped<Label>("BASES_TOTAL_Q")
		    ->setText(format("%d", state.playerBasesWeeklyCosts.size()));

		int soldiers =
		    getSalary(AgentType::Role::Soldier) * state.agent_qty[AgentType::Role::Soldier];
		int biochemists =
		    getSalary(AgentType::Role::BioChemist) * state.agent_qty[AgentType::Role::BioChemist];
		int engineers =
		    getSalary(AgentType::Role::Engineer) * state.agent_qty[AgentType::Role::Engineer];
		int physicists =
		    getSalary(AgentType::Role::Physicist) * state.agent_qty[AgentType::Role::Physicist];

		int agentsSalary = soldiers + biochemists + engineers + physicists;

		formFinance->findControlTyped<Label>("AGENTS_W")->setText(format("$%d", soldiers));
		formFinance->findControlTyped<Label>("BIOCHEMISTS_W")->setText(format("$%d", biochemists));
		formFinance->findControlTyped<Label>("ENGINEERS_W")->setText(format("$%d", engineers));
		formFinance->findControlTyped<Label>("PHYSICISTS_W")->setText(format("$%d", physicists));
		formFinance->findControlTyped<Label>("TOTAL_W")->setText(format("$%d", agentsSalary));

		int basesCosts = 0;

		for (const auto &[key, value] : state.playerBasesWeeklyCosts)
		{
			basesCosts += value;
		}

		formFinance->findControlTyped<Label>("BASES_TOTAL_W")->setText(format("$%d", basesCosts));
		formFinance->findControlTyped<Label>("OVERHEADS_W")
		    ->setText(format("$%d", agentsSalary + basesCosts));

		int balance = state.playerBalance;

		formFinance->findControlTyped<Label>("INITIAL")->setText(
		    format("%s $%d", tr("Initial funds>"), balance));
		formFinance->findControlTyped<Label>("REMAINING")
		    ->setText(
		        format("%s $%d", tr("Remaining funds>"), balance - agentsSalary - basesCosts));
	}

	title->setText(tr("FINANCE"));
	formScore->setVisible(false);
	formFinance->setVisible(true);
}

void ScoreScreen::begin() {}

void ScoreScreen::pause() {}

void ScoreScreen::resume() {}

void ScoreScreen::finish() {}

void ScoreScreen::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_ESCAPE:
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				menuform->findControl("BUTTON_OK")->click();
		}
		return;
	}
}

void ScoreScreen::update() { menuform->update(); }

void ScoreScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool ScoreScreen::isTransition() { return false; }

}; // namespace OpenApoc
