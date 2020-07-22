#include "game/ui/skirmish/selectforces.h"
#include "forms/checkbox.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gamestate.h"
#include "game/state/shared/agent.h"
#include "game/ui/skirmish/skirmish.h"
namespace OpenApoc
{

SelectForces::SelectForces(sp<GameState> state, Skirmish &skirmish,
                           std::map<StateRef<AgentType>, int> *aliens, int *guards, int *civilians)
    : Stage(), menuform(ui().getForm("selectforces")), skirmish(skirmish), state(*state)
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	menuform->findControlTyped<Label>("LOCATION")->setText(skirmish.getLocationText());

	menuform->findControlTyped<ScrollBar>("NUM_BSK_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_BSK")->setText(
		        format("%d", menuform->findControlTyped<ScrollBar>("NUM_BSK_SLIDER")->getValue()));
	    });
	menuform->findControlTyped<ScrollBar>("NUM_CHRYS_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_CHRYS")
		        ->setText(format(
		            "%d", menuform->findControlTyped<ScrollBar>("NUM_CHRYS_SLIDER")->getValue()));
	    });
	menuform->findControlTyped<ScrollBar>("NUM_EGG_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_EGG")->setText(
		        format("%d", menuform->findControlTyped<ScrollBar>("NUM_EGG_SLIDER")->getValue()));
	    });

	menuform->findControlTyped<ScrollBar>("NUM_SPITTER_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_SPITTER")
		        ->setText(format(
		            "%d", menuform->findControlTyped<ScrollBar>("NUM_SPITTER_SLIDER")->getValue()));
	    });
	menuform->findControlTyped<ScrollBar>("NUM_ANTHROPOD_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_ANTHROPOD")
		        ->setText(format(
		            "%d",
		            menuform->findControlTyped<ScrollBar>("NUM_ANTHROPOD_SLIDER")->getValue()));
	    });
	menuform->findControlTyped<ScrollBar>("NUM_MULTIWORM_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_MULTIWORM")
		        ->setText(format(
		            "%d",
		            menuform->findControlTyped<ScrollBar>("NUM_MULTIWORM_SLIDER")->getValue()));
	    });

	menuform->findControlTyped<ScrollBar>("NUM_POPPER_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_POPPER")
		        ->setText(format(
		            "%d", menuform->findControlTyped<ScrollBar>("NUM_POPPER_SLIDER")->getValue()));
	    });
	menuform->findControlTyped<ScrollBar>("NUM_SKEL_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_SKEL")
		        ->setText(format(
		            "%d", menuform->findControlTyped<ScrollBar>("NUM_SKEL_SLIDER")->getValue()));
	    });
	menuform->findControlTyped<ScrollBar>("NUM_HYPERWORM_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_HYPERWORM")
		        ->setText(format(
		            "%d",
		            menuform->findControlTyped<ScrollBar>("NUM_HYPERWORM_SLIDER")->getValue()));
	    });

	menuform->findControlTyped<ScrollBar>("NUM_MEGA_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_MEGA")
		        ->setText(format(
		            "%d", menuform->findControlTyped<ScrollBar>("NUM_MEGA_SLIDER")->getValue()));
	    });
	menuform->findControlTyped<ScrollBar>("NUM_PSI_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_PSI")->setText(
		        format("%d", menuform->findControlTyped<ScrollBar>("NUM_PSI_SLIDER")->getValue()));
	    });
	menuform->findControlTyped<ScrollBar>("NUM_MICRO_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_MICRO")
		        ->setText(format(
		            "%d", menuform->findControlTyped<ScrollBar>("NUM_MICRO_SLIDER")->getValue()));
	    });

	menuform->findControlTyped<ScrollBar>("NUM_GUARD_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_GUARD")
		        ->setText(format(
		            "%d", menuform->findControlTyped<ScrollBar>("NUM_GUARD_SLIDER")->getValue()));
	    });

	menuform->findControlTyped<ScrollBar>("NUM_CIVILIAN_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_CIVILIAN")
		        ->setText(format(
		            "%d",
		            menuform->findControlTyped<ScrollBar>("NUM_CIVILIAN_SLIDER")->getValue()));
	    });

	menuform->findControlTyped<CheckBox>("DEFAULT_ALIENS")
	    ->addCallback(FormEventType::CheckBoxChange, [this](Event *) {
		    menuform->findControlTyped<Graphic>("SHROUD_ALIENS")
		        ->setVisible(menuform->findControlTyped<CheckBox>("DEFAULT_ALIENS")->isChecked());
	    });
	menuform->findControlTyped<CheckBox>("DEFAULT_GUARDS")
	    ->addCallback(FormEventType::CheckBoxChange, [this](Event *) {
		    menuform->findControlTyped<Graphic>("SHROUD_GUARDS")
		        ->setVisible(menuform->findControlTyped<CheckBox>("DEFAULT_GUARDS")->isChecked());
	    });
	menuform->findControlTyped<CheckBox>("DEFAULT_CIVILIANS")
	    ->addCallback(FormEventType::CheckBoxChange, [this](Event *) {
		    menuform->findControlTyped<Graphic>("SHROUD_CIVILIANS")
		        ->setVisible(
		            menuform->findControlTyped<CheckBox>("DEFAULT_CIVILIANS")->isChecked());
	    });

	auto shroud = mksp<RGBImage>(Vec2<int>{640, 480});
	auto shroudColor = Colour(0, 0, 0, 196);
	{
		RGBImageLock l(shroud);

		for (int x = 0; x < 640; x++)
		{
			for (int y = 0; y < 480; y++)
			{
				l.set({x, y}, shroudColor);
			}
		}
	}
	menuform->findControlTyped<Graphic>("SHROUD_ALIENS")->setImage(shroud);
	menuform->findControlTyped<Graphic>("SHROUD_GUARDS")->setImage(shroud);
	menuform->findControlTyped<Graphic>("SHROUD_CIVILIANS")->setImage(shroud);

	if (aliens)
	{
		for (auto &a : *aliens)
		{
			if (a.first.id == "AGENTTYPE_ANTHROPOD")
			{
				menuform->findControlTyped<ScrollBar>("NUM_ANTHROPOD_SLIDER")->setValue(a.second);
			}
			else if (a.first.id == "AGENTTYPE_BRAINSUCKER")
			{
				menuform->findControlTyped<ScrollBar>("NUM_BSK_SLIDER")->setValue(a.second);
			}
			else if (a.first.id == "AGENTTYPE_CHRYSALIS")
			{
				menuform->findControlTyped<ScrollBar>("NUM_CHRYS_SLIDER")->setValue(a.second);
			}
			else if (a.first.id == "AGENTTYPE_HYPERWORM")
			{
				menuform->findControlTyped<ScrollBar>("NUM_HYPERWORM_SLIDER")->setValue(a.second);
			}
			else if (a.first.id == "AGENTTYPE_MEGASPAWN")
			{
				menuform->findControlTyped<ScrollBar>("NUM_MEGA_SLIDER")->setValue(a.second);
			}
			else if (a.first.id == "AGENTTYPE_MICRONOID_AGGREGATE")
			{
				menuform->findControlTyped<ScrollBar>("NUM_MICRO_SLIDER")->setValue(a.second);
			}
			else if (a.first.id == "AGENTTYPE_MULTIWORM")
			{
				menuform->findControlTyped<ScrollBar>("NUM_MULTIWORM_SLIDER")->setValue(a.second);
			}
			else if (a.first.id == "AGENTTYPE_MULTIWORM_EGG")
			{
				menuform->findControlTyped<ScrollBar>("NUM_EGG_SLIDER")->setValue(a.second);
			}
			else if (a.first.id == "AGENTTYPE_POPPER")
			{
				menuform->findControlTyped<ScrollBar>("NUM_POPPER_SLIDER")->setValue(a.second);
			}
			else if (a.first.id == "AGENTTYPE_PSIMORPH")
			{
				menuform->findControlTyped<ScrollBar>("NUM_PSI_SLIDER")->setValue(a.second);
			}
			else if (a.first.id == "AGENTTYPE_QUEENSPAWN")
			{
				menuform->findControlTyped<CheckBox>("QUEENSPAWN")->setChecked(a.second > 0);
			}
			else if (a.first.id == "AGENTTYPE_SKELETOID")
			{
				menuform->findControlTyped<ScrollBar>("NUM_SKEL_SLIDER")->setValue(a.second);
			}
			else if (a.first.id == "AGENTTYPE_SPITTER")
			{
				menuform->findControlTyped<ScrollBar>("NUM_SPITTER_SLIDER")->setValue(a.second);
			}
		}
	}
	else
	{
		menuform->findControlTyped<CheckBox>("DEFAULT_ALIENS")->setChecked(true);
	}
	if (guards)
	{
		if (*guards == -1)
		{
			menuform->findControl("NUM_GUARD_SLIDER")->setVisible(false);
			menuform->findControl("NUM_GUARD")->setVisible(false);
			menuform->findControl("DEFAULT_GUARDS")->setVisible(false);
			menuform->findControl("GUARDS_LABEL1")->setVisible(false);
			menuform->findControl("GUARDS_LABEL2")->setVisible(false);
			menuform->findControl("GUARDS_BUTTON1")->setVisible(false);
			menuform->findControl("GUARDS_BUTTON2")->setVisible(false);
		}
		else
		{
			menuform->findControlTyped<ScrollBar>("NUM_GUARD_SLIDER")->setValue(*guards);
		}
	}
	else
	{
		menuform->findControlTyped<CheckBox>("DEFAULT_GUARDS")->setChecked(true);
	}
	if (civilians)
	{
		if (*civilians == -1)
		{
			menuform->findControl("NUM_CIVILIAN_SLIDER")->setVisible(false);
			menuform->findControl("NUM_CIVILIAN")->setVisible(false);
			menuform->findControl("DEFAULT_CIVILIANS")->setVisible(false);
			menuform->findControl("CIVILIANS_LABEL1")->setVisible(false);
			menuform->findControl("CIVILIANS_LABEL2")->setVisible(false);
			menuform->findControl("CIVILIANS_BUTTON1")->setVisible(false);
			menuform->findControl("CIVILIANS_BUTTON2")->setVisible(false);
		}
		else
		{
			menuform->findControlTyped<ScrollBar>("NUM_CIVILIAN_SLIDER")->setValue(*civilians);
		}
	}
	else
	{
		menuform->findControlTyped<CheckBox>("NUM_CIVILIAN_SLIDER")->setChecked(true);
	}
}

