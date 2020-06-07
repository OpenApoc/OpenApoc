#include "game/ui/city/basebuyscreen.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/city/agentmission.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gamestate.h"
#include "game/state/shared/agent.h"
#include "game/state/shared/organisation.h"
#include "game/ui/components/basegraphics.h"
#include "game/ui/general/messagebox.h"
#include "game/ui/tileview/cityview.h"
#include "library/strings_format.h"

namespace OpenApoc
{

BaseBuyScreen::BaseBuyScreen(sp<GameState> state, sp<Building> building)
    : Stage(), form(ui().getForm("city/basebuy")), state(state)
{
	Vec2<int> size = building->bounds.size();
	price = std::min(size.x, 8) * std::min(size.y, 8) * COST_PER_TILE;
	base = mksp<Base>(*state, StateRef<Building>{state.get(), building});
}

BaseBuyScreen::~BaseBuyScreen() = default;

void BaseBuyScreen::begin()
{
	baseView = form->findControlTyped<Graphic>("GRAPHIC_BASE_VIEW");

	form->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());

	auto text = form->findControlTyped<Label>("TEXT_PRICE");
	text->setText(format(tr("This Building will cost $%d"), price));

	form->findControlTyped<Graphic>("GRAPHIC_MINIMAP")
	    ->setImage(BaseGraphics::drawMinimap(state, *base->building));
}

void BaseBuyScreen::pause() {}

void BaseBuyScreen::resume() {}

void BaseBuyScreen::finish() {}

void BaseBuyScreen::eventOccurred(Event *e)
{
	form->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_RETURN ||
		    e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			form->findControl("BUTTON_OK")->click();
		}
	}

	else if (e->type() == EVENT_FORM_INTERACTION &&
	         e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_OK")
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_BUY_BASE")
		{
			if (state->getPlayer()->balance >= price)
			{
				state->getPlayer()->balance -= price;
				StateRef<Building> newBuilding;
				for (auto &b : base->building->city->buildings)
				{
					if (base->building != b.second && b.second->owner == base->building->owner)
					{
						newBuilding = {state.get(), b.first};
						break;
					}
				}
				if (!newBuilding)
				{
					LogError("We just bought %s's last building? WTF?",
					         base->building->owner->name);
				}
				base->building->owner = state->getPlayer();
				// Boot organisation's vehicles and agents
				for (auto &v : state->vehicles)
				{
					if (v.second->homeBuilding == base->building)
					{
						if (newBuilding)
						{
							v.second->homeBuilding = newBuilding;
							if (v.second->currentBuilding == base->building)
							{
								v.second->setMission(
								    *state, VehicleMission::gotoBuilding(*state, *v.second));
							}
						}
						else
						{
							v.second->die(*state, true);
						}
					}
					else if (v.second->currentBuilding == base->building &&
					         v.second->owner != state->getPlayer())
					{
						v.second->setMission(*state,
						                     VehicleMission::gotoBuilding(*state, *v.second));
					}
				}
				for (auto &a : state->agents)
				{
					if (a.second->homeBuilding == base->building)
					{
						if (newBuilding)
						{
							a.second->homeBuilding = newBuilding;
							if (a.second->currentBuilding == base->building)
							{
								a.second->setMission(*state,
								                     AgentMission::gotoBuilding(*state, *a.second));
							}
						}
						else
						{
							a.second->die(*state, true);
						}
					}
					else if (a.second->currentBuilding == base->building &&
					         a.second->owner != state->getPlayer())
					{
						a.second->setMission(*state, AgentMission::gotoBuilding(*state, *a.second));
					}
				}

				state->baseIndex += 1;
				base->name = "Base " + Strings::fromInteger(state->baseIndex);
				state->player_bases[Base::getPrefix() + Strings::fromInteger(state->baseIndex)] =
				    base;
				base->building->base = {state.get(), base};

				fw().stageQueueCommand({StageCmd::Command::REPLACE, mksp<CityView>(state)});
			}
			else
			{
				auto messagebox =
				    mksp<MessageBox>(tr("No Sale"), tr("Not enough money to buy this building."),
				                     MessageBox::ButtonOptions::Ok);
				fw().stageQueueCommand({StageCmd::Command::PUSH, messagebox});
			}
		}
	}
}

void BaseBuyScreen::update() { form->update(); }

void BaseBuyScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	form->render();
	renderBase();
}

bool BaseBuyScreen::isTransition() { return false; }

void BaseBuyScreen::renderBase()
{
	BaseGraphics::renderBase(baseView->getLocationOnScreen(), *base);
}

}; // namespace OpenApoc
