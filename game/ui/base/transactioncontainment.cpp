#include "game/ui/base/transactioncontainment.h"
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

namespace OpenApoc
{

TransactionContainment::TransactionContainment(sp<GameState> state, bool forceLimits)
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

void TransactionContainment::closeScreen(bool forced)
{
	// Forced means we already asked player to confirm some secondary thing
	// (like there being no free ferries right now)
	if (forced)
	{
		executeOrders();
		fw().stageQueueCommand({StageCmd::Command::POP});
		return;
	}

	// Step 01: Check funds

	// Step 02: Check accomodation of different sorts
	{
		// FIXME: CHECK LQ SPACE
		std::vector<int> vecCargoDelta;
		std::vector<int> vecBioDelta;
		std::vector<bool> vecChanged;
		vecCargoDelta.resize(8);
		vecBioDelta.resize(8);
		vecChanged.resize(8);

		// Find all delta and mark all that have any changes
		std::set<sp<TransactionControl>> linkedControls;
		for (auto &l : transactionControls)
		{
			for (auto &c : l.second)
			{
				if (linkedControls.find(c) != linkedControls.end())
				{
					continue;
				}
				for (int i = 0; i < 8; i++)
				{
					vecCargoDelta[i] += c->getCargoDelta(i);
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
		}

		// Check every base, find first bad one
		int bindex = 0;
		StateRef<Base> bad_base;
		bool cargoOverLimit = false;
		bool alienOverLimit = false;
		bool crewOverLimit = false; //  only if mode == Mode::Transfer
		for (auto &b : state->player_bases)
		{
			if (vecChanged[bindex] || forceLimits)
			{
				if (b.second->getUsage(*state, FacilityType::Capacity::Stores,
				                       vecCargoDelta[bindex]) > 100)
				{
					bad_base = b.second->building->base;
					cargoOverLimit = true;
					break;
				}
			}
			bindex++;
		}

		// Found bad base
		if (bad_base)
		{
			UString title;
			UString message;
			if (crewOverLimit)
			{
				title = tr("Accomodation exceeded");
				message = tr("Transfer limited by available accommodation.");
				type = Type::Soldier;
			}
			else if (cargoOverLimit)
			{
				title = tr("Storage space exceeded");
				{
					message = tr("Transfer limited by available storage space.");
				}
				type = Type::AgentEquipment;
			}
			else if (alienOverLimit)
			{
				title = tr("Alien Containment space exceeded");
				//				if (mode == Mode::AlienContainment)
				{
					message = tr("Alien Containment space exceeded. Destroy more Aliens!");
				}
				type = Type::Aliens;
			}
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

	// Step 04: If we reached this then go!
	executeOrders();
	fw().stageQueueCommand({StageCmd::Command::POP});
	return;
}

void TransactionContainment::executeOrders()
{
	std::vector<StateRef<Base>> bases;
	for (auto &b : state->player_bases)
	{
		bases.push_back(b.second->building->base);
	}
	bases.resize(8);

	// AlienContainment: Simply apply
	//	if (mode == Mode::AlienContainment)
	{
		for (auto &c : transactionControls[Type::Aliens])
		{
			for (int i = 0; i < 8; i++)
			{
				if (bases[i] && c->initialStock[i] != c->currentStock[i])
				{
					bases[i]->inventoryBioEquipment[c->itemId] = c->currentStock[i];
				}
			}
		}
		return;
	}

	// Step 02: Gather data about differences in stocks
	// Map is base id to number bought/sold
	std::map<StateRef<AEquipmentType>, std::map<int, int>> aeMap;
	std::map<StateRef<AEquipmentType>, std::map<int, int>> bioMap;
	std::map<StateRef<VEquipmentType>, std::map<int, int>> veMap;
	std::map<StateRef<VAmmoType>, std::map<int, int>> vaMap;
	std::map<StateRef<VehicleType>, std::map<int, int>> vtMap;
	std::list<std::pair<StateRef<Vehicle>, int>> soldVehicles;

	std::set<sp<TransactionControl>> linkedControls;
	for (auto &l : transactionControls)
	{
		for (auto &c : l.second)
		{
			if (linkedControls.find(c) != linkedControls.end())
			{
				continue;
			}
			bool vehicleSold = c->itemType == TransactionControl::Type::Vehicle;
			for (int i = 0; i < 8; i++)
			{
				switch (c->itemType)
				{
					case TransactionControl::Type::Vehicle:
						if (c->currentStock[i] != 0)
						{
							vehicleSold = false;
						}
						break;
					case TransactionControl::Type::AgentEquipmentBio:
						bioMap[{state.get(), c->itemId}][i] =
						    c->currentStock[i] - c->initialStock[i];
						break;
					case TransactionControl::Type::AgentEquipmentCargo:
						aeMap[{state.get(), c->itemId}][i] =
						    c->currentStock[i] - c->initialStock[i];
						break;
					case TransactionControl::Type::VehicleAmmo:
						vaMap[{state.get(), c->itemId}][i] =
						    c->currentStock[i] - c->initialStock[i];
						break;
					case TransactionControl::Type::VehicleEquipment:
						veMap[{state.get(), c->itemId}][i] =
						    c->currentStock[i] - c->initialStock[i];
						break;
					case TransactionControl::Type::VehicleType:
						vtMap[{state.get(), c->itemId}][i] =
						    c->currentStock[i] - c->initialStock[i];
						break;
				}
			}
			if (vehicleSold)
			{
				soldVehicles.emplace_back(StateRef<Vehicle>{state.get(), c->itemId}, c->price);
			}
			for (auto &l : c->getLinked())
			{
				linkedControls.insert(l);
			}
		}
	}

	// Step 03: Act according to mode
	auto player = state->getPlayer();
}
}