SelectForces::~SelectForces() = default;

void SelectForces::begin() {}

void SelectForces::pause() {}

void SelectForces::resume() { fw().stageQueueCommand({StageCmd::Command::POP}); }

void SelectForces::finish() {}

void SelectForces::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

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
			std::map<StateRef<AgentType>, int> aliens;

			if (!menuform->findControlTyped<CheckBox>("DEFAULT_ALIENS")->isChecked())
			{
				if (menuform->findControlTyped<ScrollBar>("NUM_ANTHROPOD_SLIDER")->getValue() > 0)
				{
					aliens.emplace(
					    StateRef<AgentType>(&state, "AGENTTYPE_ANTHROPOD"),
					    menuform->findControlTyped<ScrollBar>("NUM_ANTHROPOD_SLIDER")->getValue());
				}
				if (menuform->findControlTyped<ScrollBar>("NUM_BSK_SLIDER")->getValue() > 0)
				{
					aliens.emplace(
					    StateRef<AgentType>(&state, "AGENTTYPE_BRAINSUCKER"),
					    menuform->findControlTyped<ScrollBar>("NUM_BSK_SLIDER")->getValue());
				}
				if (menuform->findControlTyped<ScrollBar>("NUM_CHRYS_SLIDER")->getValue() > 0)
				{
					aliens.emplace(
					    StateRef<AgentType>(&state, "AGENTTYPE_CHRYSALIS"),
					    menuform->findControlTyped<ScrollBar>("NUM_CHRYS_SLIDER")->getValue());
				}
				if (menuform->findControlTyped<ScrollBar>("NUM_HYPERWORM_SLIDER")->getValue() > 0)
				{
					aliens.emplace(
					    StateRef<AgentType>(&state, "AGENTTYPE_HYPERWORM"),
					    menuform->findControlTyped<ScrollBar>("NUM_HYPERWORM_SLIDER")->getValue());
				}
				if (menuform->findControlTyped<ScrollBar>("NUM_MEGA_SLIDER")->getValue() > 0)
				{
					aliens.emplace(
					    StateRef<AgentType>(&state, "AGENTTYPE_MEGASPAWN"),
					    menuform->findControlTyped<ScrollBar>("NUM_MEGA_SLIDER")->getValue());
				}
				if (menuform->findControlTyped<ScrollBar>("NUM_MICRO_SLIDER")->getValue() > 0)
				{
					aliens.emplace(
					    StateRef<AgentType>(&state, "AGENTTYPE_MICRONOID_AGGREGATE"),
					    menuform->findControlTyped<ScrollBar>("NUM_MICRO_SLIDER")->getValue());
				}
				if (menuform->findControlTyped<ScrollBar>("NUM_MULTIWORM_SLIDER")->getValue() > 0)
				{
					aliens.emplace(
					    StateRef<AgentType>(&state, "AGENTTYPE_MULTIWORM"),
					    menuform->findControlTyped<ScrollBar>("NUM_MULTIWORM_SLIDER")->getValue());
				}
				if (menuform->findControlTyped<ScrollBar>("NUM_EGG_SLIDER")->getValue() > 0)
				{
					aliens.emplace(
					    StateRef<AgentType>(&state, "AGENTTYPE_MULTIWORM_EGG"),
					    menuform->findControlTyped<ScrollBar>("NUM_EGG_SLIDER")->getValue());
				}
				if (menuform->findControlTyped<ScrollBar>("NUM_POPPER_SLIDER")->getValue() > 0)
				{
					aliens.emplace(
					    StateRef<AgentType>(&state, "AGENTTYPE_POPPER"),
					    menuform->findControlTyped<ScrollBar>("NUM_POPPER_SLIDER")->getValue());
				}
				if (menuform->findControlTyped<ScrollBar>("NUM_PSI_SLIDER")->getValue() > 0)
				{
					aliens.emplace(
					    StateRef<AgentType>(&state, "AGENTTYPE_PSIMORPH"),
					    menuform->findControlTyped<ScrollBar>("NUM_PSI_SLIDER")->getValue());
				}
				if (menuform->findControlTyped<ScrollBar>("NUM_SKEL_SLIDER")->getValue() > 0)
				{
					aliens.emplace(
					    StateRef<AgentType>(&state, "AGENTTYPE_SKELETOID"),
					    menuform->findControlTyped<ScrollBar>("NUM_SKEL_SLIDER")->getValue());
				}
				if (menuform->findControlTyped<CheckBox>("QUEENSPAWN")->isChecked())
				{
					aliens.emplace(StateRef<AgentType>(&state, "AGENTTYPE_QUEENSPAWN"), 1);
				}
				if (menuform->findControlTyped<ScrollBar>("NUM_SPITTER_SLIDER")->getValue() > 0)
				{
					aliens.emplace(
					    StateRef<AgentType>(&state, "AGENTTYPE_SPITTER"),
					    menuform->findControlTyped<ScrollBar>("NUM_SPITTER_SLIDER")->getValue());
				}
			}

			int guards = menuform->findControlTyped<ScrollBar>("NUM_GUARD_SLIDER")->getValue();
			int civilians =
			    menuform->findControlTyped<ScrollBar>("NUM_CIVILIAN_SLIDER")->getValue();

			skirmish.goToBattle(
			    !aliens.empty(), aliens,
			    !menuform->findControlTyped<CheckBox>("DEFAULT_GUARDS")->isChecked(), guards,
			    !menuform->findControlTyped<CheckBox>("DEFAULT_CIVILIANS")->isChecked(), civilians);

			return;
		}
	}
}

void SelectForces::update() { menuform->update(); }

void SelectForces::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool SelectForces::isTransition() { return false; }
} // namespace OpenApoc
