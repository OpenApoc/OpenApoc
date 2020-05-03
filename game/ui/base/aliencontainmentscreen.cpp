#include "game/ui/base/aliencontainmentscreen.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/radiobutton.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/renderer.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/gamestate.h"
#include "game/ui/base/basestage.h"
#include "game/ui/general/messagebox.h"
#include "game/ui/general/transactioncontrol.h"
#include <array>

namespace OpenApoc
{

AlienContainmentScreen::AlienContainmentScreen(sp<GameState> state, bool forceLimits)
    : TransactionScreen(state, forceLimits)
{
	form->findControlTyped<Label>("TITLE")->setText(tr("ALIEN CONTAINMENT"));
	form->findControlTyped<Graphic>("BG")->setImage(
	    fw().data->loadImage("xcom3/ufodata/aliencon.pcx"));
	form->findControlTyped<Graphic>("DOLLAR_ICON")->setVisible(false);
	form->findControlTyped<Graphic>("DELTA_UNDERPANTS")->setVisible(false);
	form->findControlTyped<Label>("TEXT_FUNDS_DELTA")->setVisible(false);

	form->findControlTyped<RadioButton>("BUTTON_SOLDIERS")->setVisible(false);
	form->findControlTyped<RadioButton>("BUTTON_BIOSCIS")->setVisible(false);
	form->findControlTyped<RadioButton>("BUTTON_PHYSCIS")->setVisible(false);
	form->findControlTyped<RadioButton>("BUTTON_ENGINRS")->setVisible(false);
	form->findControlTyped<RadioButton>("BUTTON_ALIENS")->setVisible(false);

	form->findControlTyped<RadioButton>("BUTTON_VEHICLES")->setVisible(false);
	form->findControlTyped<RadioButton>("BUTTON_AGENTS")->setVisible(false);
	form->findControlTyped<RadioButton>("BUTTON_FLYING")->setVisible(false);
	form->findControlTyped<RadioButton>("BUTTON_GROUND")->setVisible(false);

	confirmClosureText = tr("Confirm Alien Containment Orders");

	type = Type::Aliens;
	form->findControlTyped<RadioButton>("BUTTON_ALIENS")->setChecked(true);
}

void AlienContainmentScreen::closeScreen()
{
	// Step 01: Check accommodation of different sorts
	{
		std::array<int, MAX_BASES> vecBioDelta;
		std::array<bool, MAX_BASES> vecChanged;
		vecBioDelta.fill(0);
		vecChanged.fill(false);

		// Find all delta and mark all that have any changes
		std::set<sp<TransactionControl>> linkedControls;
		for (auto &c : transactionControls[Type::Aliens])
		{
			if (linkedControls.find(c) != linkedControls.end())
			{
				continue;
			}
			int i = 0;
			for ([[maybe_unused]] const auto &b : state->player_bases)
			{
				int bioDelta = c->getBioDelta(i);
				if (bioDelta)
				{
					vecBioDelta[i] += bioDelta;
					vecChanged[i] = true;
				}
				i++;
			}
			if (c->getLinked())
			{
				for (auto &l : *c->getLinked())
				{
					linkedControls.insert(l.lock());
				}
			}
		}

		// Check every base, find first bad one
		int i = 0;
		StateRef<Base> bad_base;
		for (auto &b : state->player_bases)
		{
			if ((vecChanged[i] || forceLimits) &&
			    b.second->getUsage(*state, FacilityType::Capacity::Aliens, vecBioDelta[i]) > 100)
			{
				bad_base = b.second->building->base;
				break;
			}
			i++;
		}

		// Found bad base
		if (bad_base)
		{
			UString title(tr("Alien Containment space exceeded"));
			UString message(tr("Alien Containment space exceeded. Destroy more Aliens!"));

			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH,
			     mksp<MessageBox>(title, message, MessageBox::ButtonOptions::Ok)});

			if (bad_base != state->current_base)
			{
				for (auto &view : miniViews)
				{
					if (bad_base == view->getData<Base>())
					{
						currentView = view;
						changeBase(bad_base);
						break;
					}
				}
			}
			return;
		}
	}

	// Step 02: If we reached this then go!
	executeOrders();
	fw().stageQueueCommand({StageCmd::Command::POP});
	return;
}

void AlienContainmentScreen::executeOrders()
{
	int rightIdx = getRightIndex();

	// AlienContainment: Simply apply
	for (auto &c : transactionControls[Type::Aliens])
	{
		int i = 0;
		for (auto &b : state->player_bases)
		{
			if (c->tradeState.shipmentsFrom(i) > 0)
			{
				b.second->inventoryBioEquipment[c->itemId] =
				    c->tradeState.getStock(i, rightIdx, true);
			}
			i++;
		}
	}
}

}; // namespace OpenApoc
