#include "game/ui/base/aliencontainmentscreen.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/radiobutton.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/logger.h"
#include "framework/renderer.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/shared/organisation.h"
#include "game/ui/base/basestage.h"
#include "game/ui/general/messagebox.h"
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

	form->findControlTyped<RadioButton>("BUTTON_ALIENS")->setChecked(true);

	confirmClosure = tr("Confirm Alien Containment Orders");
	type = Type::Aliens;
}

void AlienContainmentScreen::closeScreen()
{
	constexpr int MAX_BASES = 8;

	// Step 02: Check accomodation of different sorts
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
			for (int i = 0; i < MAX_BASES; i++)
			{
				vecBioDelta[i] += c->getBioDelta(i);
				if (c->initialStock[i] != c->currentStock[i])
				{
					vecChanged[i] = true;
				}
			}
			for (auto &l : c->getLinked())
			{
				linkedControls.insert(l);
			}
		}

		// Check every base, find first bad one
		int bindex = 0;
		StateRef<Base> bad_base;
		bool alienOverLimit = false;
		for (auto &b : state->player_bases)
		{
			if (vecChanged[bindex] || forceLimits)
			{
				if (b.second->getUsage(*state, FacilityType::Capacity::Aliens, vecBioDelta[bindex]) > 100)
				{
					bad_base = b.second->building->base;
					alienOverLimit = true;
					break;
				}
			}
			bindex++;
		}

		// Found bad base
		if (bad_base)
		{
			UString title(tr("Alien Containment space exceeded"));
			UString message(tr("Alien Containment space exceeded. Destroy more Aliens!"));

			fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<MessageBox>(title, message, MessageBox::ButtonOptions::Ok)});
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

	// Step 04: If we reached this then go!
	executeOrders();
	fw().stageQueueCommand({StageCmd::Command::POP});
	return;
}

void AlienContainmentScreen::executeOrders()
{
	// AlienContainment: Simply apply
	for (auto &c : transactionControls[Type::Aliens])
	{
		int bindex = 0;
		for (auto &b : state->player_bases)
		{
			if (c->initialStock[bindex] != c->currentStock[bindex])
			{
				b.second->inventoryBioEquipment[c->itemId] = c->currentStock[bindex];
			}
			bindex++;
		}
	}
}

}