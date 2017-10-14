#include "game/ui/base/buysellscreen.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/radiobutton.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/rules/city/vequipmenttype.h"
#include "game/ui/components/controlgenerator.h"
#include "library/strings_format.h"

namespace OpenApoc
{

BuySellScreen::BuySellScreen(sp<GameState> state) : BaseStage(state)
{
	form = ui().getForm("purchasescreen");
	viewHighlight = BaseGraphics::FacilityHighlight::Stores;

	form->findControlTyped<RadioButton>("BUTTON_VEHICLES")->setChecked(true);

	form->findControlTyped<RadioButton>("BUTTON_VEHICLES")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->changeBase(this->state->current_base); });
	form->findControlTyped<RadioButton>("BUTTON_AGENTS")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->changeBase(this->state->current_base); });
	form->findControlTyped<RadioButton>("BUTTON_FLYING")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->changeBase(this->state->current_base); });
	form->findControlTyped<RadioButton>("BUTTON_GROUND")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->changeBase(this->state->current_base); });
}

BuySellScreen::~BuySellScreen() = default;

void BuySellScreen::changeBase(sp<Base> newBase)
{
	BaseStage::changeBase(newBase);

	// Just temporary code that tests the form

	form->findControlTyped<ScrollBar>("LIST_SCROLL")->setValue(0);

	auto list = form->findControlTyped<ListBox>("LIST");
	list->clear();

	if (form->findControlTyped<RadioButton>("BUTTON_VEHICLES")->isChecked())
	{
		for (auto &v : state->vehicle_types)
		{
			if (state->economy.find(v.first) != state->economy.end())
			{
				auto control = ControlGenerator::createPurchaseControl(
				    *state, StateRef<VehicleType>{state.get(), v.first}, 0);
				if (control)
				{
					list->addItem(control);
				}
			}
		}
	}
	if (form->findControlTyped<RadioButton>("BUTTON_FLYING")->isChecked() ||
	    form->findControlTyped<RadioButton>("BUTTON_GROUND")->isChecked())
	{
		bool flying = form->findControlTyped<RadioButton>("BUTTON_FLYING")->isChecked();
		static const std::list<EquipmentSlotType> types = {EquipmentSlotType::VehicleWeapon,
		                                                   EquipmentSlotType::VehicleGeneral,
		                                                   EquipmentSlotType::VehicleEngine};
		for (auto &t : types)
		{
			StateRef<VAmmoType> ammoType;
			for (auto &ve : state->vehicle_equipment)
			{
				if (ve.second->type != t)
				{
					continue;
				}
				if (flying &&
				    ve.second->users.find(VEquipmentType::User::Air) == ve.second->users.end())
				{
					continue;
				}
				if (!flying &&
				    ve.second->users.find(VEquipmentType::User::Ground) == ve.second->users.end())
				{
					continue;
				}
				if (state->economy.find(ve.first) != state->economy.end())
				{
					if (ammoType && ve.second->ammo_type != ammoType)
					{
						if (state->economy.find(ammoType.id) != state->economy.end())
						{
							auto controlAmmo = ControlGenerator::createPurchaseControl(
							    *state, ammoType,
							    state->current_base->inventoryVehicleAmmo[ammoType.id]);
							if (controlAmmo)
							{
								list->addItem(controlAmmo);
							}
						}
						ammoType = nullptr;
					}
					auto control = ControlGenerator::createPurchaseControl(
					    *state, StateRef<VEquipmentType>{state.get(), ve.first},
					    state->current_base->inventoryVehicleEquipment[ve.first]);
					if (control)
					{
						list->addItem(control);
					}
					if (ve.second->ammo_type)
					{
						ammoType = ve.second->ammo_type;
					}
				}
			}
			if (ammoType)
			{
				if (state->economy.find(ammoType.id) != state->economy.end())
				{
					auto controlAmmo = ControlGenerator::createPurchaseControl(
					    *state, ammoType, state->current_base->inventoryVehicleAmmo[ammoType.id]);
					if (controlAmmo)
					{
						list->addItem(controlAmmo);
					}
				}
			}
		}
	}
	if (form->findControlTyped<RadioButton>("BUTTON_AGENTS")->isChecked())
	{
		for (auto &ae : state->agent_equipment)
		{
			if (ae.second->type == AEquipmentType::Type::Ammo)
			{
				continue;
			}
			if (state->economy.find(ae.first) != state->economy.end())
			{
				auto control = ControlGenerator::createPurchaseControl(
				    *state, StateRef<AEquipmentType>{state.get(), ae.first},
				    state->current_base->inventoryAgentEquipment[ae.first]);
				if (control)
				{
					list->addItem(control);
				}
			}
			for (auto &ammo : ae.second->ammo_types)
			{
				if (state->economy.find(ammo.id) != state->economy.end())
				{
					int divisor = ammo->type == AEquipmentType::Type::Ammo ? ammo->max_ammo : 1;
					auto controlAmmo = ControlGenerator::createPurchaseControl(
					    *state, ammo,
					    state->current_base->inventoryAgentEquipment[ammo.id] / divisor);
					if (controlAmmo)
					{
						list->addItem(controlAmmo);
					}
				}
			}
		}
	}
}

void BuySellScreen::begin() { BaseStage::begin(); }

void BuySellScreen::pause() {}

void BuySellScreen::resume()
{
	form->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
}

void BuySellScreen::finish() {}

void BuySellScreen::eventOccurred(Event *e)
{
	form->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION)
	{
		if (e->forms().EventFlag == FormEventType::ButtonClick)
		{
			if (e->forms().RaisedBy->Name == "BUTTON_OK")
			{
				fw().stageQueueCommand({StageCmd::Command::POP});
				return;
			}
		}
	}
}

void BuySellScreen::update() { form->update(); }

void BuySellScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	form->render();
	BaseStage::render();
}

bool BuySellScreen::isTransition() { return false; }

}; // namespace OpenApoc
