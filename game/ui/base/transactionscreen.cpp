#include "game/ui/base/transactionscreen.h"
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
#include "framework/renderer.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/rules/city/vequipmenttype.h"
#include "game/ui/components/controlgenerator.h"
#include "library/strings_format.h"

namespace OpenApoc
{

TransactionScreen::TransactionScreen(sp<GameState> state, TransactionScreen::Mode mode)
    : BaseStage(state), mode(mode)
{
	form = ui().getForm("transactionscreen");

	switch (mode)
	{
		case Mode::AlienContainment:
			form->findControlTyped<Label>("TITLE")->setText(tr("ALIEN CONTAINMENT"));
			form->findControlTyped<Graphic>("BG")->setImage(
			    fw().data->loadImage("xcom3/ufodata/aliencon.pcx"));

			form->findControlTyped<RadioButton>("BUTTON_SOLDIERS")->setVisible(false);
			form->findControlTyped<RadioButton>("BUTTON_BIOSCIS")->setVisible(false);
			form->findControlTyped<RadioButton>("BUTTON_PHYSCIS")->setVisible(false);
			form->findControlTyped<RadioButton>("BUTTON_ENGINRS")->setVisible(false);
			form->findControlTyped<RadioButton>("BUTTON_ALIENS")->setVisible(false);

			form->findControlTyped<RadioButton>("BUTTON_VEHICLES")->setVisible(false);
			form->findControlTyped<RadioButton>("BUTTON_AGENTS")->setVisible(false);
			form->findControlTyped<RadioButton>("BUTTON_FLYING")->setVisible(false);
			form->findControlTyped<RadioButton>("BUTTON_GROUND")->setVisible(false);
			break;
		case Mode::BuySell:
			viewHighlight = BaseGraphics::FacilityHighlight::Stores;
			form->findControlTyped<Label>("TITLE")->setText(tr("BUY AND SELL"));
			form->findControlTyped<Graphic>("BG")->setImage(
			    fw().data->loadImage("xcom3/ufodata/buy&sell.pcx"));

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
			break;
		case Mode::Transfer:
			form->findControlTyped<Label>("TITLE")->setText(tr("TRANSFER"));
			form->findControlTyped<Graphic>("BG")->setImage(
			    fw().data->loadImage("xcom3/ufodata/transfer.pcx"));

			form->findControlTyped<RadioButton>("BUTTON_SOLDIERS")->setVisible(true);
			form->findControlTyped<RadioButton>("BUTTON_BIOSCIS")->setVisible(true);
			form->findControlTyped<RadioButton>("BUTTON_PHYSCIS")->setVisible(true);
			form->findControlTyped<RadioButton>("BUTTON_ENGINRS")->setVisible(true);
			form->findControlTyped<RadioButton>("BUTTON_ALIENS")->setVisible(true);

			form->findControlTyped<RadioButton>("BUTTON_VEHICLES")->setVisible(true);
			form->findControlTyped<RadioButton>("BUTTON_VEHICLES")->Location.y = 200;
			form->findControlTyped<RadioButton>("BUTTON_AGENTS")->setVisible(true);
			form->findControlTyped<RadioButton>("BUTTON_AGENTS")->Location.y = 240;
			form->findControlTyped<RadioButton>("BUTTON_FLYING")->setVisible(true);
			form->findControlTyped<RadioButton>("BUTTON_FLYING")->Location.y = 280;
			form->findControlTyped<RadioButton>("BUTTON_GROUND")->setVisible(true);
			form->findControlTyped<RadioButton>("BUTTON_GROUND")->Location.y = 320;

			break;
	}
}

TransactionScreen::~TransactionScreen() = default;

void TransactionScreen::changeBase(sp<Base> newBase)
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
		static const std::list<EquipmentSlotType> vehTypes = {EquipmentSlotType::VehicleWeapon,
		                                                      EquipmentSlotType::VehicleGeneral,
		                                                      EquipmentSlotType::VehicleEngine};
		for (auto &t : vehTypes)
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
		static const std::list<AEquipmentType::Type> agTypes = {
		    AEquipmentType::Type::Grenade, AEquipmentType::Type::Weapon,
		    // Ammo means everything else
		    AEquipmentType::Type::Ammo, AEquipmentType::Type::Armor, AEquipmentType::Type::Loot,
		};

