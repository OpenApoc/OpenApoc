#include "game/ui/base/basescreen.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/textedit.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/facility.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/ufopaedia.h"
#include "game/ui/base/recruitscreen.h"
#include "game/ui/base/researchscreen.h"
#include "game/ui/base/transactionscreen.h"
#include "game/ui/base/vequipscreen.h"
#include "game/ui/components/basegraphics.h"
#include "game/ui/general/aequipscreen.h"
#include "game/ui/general/messagebox.h"
#include "game/ui/ufopaedia/ufopaediacategoryview.h"
#include "library/strings_format.h"

namespace OpenApoc
{

const Vec2<int> BaseScreen::NO_SELECTION = {-1, -1};

BaseScreen::BaseScreen(sp<GameState> state) : BaseStage(state), selection(NO_SELECTION), drag(false)
{
	form = ui().getForm("basescreen");
	viewHighlight = BaseGraphics::FacilityHighlight::Construction;
}

BaseScreen::~BaseScreen() = default;

void BaseScreen::changeBase(sp<Base> newBase)
{
	BaseStage::changeBase(newBase);
	form->findControlTyped<TextEdit>("TEXT_BASE_NAME")->setText(state->current_base->name);
	form->findControlTyped<Graphic>("GRAPHIC_MINIMAP")
	    ->setImage(BaseGraphics::drawMinimap(state, state->current_base->building));
}

void BaseScreen::begin()
{
	BaseStage::begin();

	baseView = form->findControlTyped<Graphic>("GRAPHIC_BASE_VIEW");
	selText = form->findControlTyped<Label>("TEXT_SELECTED_FACILITY");
	selGraphic = form->findControlTyped<Graphic>("GRAPHIC_SELECTED_FACILITY");
	for (int i = 0; i < 3; i++)
	{
		auto labelName = format("LABEL_%d", i + 1);
		auto label = form->findControlTyped<Label>(labelName);
		if (!label)
		{
			LogError("Failed to find UI control matching \"%s\"", labelName);
		}
		statsLabels.push_back(label);

		auto valueName = format("VALUE_%d", i + 1);
		auto value = form->findControlTyped<Label>(valueName);
		if (!value)
		{
			LogError("Failed to find UI control matching \"%s\"", valueName);
		}
		statsValues.push_back(value);
	}

	auto facilities = form->findControlTyped<ListBox>("LISTBOX_FACILITIES");
	for (auto &i : state->facility_types)
	{
		auto &facility = i.second;
		if (!facility->isVisible())
			continue;

		auto graphic = mksp<Graphic>(facility->sprite);
		graphic->AutoSize = true;
		graphic->setData(mksp<UString>(i.first));
		graphic->Name = "FACILITY_BUILD_TILE";
		facilities->addItem(graphic);
	}

	form->findControlTyped<GraphicButton>("BUTTON_OK")
	    ->addCallback(FormEventType::ButtonClick,
	                  [](Event *) { fw().stageQueueCommand({StageCmd::Command::POP}); });
	form->findControlTyped<GraphicButton>("BUTTON_BASE_BUYSELL")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH,
		         mksp<TransactionScreen>(state, TransactionScreen::Mode::BuySell)});
		});
	form->findControlTyped<GraphicButton>("BUTTON_BASE_HIREFIRESTAFF")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<RecruitScreen>(state)});
		});
	form->findControlTyped<GraphicButton>("BUTTON_BASE_TRANSFER")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    if (this->state->player_bases.size() <= 1)
		    {
			    fw().stageQueueCommand(
			        {StageCmd::Command::PUSH,
			         mksp<MessageBox>(
			             tr("Transfer"),
			             tr("At least two bases are required before transfers become possible."),
			             MessageBox::ButtonOptions::Ok)});
		    }
		    else
		    {
			    fw().stageQueueCommand(
			        {StageCmd::Command::PUSH,
			         mksp<TransactionScreen>(state, TransactionScreen::Mode::Transfer)});
		    }
		});
	form->findControlTyped<GraphicButton>("BUTTON_BASE_ALIEN_CONTAINMENT")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH,
		         mksp<TransactionScreen>(state, TransactionScreen::Mode::AlienContainment)});
		});
	form->findControlTyped<GraphicButton>("BUTTON_BASE_EQUIPAGENT")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    // FIXME: If you don't have any vehicles this button should do nothing
		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<AEquipScreen>(state)});
		});
	form->findControlTyped<GraphicButton>("BUTTON_BASE_EQUIPVEHICLE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    // FIXME: If you don't have any vehicles this button should do nothing
		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<VEquipScreen>(state)});
		});
	form->findControlTyped<GraphicButton>("BUTTON_BASE_RES_AND_MANUF")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    // FIXME: If you don't have any facilities this button should do nothing
		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<ResearchScreen>(state)});
		});
	// Base name edit
	form->findControlTyped<TextEdit>("TEXT_BASE_NAME")
	    ->addCallback(FormEventType::TextEditFinish, [this](FormsEvent *e) {
		    this->state->current_base->name =
		        std::dynamic_pointer_cast<TextEdit>(e->forms().RaisedBy)->getText();
		});
	form->findControlTyped<TextEdit>("TEXT_BASE_NAME")
	    ->addCallback(FormEventType::TextEditCancel, [this](FormsEvent *e) {
		    std::dynamic_pointer_cast<TextEdit>(e->forms().RaisedBy)
		        ->setText(this->state->current_base->name);
		});
}

