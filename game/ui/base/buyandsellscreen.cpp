#include "game/ui/base/buyandsellscreen.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/radiobutton.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/renderer.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/shared/organisation.h"
#include "game/ui/general/messagebox.h"
#include "game/ui/general/transactioncontrol.h"
#include <array>

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

	// Adding callbacks after checking the button because we don't need to
	// have the callback be called since changeBase() will update display anyways

	form->findControlTyped<RadioButton>("BUTTON_VEHICLES")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(Type::Vehicle); });
	form->findControlTyped<RadioButton>("BUTTON_AGENTS")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(Type::AgentEquipment); });
	form->findControlTyped<RadioButton>("BUTTON_FLYING")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(Type::FlyingEquipment); });
	form->findControlTyped<RadioButton>("BUTTON_GROUND")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(Type::GroundEquipment); });

	confirmClosureText = tr("Confirm Sales/Purchases");

	type = Type::Vehicle;
	form->findControlTyped<RadioButton>("BUTTON_VEHICLES")->setChecked(true);
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
	auto player = state->getPlayer();

	// Step 01: Check funds
	{
		if (player->balance + moneyDelta < 0)
		{
			fw().stageQueueCommand({StageCmd::Command::PUSH,
			                        mksp<MessageBox>(tr("Funds exceeded"),
			                                         tr("Order limited by your available funds."),
			                                         MessageBox::ButtonOptions::Ok)});
			return;
		}
	}

	// Step 02: Check accommodation of different sorts
	{
		std::array<int, MAX_BASES> vecCargoDelta;
		std::array<bool, MAX_BASES> vecChanged;
		vecCargoDelta.fill(0);
		vecChanged.fill(false);

		// Find all delta and mark all that have any changes
		for (auto &l : transactionControls)
		{
			for (auto &c : l.second)
			{
				if (!c->getLinked() || c->getLinked()->front().lock() == c)
				{
					int i = 0;
					for ([[maybe_unused]] const auto &b : state->player_bases)
					{
						int cargoDelta = c->getCargoDelta(i);
						if (cargoDelta)
						{
							vecCargoDelta[i] += cargoDelta;
							vecChanged[i] = true;
						}
						i++;
					}
				}
			}
		}

		// Check every base, find first bad one
		int i = 0;
		StateRef<Base> bad_base;
		for (auto &b : state->player_bases)
		{
			if ((vecChanged[i] || forceLimits) && vecCargoDelta[i] > 0 &&
			    b.second->getUsage(*state, FacilityType::Capacity::Stores, vecCargoDelta[i]) > 100)
			{
				bad_base = b.second->building->base;
				break;
			}
			i++;
		}

		// Found bad base
		if (bad_base)
		{
			UString title(tr("Storage space exceeded"));
			UString message(forceLimits
			                    ? tr("Storage space exceeded. Sell off more items!")
			                    : tr("Order limited by the available storage space at this base."));

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

	// Step 03: Check transportation

	// Step 03.01: Check transportation for purchases
	bool purchaseTransferFound = false;
	{
		// Find all orgs that we buy from
		std::set<StateRef<Organisation>> orgsBuyFrom;
		for (auto &l : transactionControls)
		{
			for (auto &c : l.second)
			{
				if (!c->getLinked() || c->getLinked()->front().lock() == c)
				{
					// See if we transfer-bought from an org that is not hostile
					if (c->tradeState.shipmentsFrom(ECONOMY_IDX) > 0)
					{
						switch (c->itemType)
						{
							case TransactionControl::Type::AgentEquipmentCargo:
							case TransactionControl::Type::VehicleEquipment:
							case TransactionControl::Type::VehicleAmmo:
								orgsBuyFrom.insert(c->manufacturer);
								break;
							default:
								// Other types find their own transportation
								break;
						}
					}
				}
			}
		}
		purchaseTransferFound = !orgsBuyFrom.empty();

		// Check orgs
		std::list<StateRef<Organisation>> badOrgs;
		bool transportationHostile = false;
		for (auto &o : orgsBuyFrom)
		{
			if (o == player)
			{
				continue;
			}
			// Expecting all bases to be in one city
			auto canBuy = o->canPurchaseFrom(*state, state->current_base->building, false);
			switch (canBuy)
			{
				case Organisation::PurchaseResult::NoTransportAvailable:
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
				case Organisation::PurchaseResult::OK:
					// Everything went fine
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
				UString message = transportationHostile
				                      ? format("%s %s",
				                               tr("Hostile organization refuses to carry out the "
				                                  "requested transportation for this company."),
				                               tr("Proceed?"))
				                      : format("%s %s",
				                               tr("No free transport to carry out the requested "
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
				UString message = format("%s %s",
				                         tr("No free transport to carry out the requested "
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
				     mksp<MessageBox>(title,
				                      tr("Hostile organization refuses to carry out the "
				                         "requested transportation for this company."),
				                      MessageBox::ButtonOptions::Ok)});
				return;
			}
		}
	}

	// Step 03.02: Check transportation for purchase-transfers
	if (purchaseTransferFound)
	{
		// Find out who provides transportation services
		std::list<StateRef<Organisation>> badOrgs;
		std::list<StateRef<Organisation>> ferryCompanies;
		for (auto &o : state->organisations)
		{
			if (o.second->providesTransportationServices)
			{
				StateRef<Organisation> org{state.get(), o.first};
				if (o.second->isRelatedTo(player) == Organisation::Relation::Hostile)
				{
					badOrgs.push_back(org);
				}
				else
				{
					ferryCompanies.push_back(org);
				}
			}
		}

		bool transportationBusy = false;
		bool transportationHostile = ferryCompanies.empty();
		if (!transportationHostile)
		{
			// Check if ferry provider has free ferries
			// TODO: rewrite
			if (config().getBool("OpenApoc.NewFeature.CallExistingFerry"))
			{
				bool ferryFound = false;
				for (auto &o : ferryCompanies)
				{
					for (auto &v : state->vehicles)
					{
						if (v.second->owner != o ||
						    (!v.second->type->provideFreightCargo &&
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
				        ? format("%s %s",
				                 tr("This hostile organization refuses to carry out the "
				                    "requested transfer."),
				                 tr("Proceed?"))
				        : format("%s %s",
				                 tr("No free transport to carry out the requested "
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
				UString message = format("%s %s",
				                         tr("No free transport to carry out the requested "
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
				     mksp<MessageBox>(title,
				                      tr("This hostile organization refuses to carry out "
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
	auto player = state->getPlayer();
	for (auto &l : transactionControls)
	{
		for (auto &c : l.second)
		{
			if (!c->getLinked() || c->getLinked()->front().lock() == c)
			{
				if (c->itemType != TransactionControl::Type::Vehicle &&
				    state->economy.find(c->itemId) == state->economy.end())
				{
					LogError("Economy not found for %s: How are we selling it then!?", c->itemId);
					continue;
				}

				int i = 0;
				auto &economy = state->economy[c->itemId];
				for (auto &b : state->player_bases)
				{
					int order = c->tradeState.shipmentsTotal(i++);

					// Sell
					if (order > 0)
					{
						switch (c->itemType)
						{
							case TransactionControl::Type::Vehicle:
							{
								StateRef<Vehicle> vehicle{state.get(), c->itemId};
								// Expecting sold vehicle to be parked
								// Offload agents
								while (!vehicle->currentAgents.empty())
								{
									auto agent = *vehicle->currentAgents.begin();
									agent->enterBuilding(*state, vehicle->currentBuilding);
								}
								// Offload cargo
								for (auto &c : vehicle->cargo)
								{
									vehicle->currentBuilding->cargo.push_back(c);
								}
								vehicle->die(*state, true);
								player->balance += c->price;
								break;
							}
							case TransactionControl::Type::AgentEquipmentBio:
							{
								// kill aliens
								b.second->inventoryAgentEquipment[c->itemId] -= order;
								break;
							}
							case TransactionControl::Type::AgentEquipmentCargo:
							{
								economy.currentStock += order;
								player->balance += order * economy.currentPrice;
								StateRef<AEquipmentType> equipment{state.get(), c->itemId};

								const auto numItemsPerUnit =
								    equipment->type == AEquipmentType::Type::Ammo
								        ? equipment->max_ammo
								        : 1;
								auto numItems = numItemsPerUnit * order;

								LogAssert(b.second->inventoryAgentEquipment[c->itemId] <=
								          std::numeric_limits<int>::max());
								numItems = std::min(
								    numItems, (int)b.second->inventoryAgentEquipment[c->itemId]);

								b.second->inventoryAgentEquipment[c->itemId] -= numItems;
								break;
							}
							case TransactionControl::Type::VehicleAmmo:
							{
								economy.currentStock += order;
								player->balance += order * economy.currentPrice;
								b.second->inventoryVehicleAmmo[c->itemId] -= order;
								break;
							}
							case TransactionControl::Type::VehicleEquipment:
							{
								economy.currentStock += order;
								player->balance += order * economy.currentPrice;
								b.second->inventoryVehicleEquipment[c->itemId] -= order;
								break;
							}
							case TransactionControl::Type::VehicleType:
							{
								LogError("How did we manage to sell a vehicle type %s!?",
								         c->itemId);
								break;
							}
							case TransactionControl::Type::Soldier:
							case TransactionControl::Type::BioChemist:
							case TransactionControl::Type::Physicist:
							case TransactionControl::Type::Engineer:
							{
								LogError("How did we manage to sell an agent type %s!", c->itemId);
								break;
							}
						}
					}

					// Buy
					else if (order < 0)
					{
						auto org = c->manufacturer;
						if (org->isRelatedTo(player) == Organisation::Relation::Hostile)
						{
							LogError("How the hell is being bought from a hostile org %s?",
							         c->manufacturerName);
							continue;
						}

						switch (c->itemType)
						{
							case TransactionControl::Type::Vehicle:
							{
								LogError("It should be impossible to buy a particular vehicle %s.",
								         c->itemId);
								break;
							}
							case TransactionControl::Type::AgentEquipmentBio:
							{
								LogError("Alien %s: How are we buying it!?", c->itemId);
								break;
							}
							case TransactionControl::Type::AgentEquipmentCargo:
							{
								StateRef<AEquipmentType> equipment{state.get(), c->itemId};
								org->purchase(*state, b.second->building, equipment, -order);
								break;
							}
							case TransactionControl::Type::VehicleAmmo:
							{
								StateRef<VAmmoType> ammo{state.get(), c->itemId};
								org->purchase(*state, b.second->building, ammo, -order);
								break;
							}
							case TransactionControl::Type::VehicleEquipment:
							{
								StateRef<VEquipmentType> equipment{state.get(), c->itemId};
								org->purchase(*state, b.second->building, equipment, -order);
								break;
							}
							case TransactionControl::Type::VehicleType:
							{
								StateRef<VehicleType> vehicle{state.get(), c->itemId};
								org->purchase(*state, b.second->building, vehicle, -order);
								break;
							}
							case TransactionControl::Type::Soldier:
							case TransactionControl::Type::BioChemist:
							case TransactionControl::Type::Physicist:
							case TransactionControl::Type::Engineer:
							{
								LogError("How did we manage to sell an agent type %s!", c->itemId);
								break;
							}
						}
					}
				}
			}
		}
	}
	// Rest in peace, vehicles
	state->cleanUpDeathNote();
}

}; // namespace OpenApoc