		for (auto &t : agTypes)
		{
			for (auto &ae : state->agent_equipment)
			{
				if (ae.second->type == AEquipmentType::Type::Ammo)
				{
					continue;
				}
				if (t == AEquipmentType::Type::Ammo)
				{
					if (std::find(agTypes.begin(), agTypes.end(), ae.second->type) != agTypes.end())
					{
						continue;
					}
				}
				else
				{
					if (ae.second->type != t)
					{
						continue;
					}
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
}

void TransactionScreen::begin() { BaseStage::begin(); }

void TransactionScreen::pause() {}

void TransactionScreen::resume()
{
	form->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
}

void TransactionScreen::finish() {}

void TransactionScreen::eventOccurred(Event *e)
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

void TransactionScreen::update() { form->update(); }

void TransactionScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	form->render();
	BaseStage::render();
}

bool TransactionScreen::isTransition() { return false; }

void TransactionScreen::TransactionControl::setIndex(int indexLeft, int indexRight)
{
	this->indexLeft = indexLeft;
	this->indexRight = indexRight;

	if (indexLeft == indexRight)
	{
		scrollBar->Minimum = 0;
		scrollBar->Maximum = 0;
		scrollBar->setValue(0);
	}
	else
	{
		scrollBar->Minimum = 0;
		scrollBar->Maximum = currentStock[indexLeft] + currentStock[indexRight];
		scrollBar->setValue(currentStock[indexRight]);
	}

	updateValues();
}

void TransactionScreen::TransactionControl::updateValues()
{
	if (scrollBar->Maximum != 0)
	{
		int newRight = scrollBar->getValue();
		int newLeft = scrollBar->Maximum - scrollBar->getValue();
		if (newRight != currentStock[indexRight] || newLeft != currentStock[indexLeft])
		{
			currentStock[indexRight] = newRight;
			currentStock[indexLeft] = newLeft;
			this->pushFormEvent(FormEventType::ScrollBarChange, nullptr);
		}
	}
	stockLeft->setText(format("%d", currentStock[indexLeft]));
	stockRight->setText(format("%d", currentStock[indexRight]));
	int curDeltaLeft = currentStock[indexLeft] - initialStock[indexLeft];
	int curDeltaRight = currentStock[indexRight] - initialStock[indexRight];
	deltaLeft->setText(format("%s%d", curDeltaLeft > 0 ? "+" : "", curDeltaLeft));
	deltaLeft->setText(format("%s%d", curDeltaRight > 0 ? "+" : "", curDeltaRight));
	deltaLeft->setVisible(indexLeft != 8 && curDeltaLeft != 0);
	deltaRight->setVisible(indexRight != 8 && curDeltaRight != 0);
}

TransactionScreen::TransactionControl::TransactionControl(UString id, Type type, UString name,
                                                          UString manufacturer, bool isAmmo,
                                                          bool isBio, int price, int storeSpace,
                                                          std::vector<int> initialStock)
    : itemId(id), itemType(type), isAmmo(isAmmo), isBio(isBio), price(price),
      storeSpace(storeSpace), initialStock(initialStock), currentStock(initialStock)
{
	// Setup vars and load resources

	Size = Vec2<int>{173 + 178 - 2, 47};

	bgLeft = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 45));
	bgRight = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 45));

	purchaseBoxIcon = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 47));
	purchaseXComIcon = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 48));
	purchaseArrow = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 52));

	alienContainedDetain = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 75));
	alienContainedKill = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 76));

	scrollLeft = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 53));
	scrollRight = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 54));

	transactionShade = fw().data->loadImage("city/transaction-shade.png");

	auto labelFont = ui().getFont("smalfont");

	// Add controls

	// Name
	if (name.length() > 0)
	{
		auto label = createChild<Label>(name, labelFont);
		label->Location = {isAmmo ? 32 : 11, 3};
		label->Size = {256, 16};
		label->TextHAlign = HorizontalAlignment::Left;
		label->TextVAlign = VerticalAlignment::Centre;
	}
	// Manufacturer
	if (manufacturer.length() > 0)
	{
		auto label = createChild<Label>(manufacturer, labelFont);
		label->Location = {34, 3};
		label->Size = {256, 16};
		label->TextHAlign = HorizontalAlignment::Right;
		label->TextVAlign = VerticalAlignment::Centre;
	}
	// Price
	if (price != 0)
	{
		auto label = createChild<Label>(format("$%d", price), labelFont);
		label->Location = {290, 3};
		label->Size = {47, 16};
		label->TextHAlign = HorizontalAlignment::Right;
		label->TextVAlign = VerticalAlignment::Centre;
	}
	// Stock (values set in updateValues)
	stockLeft = createChild<Label>("", labelFont);
	stockLeft->Location = {11, 26};
	stockLeft->Size = {32, 14};
	stockLeft->TextHAlign = HorizontalAlignment::Right;
	stockLeft->TextVAlign = VerticalAlignment::Centre;
	stockRight = createChild<Label>("", labelFont);
	stockRight->Location = {303, 26};
	stockRight->Size = {32, 14};
	stockRight->TextHAlign = HorizontalAlignment::Right;
	stockRight->TextVAlign = VerticalAlignment::Centre;
	// Delta (values set in updateValues)
	deltaLeft = createChild<Label>("", labelFont);
	deltaLeft->Location = {50, 26};
	deltaLeft->Size = {32, 14};
	deltaLeft->TextHAlign = HorizontalAlignment::Right;
	deltaLeft->TextVAlign = VerticalAlignment::Centre;
	deltaRight = createChild<Label>("", labelFont);
	deltaRight->Location = {264, 26};
	deltaRight->Size = {30, 14};
	deltaRight->TextHAlign = HorizontalAlignment::Right;
	deltaRight->TextVAlign = VerticalAlignment::Centre;
	// ScrollBar
	scrollBar = createChild<ScrollBar>();
	scrollBar->Location = {102, 24};
	scrollBar->Size = {147, 20};
	scrollBar->Minimum = 0;
	scrollBar->Maximum = 0;
	// ScrollBar buttons
	auto buttonScrollLeft = createChild<GraphicButton>(nullptr, scrollLeft);
	buttonScrollLeft->Size = scrollLeft->size;
	buttonScrollLeft->Location = {87, 24};
	buttonScrollLeft->ScrollBarPrev = scrollBar;
	auto buttonScrollRight = createChild<GraphicButton>(nullptr, scrollRight);
	buttonScrollRight->Size = scrollRight->size;
	buttonScrollRight->Location = {247, 24};
	buttonScrollRight->ScrollBarNext = scrollBar;
	// Callback
	std::function<void(FormsEvent * e)> onScrollChange = [this](Event *) { this->updateValues(); };
	scrollBar->addCallback(FormEventType::ScrollBarChange, onScrollChange);
}

