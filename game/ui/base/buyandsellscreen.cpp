#include "game/ui/base/buyandsellscreen.h"
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
#include "game/state/city/vehiclemission.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/shared/organisation.h"
#include "game/ui/base/basestage.h"
#include "game/ui/general/messagebox.h"

namespace OpenApoc
{

BuyAndSellScreen::BuyAndSellScreen(sp<GameState> state, bool forceLimits)
    : TransactionScreen(state, forceLimits)
{
	form->findControlTyped<Label>("TITLE")->setText(tr("BUY AND SELL"));
	form->findControlTyped<Graphic>("BG")->setImage(
	    fw().data->loadImage("xcom3/ufodata/buy&sell.pcx"));
	form->findControlTyped<Graphic>("DOLLAR_ICON")->setVisible(true);
	form->findControlTyped<Label>("TEXT_FUNDS_DELTA")->setVisible(true);
	form->findControlTyped<Graphic>("DELTA_UNDERPANTS")->setVisible(true);

	form->findControlTyped<RadioButton>("BUTTON_SOLDIERS")->setVisible(false);
	form->findControlTyped<RadioButton>("BUTTON_BIOSCIS")->setVisible(false);
	form->findControlTyped<RadioButton>("BUTTON_PHYSCIS")->setVisible(false);
	form->findControlTyped<RadioButton>("BUTTON_ENGINRS")->setVisible(false);
	form->findControlTyped<RadioButton>("BUTTON_ALIENS")->setVisible(false);

	form->findControlTyped<RadioButton>("BUTTON_VEHICLES")->setVisible(true);
	form->findControlTyped<RadioButton>("BUTTON_VEHICLES")->Location.y = 40;
	form->findControlTyped<RadioButton>("BUTTON_AGENTS")->setVisible(true);
	form->findControlTyped<RadioButton>("BUTTON_AGENTS")->Location.y = 80;
	form->findControlTyped<RadioButton>("BUTTON_FLYING")->setVisible(true);
	form->findControlTyped<RadioButton>("BUTTON_FLYING")->Location.y = 120;
	form->findControlTyped<RadioButton>("BUTTON_GROUND")->setVisible(true);
	form->findControlTyped<RadioButton>("BUTTON_GROUND")->Location.y = 160;

	form->findControlTyped<RadioButton>("BUTTON_VEHICLES")->setChecked(true);

	confirmClosure = tr("Confirm Sales/Purchases");
	type = Type::Vehicle;
}

int BuyAndSellScreen::getLeftIndex()
{
	return config().getBool("OpenApoc.NewFeature.MarketOnRight")
	           ? TransactionScreen::getLeftIndex()
	           : TransactionScreen::getRightIndex();
}

int BuyAndSellScreen::getRightIndex()
{
	return config().getBool("OpenApoc.NewFeature.MarketOnRight")
	           ? TransactionScreen::getRightIndex()
	           : TransactionScreen::getLeftIndex();
}

void BuyAndSellScreen::updateFormValues(bool queueHighlightUpdate)
{
	TransactionScreen::updateFormValues(queueHighlightUpdate);

	// Update money
	int balance = state->getPlayer()->balance + moneyDelta;
	form->findControlTyped<Label>("TEXT_FUNDS")->setText(Strings::fromInteger(balance));
	form->findControlTyped<Label>("TEXT_FUNDS_DELTA")
	    ->setText(format("%s%s", moneyDelta > 0 ? "+" : "", Strings::fromInteger(moneyDelta)));
}

void BuyAndSellScreen::closeScreen()
{
	// Step 01: Check funds
	//	if (mode == Mode::BuySell)
	{
		int moneyDelta = 0;

		std::set<sp<TransactionControl>> linkedControls;
		for (auto &l : transactionControls)
		{
			for (auto &c : l.second)
			{
				if (linkedControls.find(c) != linkedControls.end())
				{
					continue;
				}
				moneyDelta += c->getPriceDelta();
				for (auto &l : c->getLinked())
				{
					linkedControls.insert(l);
				}
			}
		}
		int balance = state->getPlayer()->balance + moneyDelta;
		if (balance < 0)
		{
			fw().stageQueueCommand({StageCmd::Command::PUSH,
			                        mksp<MessageBox>(tr("Funds exceeded"),
			                                         tr("Order limited by your available funds."),
			                                         MessageBox::ButtonOptions::Ok)});
			return;
		}
	}

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
				//				if (mode == Mode::BuySell)
				{
					if (forceLimits)
					{
						message = tr("Storage space exceeded. Sell off more items!");
					}
					else
					{
						message = tr("Order limited by the available storage space at this base.");
					}
				}
				// else
				//{
				//	message = tr("Transfer limited by available storage space.");
				//}
				type = Type::AgentEquipment;
			}
			// else if (alienOverLimit)
			//{
			//	title = tr("Alien Containment space exceeded");
			//	if (mode == Mode::AlienContainment)
			//	{
			//		message = tr("Alien Containment space exceeded. Destroy more Aliens!");
			//	}
			//	else
			//	{
			//		message = tr("Transfer limited by available Alien Containment space.");
			//	}
			//	type = Type::Aliens;
			//}
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH,
			     mksp<MessageBox>(title, message, MessageBox::ButtonOptions::Ok)});
			//			if (bad_base != state->current_base && bad_base != second_base)
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

	// Step 03: Check transportation

	// Step 03.01: Check transportation for purchases
	bool purchaseTransferFound = false;
	//	if (mode == Mode::BuySell)
	{
		bool noFerry = false;

		// Find all orgs that we buy from
		std::set<StateRef<Organisation>> orgsBuyFrom;
		std::set<sp<TransactionControl>> linkedControls;
		for (auto &l : transactionControls)
		{
			for (auto &c : l.second)
			{
				if (linkedControls.find(c) != linkedControls.end())
				{
					continue;
				}
				// See if we transfer-bought from an org that's hostile
				if (!purchaseTransferFound)
				{
					for (int i = 0; i < 7; i++)
					{
						if (c->initialStock[8] < c->currentStock[8])
						{
							StateRef<Organisation> owner;
							switch (c->itemType)
							{
								case TransactionControl::Type::AgentEquipmentBio:
								case TransactionControl::Type::AgentEquipmentCargo:
									owner = StateRef<AEquipmentType> { state.get(), c->itemId }
									->manufacturer;
									break;
								case TransactionControl::Type::VehicleEquipment:
									owner = StateRef<VEquipmentType> { state.get(), c->itemId }
									->manufacturer;
									break;
								case TransactionControl::Type::VehicleAmmo:
									owner = StateRef<VAmmoType> { state.get(), c->itemId }
									->manufacturer;
									break;
								case TransactionControl::Type::VehicleType:
								case TransactionControl::Type::Vehicle:
									// Vehicles need no transportation
									break;
							}
							if (owner &&
							    owner->isRelatedTo(state->getPlayer()) ==
							        Organisation::Relation::Hostile)
							{
								purchaseTransferFound = true;
							}
						}
					}
				}
				if (c->initialStock[8] > c->currentStock[8])
				{
					switch (c->itemType)
					{
						case TransactionControl::Type::AgentEquipmentBio:
						case TransactionControl::Type::AgentEquipmentCargo:
							orgsBuyFrom.insert(
							    StateRef<AEquipmentType> { state.get(), c->itemId }->manufacturer);
							break;
						case TransactionControl::Type::VehicleEquipment:
							orgsBuyFrom.insert(
							    StateRef<VEquipmentType> { state.get(), c->itemId }->manufacturer);
							break;
						case TransactionControl::Type::VehicleAmmo:
							orgsBuyFrom.insert(
							    StateRef<VAmmoType> { state.get(), c->itemId }->manufacturer);
							break;
						case TransactionControl::Type::VehicleType:
						case TransactionControl::Type::Vehicle:
							// Vehicles need no transportation
							break;
					}
				}
				for (auto &l : c->getLinked())
				{
					linkedControls.insert(l);
				}
			}
		}
		// Check orgs
		std::list<StateRef<Organisation>> badOrgs;
		bool transportationHostile = false;
		bool transportationBusy = false;
		Organisation::PurchaseResult canBuy;
		for (auto &o : orgsBuyFrom)
		{
			if (o == state->getPlayer())
			{
				continue;
			}
			// Expecting all bases to be in one city
			auto canBuy = o->canPurchaseFrom(*state, state->current_base->building, false);
			switch (canBuy)
			{
				case Organisation::PurchaseResult::NoTransportAvailable:
					transportationBusy = true;
					badOrgs.push_back(o);
					break;
				case Organisation::PurchaseResult::TranportHostile:
					transportationHostile = true;
					badOrgs.push_back(o);
					break;
				case Organisation::PurchaseResult::OrgHasNoBuildings:
				case Organisation::PurchaseResult::OrgHostile:
					LogError("How did we end up buying from an org we can't buy from!?");
					break;
			}
		}
		// There are bad orgs
		if (!badOrgs.empty())
		{
			UString title =
			    format("%s%s", badOrgs.front()->name, badOrgs.size() > 1 ? " & others" : "");

			// If player can ferry themselves then give option
			if (config().getBool("OpenApoc.NewFeature.AllowManualCargoFerry"))
			{
				UString message =
				    transportationHostile
				        ? format("%s %s", tr("Hostile organization refuses to carry out the "
				                             "requested transportation for this company."),
				                 tr("Proceed?"))
				        : format("%s %s", tr("No free transport to carry out the requested "
				                             "transportation detected in the city."),
				                 tr("Proceed?"));
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(title, message, MessageBox::ButtonOptions::YesNo,
				                      [this] { this->forcedCloseScreen(); })});
				return;
			}
			// Otherwise if transportation is only busy give option
			else if (!transportationHostile)
			{
				// FIXME: Different message maybe? Same for now
				UString message = format("%s %s", tr("No free transport to carry out the requested "
				                                     "transportation detected in the city."),
				                         tr("Proceed?"));
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(title, message, MessageBox::ButtonOptions::YesNo,
				                      [this] { this->forcedCloseScreen(); })});
				return;
			}
			// Otherwise deny
			else
			{
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(title, tr("Hostile organization refuses to carry out the "
				                                "requested transportation for this company."),
				                      MessageBox::ButtonOptions::Ok)});
				return;
			}
		}
	}

	// Step 03.02: Check transportation for transfers
	// or for purchase-transfers
	//	if (mode == Mode::Transfer || purchaseTransferFound)
	if (purchaseTransferFound)
	{
		bool transportationHostile = false;
		bool transportationBusy = false;
		std::list<StateRef<Organisation>> badOrgs;

		// Find out who provides transportation services
		std::list<StateRef<Organisation>> ferryCompanies;
		for (auto &o : state->organisations)
		{
			if (o.second->providesTransportationServices)
			{
				ferryCompanies.emplace_back(state.get(), o.first);
			}
		}
		// Check if a ferry provider exists that likes us
		// Clear those that don't
		for (auto it = ferryCompanies.begin(); it != ferryCompanies.end();)
		{
			if ((*it)->isRelatedTo(state->getPlayer()) == Organisation::Relation::Hostile)
			{
				badOrgs.push_back(*it);
				it = ferryCompanies.erase(it);
			}
			else
			{
				it++;
			}
		}
		if (ferryCompanies.empty())
		{
			transportationHostile = true;
		}
		else
		{
			// Check if ferry provider has free ferries
			if (config().getBool("OpenApoc.NewFeature.CallExistingFerry"))
			{
				bool ferryFound = false;
				for (auto &o : ferryCompanies)
				{
					for (auto &v : state->vehicles)
					{
						if (v.second->owner != o || (!v.second->type->provideFreightCargo &&
						                             !v.second->type->provideFreightBio) ||
						    !v.second->missions.empty())
						{
							continue;
						}
						ferryFound = true;
						break;
					}
				}
				if (!ferryFound)
				{
					transportationBusy = true;
					for (auto &o : ferryCompanies)
					{
						badOrgs.push_back(o);
					}
				}
			}
		}

		if (transportationBusy || transportationHostile)
		{
			UString title =
			    format("%s%s", badOrgs.front()->name, badOrgs.size() > 1 ? " & others" : "");

			// If player can ferry themselves then give option
			if (config().getBool("OpenApoc.NewFeature.AllowManualCargoFerry"))
			{
				UString message =
				    transportationHostile
				        ? format("%s %s", tr("This hostile organization refuses to carry out the "
				                             "requested transfer."),
				                 tr("Proceed?"))
				        : format("%s %s", tr("No free transport to carry out the requested "
				                             "transportation detected in the city."),
				                 tr("Proceed?"));
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(title, message, MessageBox::ButtonOptions::YesNo,
				                      [this] { this->forcedCloseScreen(); })});
				return;
			}
			// Otherwise if transportation is only busy give option
			else if (!transportationHostile)
			{
				// FIXME: Different message maybe? Same for now
				UString message = format("%s %s", tr("No free transport to carry out the requested "
				                                     "transportation detected in the city."),
				                         tr("Proceed?"));
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(title, message, MessageBox::ButtonOptions::YesNo,
				                      [this] { this->forcedCloseScreen(); })});
				return;
			}
			// Otherwise deny
			else
			{
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(title, tr("This hostile organization refuses to carry out "
				                                "the requested transfer."),
				                      MessageBox::ButtonOptions::Ok)});
				return;
			}
		}
	}

	// Step 04: If we reached this then go!
	executeOrders();
	fw().stageQueueCommand({StageCmd::Command::POP});
	return;
}