void BaseScreen::pause() {}

void BaseScreen::resume() { textFunds->setText(state->getPlayerBalance()); }

void BaseScreen::finish() {}

void BaseScreen::eventOccurred(Event *e)
{
	form->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (form->findControlTyped<TextEdit>("TEXT_BASE_NAME")->isFocused())
			return;
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
		if (e->keyboard().KeyCode == SDLK_RETURN)
		{
			form->findControl("BUTTON_OK")->click();
			return;
		}
		if (e->keyboard().KeyCode == SDLK_F10)
		{
			for (auto &facility : state->current_base->facilities)
			{
				{
					facility->buildTime = 0;
				}
			}
			return;
		}
	}

	if (e->type() == EVENT_MOUSE_MOVE)
	{
		mousePos = {e->mouse().X, e->mouse().Y};
	}

	if (e->type() == EVENT_FORM_INTERACTION)
	{
		if (e->forms().RaisedBy == baseView)
		{
			if (e->forms().EventFlag == FormEventType::MouseMove)
			{
				selection = {e->forms().MouseInfo.X, e->forms().MouseInfo.Y};
				selection /= BaseGraphics::TILE_SIZE;
				if (!drag)
				{
					selFacility = state->current_base->getFacility(selection);
				}
				return;
			}
			else if (e->forms().EventFlag == FormEventType::MouseLeave)
			{
				selection = NO_SELECTION;
				selFacility = nullptr;
				return;
			}
		}
		if (e->forms().RaisedBy->Name == "LISTBOX_FACILITIES")
		{
			if (!drag && e->forms().EventFlag == FormEventType::ListBoxChangeHover)
			{
				auto list = form->findControlTyped<ListBox>("LISTBOX_FACILITIES");
				auto dragFacilityName = list->getHoveredData<UString>();
				if (dragFacilityName)
				{
					dragFacility = StateRef<FacilityType>{state.get(), *dragFacilityName};
					return;
				}
			}
		}
		if (e->forms().RaisedBy->Name == "FACILITY_BUILD_TILE")
		{
			if (!drag && e->forms().EventFlag == FormEventType::MouseLeave)
			{
				selection = NO_SELECTION;
				selFacility = nullptr;
				dragFacility = "";
			}
		}

		if (e->forms().EventFlag == FormEventType::MouseDown)
		{
			if (Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Left))
			{
				if (!drag && dragFacility)
				{
					if (e->forms().RaisedBy->Name == "LISTBOX_FACILITIES")
					{
						drag = true;
					}
				}
			}
			else if (Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Right))
			{
				sp<UString> clickedFacilityName;
				if (e->forms().RaisedBy->Name == "LISTBOX_FACILITIES")
				{
					auto list = std::dynamic_pointer_cast<ListBox>(e->forms().RaisedBy);
					clickedFacilityName = list->getHoveredData<UString>();
				}
				else if (e->forms().RaisedBy->Name == "GRAPHIC_BASE_VIEW")
				{
					if (selFacility)
						clickedFacilityName = mksp<UString>(selFacility->type.id);
				}

				StateRef<FacilityType> clickedFacility;
				if (clickedFacilityName)
					clickedFacility = StateRef<FacilityType>{state.get(), *clickedFacilityName};
				if (!clickedFacility)
					return;

				auto ufopaedia_entry = clickedFacility->ufopaedia_entry;
				sp<UfopaediaCategory> ufopaedia_category;
				if (ufopaedia_entry)
				{
					for (auto &cat : this->state->ufopaedia)
					{
						for (auto &entry : cat.second->entries)
						{
							if (ufopaedia_entry == entry.second)
							{
								ufopaedia_category = cat.second;
								break;
							}
						}
						if (ufopaedia_category)
							break;
					}
					if (!ufopaedia_category)
					{
						LogError("No UFOPaedia category found for entry %s",
						         ufopaedia_entry->title);
					}
					fw().stageQueueCommand(
					    {StageCmd::Command::PUSH,
					     mksp<UfopaediaCategoryView>(state, ufopaedia_category, ufopaedia_entry)});
				}
			}
		}

		if (e->forms().EventFlag == FormEventType::MouseUp)
		{
			// Facility construction
			if (drag && dragFacility)
			{
				if (selection != NO_SELECTION)
				{
					Base::BuildError error =
					    state->current_base->canBuildFacility(dragFacility, selection);
					switch (error)
					{
						case Base::BuildError::NoError:
							state->current_base->buildFacility(*state, dragFacility, selection);
							textFunds->setText(state->getPlayerBalance());
							refreshView();
							break;
						case Base::BuildError::Occupied:
							fw().stageQueueCommand(
							    {StageCmd::Command::PUSH,
							     mksp<MessageBox>(tr("Area Occupied By Existing Facility"),
							                      tr("Existing facilities in this area of the base "
							                         "must be destroyed "
							                         "before construction work can begin."),
							                      MessageBox::ButtonOptions::Ok)});
							break;
						case Base::BuildError::OutOfBounds:
							fw().stageQueueCommand(
							    {StageCmd::Command::PUSH,
							     mksp<MessageBox>(
							         tr("Planning Permission Denied"),
							         tr("Planning permission is denied for this proposed extension "
							            "to "
							            "the base, on the grounds that the additional excavations "
							            "required would seriously weaken the foundations of the "
							            "building."),
							         MessageBox::ButtonOptions::Ok)});
							break;
						case Base::BuildError::NoMoney:
							fw().stageQueueCommand(
							    {StageCmd::Command::PUSH,
							     mksp<MessageBox>(tr("Funds exceeded"),
							                      tr("The proposed construction work is not "
							                         "possible with your available funds."),
							                      MessageBox::ButtonOptions::Ok)});
							break;
						case Base::BuildError::Indestructible:
							// Indestrictible facilities (IE the access lift) are just silently
							// ignored
							break;
					}
				}
				drag = false;
				dragFacility = "";
			}
			// Facility removal
			else if (selFacility)
			{
				if (selection != NO_SELECTION)
				{
					Base::BuildError error =
					    state->current_base->canDestroyFacility(*state, selection);
					switch (error)
					{
						case Base::BuildError::NoError:
							fw().stageQueueCommand(
							    {StageCmd::Command::PUSH,
							     mksp<MessageBox>(tr("Destroy facility"), tr("Are you sure?"),
							                      MessageBox::ButtonOptions::YesNo, [this] {
								                      this->state->current_base->destroyFacility(
								                          *this->state, this->selection);
								                      this->refreshView();
								                  })});
							break;
						case Base::BuildError::Occupied:
							fw().stageQueueCommand(
							    {StageCmd::Command::PUSH,
							     mksp<MessageBox>(tr("Facility in use"), tr(""),
							                      MessageBox::ButtonOptions::Ok)});
						default:
							break;
					}
					selFacility = nullptr;
				}
			}
		}
	}

	selText->setText("");
	selGraphic->setImage(nullptr);
	for (auto &label : statsLabels)
	{
		label->setText("");
	}
	for (auto &value : statsValues)
	{
		value->setText("");
	}
	if (dragFacility)
	{
		selText->setText(tr(dragFacility->name));
		selGraphic->setImage(dragFacility->sprite);
		statsLabels[0]->setText(tr("Cost to build"));
		statsValues[0]->setText(format("$%d", dragFacility->buildCost));
		statsLabels[1]->setText(tr("Days to build"));
		statsValues[1]->setText(format("%d", dragFacility->buildTime));
		statsLabels[2]->setText(tr("Maintenance cost"));
		statsValues[2]->setText(format("$%d", dragFacility->weeklyCost));
	}
	else if (selFacility != nullptr)
	{
		selText->setText(tr(selFacility->type->name));
		selGraphic->setImage(selFacility->type->sprite);
		if (selFacility->type->capacityAmount > 0)
		{
			statsLabels[0]->setText(tr("Capacity"));
			statsValues[0]->setText(format("%d", selFacility->type->capacityAmount));
			statsLabels[1]->setText(tr("Usage"));
			statsValues[1]->setText(
			    format("%d%%", state->current_base->getUsage(*state, selFacility)));
		}
	}
	else if (selection != NO_SELECTION)
	{
		int sprite = BaseGraphics::getCorridorSprite(state->current_base, selection);
		auto image = format(
		    "PCK:xcom3/ufodata/base.pck:xcom3/ufodata/base.tab:%d:xcom3/ufodata/base.pcx", sprite);
		if (sprite != 0)
		{
			selText->setText(tr("Corridor"));
		}
		else
		{
			selText->setText(tr("Earth"));
		}
		selGraphic->setImage(fw().data->loadImage(image));
	}
}