int TransactionScreen::TransactionControl::getStoreDelta(int index) const
{
	return (currentStock[index] - initialStock[index]) * storeSpace;
}

int TransactionScreen::TransactionControl::getPriceDelta(int index) const
{
	return (currentStock[index] - initialStock[index]) * price;
}

void TransactionScreen::TransactionControl::onRender()
{
	static Vec2<int> bgLeftPos = {0, 2};
	static Vec2<int> bgRightPos = {172, 2};
	static Vec2<int> ammoPos = {4, 2};
	static Vec2<int> iconLeftPos = {58, 24};
	static Vec2<int> iconRightPos = {270, 24};
	static Vec2<int> iconSize = {22, 20};

	// Draw BG
	fw().renderer->draw(bgLeft, bgLeftPos);
	fw().renderer->draw(bgRight, bgRightPos);
	// Draw Ammo Arrow
	if (isAmmo)
	{
		fw().renderer->draw(purchaseArrow, ammoPos);
	}
	// Draw Icons
	if (!deltaLeft->isVisible())
	{
		sp<Image> icon;
		if (isBio)
		{
			icon = indexLeft == 8 ? alienContainedKill : alienContainedDetain;
		}
		else
		{
			icon = indexLeft == 8 ? purchaseBoxIcon : purchaseXComIcon;
		}
		auto iconPos = iconLeftPos + (iconSize - (Vec2<int>)icon->size) / 2;
		fw().renderer->draw(icon, iconPos);
	}
	if (!deltaRight->isVisible())
	{
		sp<Image> icon;
		if (isBio)
		{
			icon = indexRight == 8 ? alienContainedKill : alienContainedDetain;
		}
		else
		{
			icon = indexRight == 8 ? purchaseBoxIcon : purchaseXComIcon;
		}
		auto iconPos = iconRightPos + (iconSize - (Vec2<int>)icon->size) / 2;
		fw().renderer->draw(icon, iconPos);
	}
}

void TransactionScreen::TransactionControl::postRender()
{
	Control::postRender();

	// Draw shade if inactive
	static Vec2<int> shadePos = {0, 0};
	if (indexLeft == indexRight || (currentStock[indexLeft == 0] && currentStock[indexRight] == 0))
	{
		fw().renderer->draw(transactionShade, shadePos);
	}
}

void TransactionScreen::TransactionControl::unloadResources()
{
	bgLeft.reset();
	bgRight.reset();

	purchaseBoxIcon.reset();
	purchaseXComIcon.reset();
	purchaseArrow.reset();

	alienContainedDetain.reset();
	alienContainedKill.reset();

	scrollLeft.reset();
	scrollRight.reset();

	transactionShade.reset();

	Control::unloadResources();
}

}; // namespace OpenApoc