void BuyAndSellScreen::executeOrders()
{
	std::vector<StateRef<Base>> bases;
	for (auto &b : state->player_bases)
	{
		bases.push_back(b.second->building->base);
	}
	bases.resize(8);

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

	// Step 03.01: If buy&sell then:
	// - remove everything negative, order everything positive, adjust balance
	bool needTransfer = false;
	//	if (mode == Mode::BuySell)
	{
		// Step 03.01.01: Buy stuff
		for (auto &e : aeMap)
		{
			for (auto &ae : e.second)
			{
				if (ae.first == 8)
				{
					continue;
				}
				if (ae.second > 0)
				{
					int count =
					    ae.second *
					    (e.first->type == AEquipmentType::Type::Ammo ? e.first->max_ammo : 1);

					auto org = e.first->manufacturer;
					if (org->isRelatedTo(player) != Organisation::Relation::Hostile)
					{
						org->purchase(*state, bases[ae.first]->building, e.first, ae.second);
						ae.second = 0;
					}
					else
					{
						e.second[8] += ae.second;
						needTransfer = true;
					}
				}
			}
		}
		for (auto &e : bioMap)
		{
			for (auto &ae : e.second)
			{
				if (ae.second > 0)
				{
					LogError("Alien %s: How are we buying it!?", e.first.id);
				}
			}
		}
		for (auto &e : veMap)
		{
			for (auto &ve : e.second)
			{
				if (ve.first == 8)
				{
					continue;
				}
				if (ve.second > 0)
				{
					auto org = e.first->manufacturer;
					if (org->isRelatedTo(player) != Organisation::Relation::Hostile)
					{
						org->purchase(*state, bases[ve.first]->building, e.first, ve.second);
						ve.second = 0;
					}
					else
					{
						e.second[8] += ve.second;
						needTransfer = true;
					}
				}
			}
		}
		for (auto &e : vaMap)
		{
			for (auto &va : e.second)
			{
				if (va.first == 8)
				{
					continue;
				}
				if (va.second > 0)
				{
					auto org = e.first->manufacturer;
					if (org->isRelatedTo(player) != Organisation::Relation::Hostile)
					{
						org->purchase(*state, bases[va.first]->building, e.first, va.second);
						va.second = 0;
					}
					else
					{
						e.second[8] += va.second;
						needTransfer = true;
					}
				}
			}
		}
		for (auto &e : vtMap)
		{
			for (auto &vt : e.second)
			{
				if (vt.second > 0)
				{
					auto org = e.first->manufacturer;
					if (org->isRelatedTo(player) != Organisation::Relation::Hostile)
					{
						org->purchase(*state, bases[vt.first]->building, e.first, vt.second);
						vt.second = 0;
					}
					else
					{
						LogError("VehicleType %s: How the hell is being bought from a hostile org?",
						         e.first.id);
					}
				}
			}
		}
		// Step 03.01.02:  Sell stuff
		for (auto &e : aeMap)
		{
			for (auto &ae : e.second)
			{
				int aeSecond = ae.second;
				if (ae.second < 0 && e.second[8] > 0)
				{
					int reserve = std::min(e.second[8], -ae.second);
					e.second[8] -= reserve;
					aeSecond += reserve;
				}
				if (aeSecond < 0)
				{
					int count =
					    aeSecond *
					    (e.first->type == AEquipmentType::Type::Ammo ? e.first->max_ammo : 1);

					if (-count > bases[ae.first]->inventoryAgentEquipment[e.first.id])
					{
						bases[ae.first]->inventoryAgentEquipment[e.first.id] = 0;
					}
					else
					{
						bases[ae.first]->inventoryAgentEquipment[e.first.id] += count;
					}
					int price = 0;
					if (state->economy.find(e.first.id) == state->economy.end())
					{
						LogError("Economy not found for %s: How are we selling it then!?",
						         e.first.id);
					}
					else
					{
						auto &economy = state->economy[e.first.id];
						price = economy.currentPrice;
						economy.currentStock -= aeSecond;
					}
					player->balance -= aeSecond * price;
				}
			}
		}
		for (auto &e : bioMap)
		{
			for (auto &ae : e.second)
			{
				if (ae.second < 0 && e.second[8] > 0)
				{
					LogError("Alien %s: How the hell is it in reserve?", e.first.id);
				}
				if (ae.second < 0)
				{
					int price = 0;
					if (state->economy.find(e.first.id) == state->economy.end())
					{
						// That's how it should be for alien containment
						// LogError("Economy not found for %s: How are we buying it then!?",
						// e.first.id);
					}
					else
					{
						LogError("Economy found for alien containment item %s? WTF?", e.first.id);
						auto &economy = state->economy[e.first.id];
						price = economy.currentPrice;
						economy.currentStock -= ae.second;
					}
					bases[ae.first]->inventoryAgentEquipment[e.first.id] += ae.second;
					player->balance -= ae.second * price;
				}
			}
		}
		for (auto &e : veMap)
		{
			for (auto &ve : e.second)
			{
				int veSecond = ve.second;
				if (ve.second < 0 && e.second[8] > 0)
				{
					int reserve = std::min(e.second[8], -ve.second);
					e.second[8] -= reserve;
					veSecond += reserve;
				}
				if (veSecond < 0)
				{
					int price = 0;
					if (state->economy.find(e.first.id) == state->economy.end())
					{
						LogError("Economy not found for %s: How are we selling it then!?",
						         e.first.id);
					}
					else
					{
						auto &economy = state->economy[e.first.id];
						price = economy.currentPrice;
						economy.currentStock -= veSecond;
					}
					bases[ve.first]->inventoryVehicleEquipment[e.first.id] += veSecond;
					player->balance -= veSecond * price;
				}
			}
		}
		for (auto &e : vaMap)
		{
			for (auto &va : e.second)
			{
				int vaSecond = va.second;
				if (va.second < 0 && e.second[8] > 0)
				{
					int reserve = std::min(e.second[8], -va.second);
					e.second[8] -= reserve;
					vaSecond += reserve;
				}
				if (vaSecond < 0)
				{
					int price = 0;
					if (state->economy.find(e.first.id) == state->economy.end())
					{
						LogError("Economy not found for %s: How are we buying it then!?",
						         e.first.id);
					}
					else
					{
						auto &economy = state->economy[e.first.id];
						price = economy.currentPrice;
						economy.currentStock -= vaSecond;
					}
					bases[va.first]->inventoryVehicleAmmo[e.first.id] += vaSecond;
					player->balance -= vaSecond * price;
				}
			}
		}
		for (auto &e : vtMap)
		{
			for (auto &vt : e.second)
			{
				if (vt.second < 0 && e.second[8] > 0)
				{
					LogError("VehicleType %s: How the hell is it in reserve?", e.first.id);
				}
				if (vt.second < 0)
				{
					LogError("How did we manage to sell a vehicle type!?");
				}
			}
		}

		// Step 03.01.03: Sell vehicles
		for (auto &v : soldVehicles)
		{
			// Expecting sold vehicle to be parked
			// Offload agents
			while (!v.first->currentAgents.empty())
			{
				auto agent = *v.first->currentAgents.begin();
				agent->enterBuilding(*state, v.first->currentBuilding);
			}
			// Offload cargo
			for (auto &c : v.first->cargo)
			{
				v.first->currentBuilding->cargo.push_back(c);
			}
			v.first->die(*state, true);
			player->balance += v.second;
		}
	}
	state->cleanUpDeathNote();

	// Step 03.02: If transfer then move stuff from negative to positive
	//	if (mode == Mode::Transfer || needTransfer)
	if (needTransfer)
	{
		// Agent items
		for (auto &e : aeMap)
		{
			std::list<std::pair<int, int>> source;
			std::list<std::pair<int, int>> destination;
			for (auto &ae : e.second)
			{
				if (ae.second < 0)
				{
					source.emplace_back(ae.first, -ae.second);
				}
				if (ae.second > 0)
				{
					destination.emplace_back(ae.first, ae.second);
				}
			}
			for (auto &d : destination)
			{
				int remaining = d.second;
				for (auto &s : source)
				{
					if (s.second == 0)
					{
						continue;
					}
					int count = std::min(remaining, s.second);
					s.second -= count;
					remaining -= count;
					int realCount =
					    (e.first->type == AEquipmentType::Type::Ammo ? e.first->max_ammo : 1) *
					    count;
					// We may be transferring last bullets in a clip here so adjust accordingly
					realCount = std::min(realCount,
					                     (int)bases[s.first]->inventoryAgentEquipment[e.first.id]);
					bases[s.first]->building->cargo.emplace_back(*state, e.first, count, 0, player,
					                                             bases[d.first]->building);
					bases[s.first]->inventoryAgentEquipment[e.first.id] -= realCount;
					if (remaining == 0)
					{
						break;
					}
				}
				if (remaining != 0)
				{
					LogError("General screw up in transactions, have remaining demand of %d for %s",
					         remaining, e.first.id);
				}
			}
		}
		// Bio items (Aliens)
		for (auto &e : bioMap)
		{
			std::list<std::pair<int, int>> source;
			std::list<std::pair<int, int>> destination;
			for (auto &be : e.second)
			{
				if (be.second < 0)
				{
					source.emplace_back(be.first, be.second);
				}
				if (be.second > 0)
				{
					destination.emplace_back(be.first, be.second);
				}
			}
			for (auto &d : destination)
			{
				int remaining = d.second;
				for (auto &s : source)
				{
					if (s.second == 0)
					{
						continue;
					}
					int count = std::min(remaining, s.second);
					s.second -= count;
					remaining -= count;
					bases[s.first]->building->cargo.emplace_back(*state, e.first, count, 0, player,
					                                             bases[d.first]->building);
					bases[s.first]->inventoryBioEquipment[e.first.id] -= count;
					if (remaining == 0)
					{
						break;
					}
				}
				if (remaining != 0)
				{
					LogError("General screw up in transactions, have remaining demand of %d for %s",
					         remaining, e.first.id);
				}
			}
		}
		// Transfer Vehicle Equipment
		for (auto &e : veMap)
		{
			std::list<std::pair<int, int>> source;
			std::list<std::pair<int, int>> destination;
			for (auto &ve : e.second)
			{
				if (ve.second < 0)
				{
					source.emplace_back(ve.first, ve.second);
				}
				if (ve.second > 0)
				{
					destination.emplace_back(ve.first, ve.second);
				}
			}
			for (auto &d : destination)
			{
				int remaining = d.second;
				for (auto &s : source)
				{
					if (s.second == 0)
					{
						continue;
					}
					int count = std::min(remaining, s.second);
					s.second -= count;
					remaining -= count;
					bases[s.first]->building->cargo.emplace_back(*state, e.first, count, 0, player,
					                                             bases[d.first]->building);
					bases[s.first]->inventoryVehicleEquipment[e.first.id] -= count;
					if (remaining == 0)
					{
						break;
					}
				}
				if (remaining != 0)
				{
					LogError("General screw up in transactions, have remaining demand of %d for %s",
					         remaining, e.first.id);
				}
			}
		}
		// Transfer Vehicle Ammo
		for (auto &e : vaMap)
		{
			std::list<std::pair<int, int>> source;
			std::list<std::pair<int, int>> destination;
			for (auto &va : e.second)
			{
				if (va.second < 0)
				{
					source.emplace_back(va.first, va.second);
				}
				if (va.second > 0)
				{
					destination.emplace_back(va.first, va.second);
				}
			}
			for (auto &d : destination)
			{
				int remaining = d.second;
				for (auto &s : source)
				{
					if (s.second == 0)
					{
						continue;
					}
					int count = std::min(remaining, s.second);
					s.second -= count;
					remaining -= count;
					bases[s.first]->building->cargo.emplace_back(*state, e.first, count, 0, player,
					                                             bases[d.first]->building);
					bases[s.first]->inventoryVehicleAmmo[e.first.id] -= count;
					if (remaining == 0)
					{
						break;
					}
				}
				if (remaining != 0)
				{
					LogError("General screw up in transactions, have remaining demand of %d for %s",
					         remaining, e.first.id);
				}
			}
		}
		// Vehicles and Agents already processed above
		return;
	}
}
}