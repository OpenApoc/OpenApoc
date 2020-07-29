#include "game/ui/general/cheatoptions.h"
#include "forms/checkbox.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/scrollbar.h"
#include "forms/textbutton.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gamestate.h"
#include "game/state/shared/organisation.h"
#include <limits>

namespace OpenApoc
{

namespace
{
struct multiplierDescriptor
{
	UString controlName;
	float multMin;
	float multMax;
	UString optionName;
};
std::list<multiplierDescriptor> multiplierDescriptors{
    {"DAMAGE_INFLICTED_MULT", 0.0f, 5.0f, "OpenApoc.Cheat.DamageInflictedMultiplier"},
    {"DAMAGE_RECEIVED_MULT", 0.0f, 5.0f, "OpenApoc.Cheat.DamageReceivedMultiplier"},
    {"HOSTILES_MULT", 0.5f, 3.0f, "OpenApoc.Cheat.HostilesMultiplier"},
    {"STAT_GROWTH_MULT", 0.0f, 99.5f, "OpenApoc.Cheat.StatGrowthMultiplier"}};
} // namespace

CheatOptions::CheatOptions(sp<GameState> state)
    : Stage(), menuform(ui().getForm("cheatoptions")), state(state)
{
}
CheatOptions::~CheatOptions() {}

bool CheatOptions::isTransition() { return false; }

int CheatOptions::scaleMultiplierToScrollbar(float multiplierValue, float multMin, float multMax,
                                             int scrollMin, int scrollMax)
{
	return (multiplierValue - multMin) / (multMax - multMin) * (scrollMax - scrollMin) + scrollMin;
}
float CheatOptions::scaleScrollbarToMultiplier(int scrollbarValue, float multMin, float multMax,
                                               int scrollMin, int scrollMax)
{
	return (double)(scrollbarValue - scrollMin) / (double)(scrollMax - scrollMin) *
	           (multMax - multMin) +
	       multMin;
}
void CheatOptions::begin()
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	menuform->findControlTyped<CheckBox>("CHECKBOX_INFINITE_AMMO")
	    ->setChecked(config().getBool("OpenApoc.Cheat.InfiniteAmmo"));

	for (auto &desc : multiplierDescriptors)
	{
		auto bar = menuform->findControlTyped<ScrollBar>(desc.controlName);
		bar->setValue(scaleMultiplierToScrollbar(config().getFloat(desc.optionName), desc.multMin,
		                                         desc.multMax, bar->getMinimum(),
		                                         bar->getMaximum()));
		updateMultiplierText(desc.controlName, desc.multMin, desc.multMax);
	}
}

void CheatOptions::updateMultiplierText(UString controlName, float multMin, float multMax)
{
	auto bar = menuform->findControlTyped<ScrollBar>(controlName);
	auto label = menuform->findControlTyped<Label>("TEXT_" + controlName);
	label->setText(
	    format("%.0f%%", scaleScrollbarToMultiplier(bar->getValue(), multMin, multMax,
	                                                bar->getMinimum(), bar->getMaximum()) *
	                         100));
}

void CheatOptions::pause() {}

void CheatOptions::resume() {}

void CheatOptions::finish()
{
	/* Store persistent options */

	config().set("OpenApoc.Cheat.InfiniteAmmo",
	             menuform->findControlTyped<CheckBox>("CHECKBOX_INFINITE_AMMO")->isChecked());

	for (auto &desc : multiplierDescriptors)
	{
		auto bar = menuform->findControlTyped<ScrollBar>(desc.controlName);
		config().set(desc.optionName,
		             scaleScrollbarToMultiplier(bar->getValue(), desc.multMin, desc.multMax,
		                                        bar->getMinimum(), bar->getMaximum()));
	}
}

void CheatOptions::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_RETURN ||
		    e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			menuform->findControl("BUTTON_OK")->click();
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
		else if (e->forms().RaisedBy->Name == "BUTTON_GIVE_ALL_RESEARCH")
		{
			for (auto &r : this->state->research.topics)
			{
				LogWarning("Topic \"%s\"", r.first);
				auto &topic = r.second;
				if (topic->isComplete())
				{
					LogWarning("Topic \"%s\" already complete", r.first);
				}
				else
				{
					topic->forceComplete();
					LogWarning("Topic \"%s\" marked as complete", r.first);
				}
			}
			this->state->research.resortTopicList();
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_ORGS_FRIENDLY")
		{
			for (auto &r : state->organisations)
			{
				r.second->adjustRelationTo(*state, state->player,
				                           std::numeric_limits<float>::infinity());
			}
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_ORGS_HOSTILE")
		{
			for (auto &r : state->organisations)
			{
				r.second->adjustRelationTo(*state, state->player,
				                           -std::numeric_limits<float>::infinity());
			}
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_ORGS_UTOPIA")
		{
			for (auto &r : state->organisations)
			{
				for (auto &s : state->organisations)
				{
					r.second->adjustRelationTo(*state, state->getOrganisation(s.first),
					                           std::numeric_limits<float>::infinity());
				}
			}
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_ORGS_CHAOS")
		{
			for (auto &r : state->organisations)
			{
				for (auto &s : state->organisations)
				{
					r.second->adjustRelationTo(*state, state->getOrganisation(s.first),
					                           -std::numeric_limits<float>::infinity());
				}
			}
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_MODIFY_FUNDS")
		{
			state->getPlayer()->balance +=
			    menuform->findControlTyped<ScrollBar>("MODIFY_FUNDS")->getValue() * 1000;
			menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_FAST_FORWARD_DAY")
		{
			state->gameTime.addTicks(TICKS_PER_DAY);
			// state->gameTime.setDayPassed( true);
			LogWarning("Scheduling end of day");
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_FAST_FORWARD_WEEK")
		{
			state->gameTime.addTicks(TICKS_PER_DAY * 7);
			//	state->gameTime.setWeekPassed( true);
			LogWarning("Scheduling end of week");
		}
	}
	if (e->type() == EVENT_FORM_INTERACTION &&
	    e->forms().EventFlag == FormEventType::ScrollBarChange)
	{
		if (e->forms().RaisedBy->Name == "MODIFY_FUNDS")
		{
			auto bar = std::dynamic_pointer_cast<ScrollBar>(e->forms().RaisedBy);
			if (!bar)
			{
				LogError("Failed to cast \"%s\" control to ScrollBar", e->forms().RaisedBy->Name);
				return;
			}
			menuform->findControlTyped<Label>("TEXT_MODIFY_FUNDS")
			    ->setText(format("%+dk", bar->getValue()));
		}
		else
		{
			for (auto &desc : multiplierDescriptors)
			{
				if (e->forms().RaisedBy->Name == desc.controlName)
				{
					auto bar = std::dynamic_pointer_cast<ScrollBar>(e->forms().RaisedBy);
					if (!bar)
					{
						LogError("Failed to cast \"%s\" control to ScrollBar",
						         e->forms().RaisedBy->Name);
						return;
					}
					updateMultiplierText(desc.controlName, desc.multMin, desc.multMax);
					break;
				}
			}
		}
	}
}

void CheatOptions::update() { menuform->update(); }

void CheatOptions::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}
} // namespace OpenApoc