void BaseScreen::update() { form->update(); }

void BaseScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	form->render();
	renderBase();
	BaseStage::render();
}

bool BaseScreen::isTransition() { return false; }

void BaseScreen::renderBase()
{
	const Vec2<int> BASE_POS = form->Location + baseView->Location;

	BaseGraphics::renderBase(BASE_POS, state->current_base);

	// Draw selection
	if (selection != NO_SELECTION)
	{
		Vec2<int> pos = selection;
		Vec2<int> size = {BaseGraphics::TILE_SIZE, BaseGraphics::TILE_SIZE};
		if (drag && dragFacility)
		{
			size *= dragFacility->size;
		}
		else if (selFacility != nullptr)
		{
			pos = selFacility->pos;
			size *= selFacility->type->size;
		}
		pos = BASE_POS + pos * BaseGraphics::TILE_SIZE;
		fw().renderer->drawRect(pos, size, Colour{255, 255, 255});
	}

	// Draw dragged facility
	if (drag && dragFacility)
	{
		sp<Image> facility = dragFacility->sprite;
		Vec2<int> pos;
		if (selection == NO_SELECTION)
		{
			pos = mousePos -
			      Vec2<int>{BaseGraphics::TILE_SIZE, BaseGraphics::TILE_SIZE} / 2 *
			          dragFacility->size;
		}
		else
		{
			pos = BASE_POS + selection * BaseGraphics::TILE_SIZE;
		}
		fw().renderer->draw(facility, pos);
	}
}

}; // namespace OpenApoc
