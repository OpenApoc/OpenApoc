#include "game/state/base/base.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "game/state/base/facility.h"
#include "game/ui/base/basegraphics.h"
#include "game/ui/base/basescreen.h"
#include "game/ui/base/researchscreen.h"
#include "game/ui/base/vequipscreen.h"
#include "game/ui/general/messagebox.h"

namespace OpenApoc
{

const Vec2<int> BaseScreen::NO_SELECTION = {-1, -1};

BaseScreen::BaseScreen(sp<GameState> state, StateRef<Base> base)
    : Stage(), form(ui().GetForm("FORM_BASESCREEN")), base(base), selection(NO_SELECTION),
      drag(false), state(state)
{
}

BaseScreen::~BaseScreen() {}

void BaseScreen::ChangeBase(sp<Base> newBase)
{
	base = {state.get(), newBase};
	form->FindControlTyped<TextEdit>("TEXT_BASE_NAME")->SetText(base->name);
	form->FindControlTyped<Graphic>("GRAPHIC_MINIMAP")
	    ->SetImage(BaseGraphics::drawMinimap(state, base->building));
}

void BaseScreen::Begin()
{
	ChangeBase(base);

	form->FindControlTyped<Label>("TEXT_FUNDS")->SetText(state->getPlayerBalance());

	baseView = form->FindControlTyped<Graphic>("GRAPHIC_BASE_VIEW");
	selText = form->FindControlTyped<Label>("TEXT_SELECTED_FACILITY");
	selGraphic = form->FindControlTyped<Graphic>("GRAPHIC_SELECTED_FACILITY");
	for (int i = 0; i < 3; i++)
	{
		auto labelName = UString::format("LABEL_%d", i + 1);
		auto label = form->FindControlTyped<Label>(labelName);
		if (!label)
		{
			LogError("Failed to find UI control matching \"%s\"", labelName.c_str());
		}
		statsLabels.push_back(label);

		auto valueName = UString::format("VALUE_%d", i + 1);
		auto value = form->FindControlTyped<Label>(valueName);
		if (!value)
		{
			LogError("Failed to find UI control matching \"%s\"", valueName.c_str());
		}
		statsValues.push_back(value);
	}
	int b = 0;
	for (auto &pair : state->player_bases)
	{
		auto &viewBase = pair.second;
		auto viewName = UString::format("BUTTON_BASE_%d", ++b);
		auto view = form->FindControlTyped<GraphicButton>(viewName);
		if (!view)
		{
			LogError("Failed to find UI control matching \"%s\"", viewName.c_str());
		}
		view->SetData(viewBase);
		auto viewImage =
		    BaseGraphics::drawMiniBase(viewBase, BaseGraphics::FacilityHighlight::Construction);
		view->SetImage(viewImage);
		view->SetDepressedImage(viewImage);
		view->addCallback(FormEventType::ButtonClick, [this](Event *e) {
			this->ChangeBase(e->Forms().RaisedBy->GetData<Base>());
		});
		miniViews.push_back(view);
	}

	auto facilities = form->FindControlTyped<ListBox>("LISTBOX_FACILITIES");
	for (auto &i : state->facility_types)
	{
		auto &facility = i.second;
		if (facility->fixed)
			continue;

		auto graphic = mksp<Graphic>(facility->sprite);
		graphic->AutoSize = true;
		graphic->SetData(mksp<UString>(i.first));
		graphic->Name = "FACILITY_BUILD_TILE";
		facilities->AddItem(graphic);
	}

	form->FindControlTyped<GraphicButton>("BUTTON_OK")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *e) { this->stageCmd.cmd = StageCmd::Command::POP; });
	form->FindControlTyped<GraphicButton>("BUTTON_BASE_EQUIPVEHICLE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *e) {
		    // FIXME: If you don't have any vehicles this button should do nothing
		    this->stageCmd.cmd = StageCmd::Command::PUSH;
		    this->stageCmd.nextStage = mksp<VEquipScreen>(state);
		});
	form->FindControlTyped<GraphicButton>("BUTTON_BASE_RES_AND_MANUF")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *e) {
		    // FIXME: If you don't have any facilities this button should do nothing
		    this->stageCmd.cmd = StageCmd::Command::PUSH;
		    this->stageCmd.nextStage = mksp<ResearchScreen>(state, this->base);
		});
	form->FindControlTyped<TextEdit>("TEXT_BASE_NAME")
	    ->addCallback(FormEventType::TextEditFinish, [this](Event *e) {
		    this->base->name = std::dynamic_pointer_cast<TextEdit>(e->Forms().RaisedBy)->GetText();
		});
}

void BaseScreen::Pause() {}

void BaseScreen::Resume() {}

void BaseScreen::Finish() {}

void BaseScreen::EventOccurred(Event *e)
{
	form->EventOccured(e);

	if (e->Type() == EVENT_KEY_DOWN)
	{
		if (e->Keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type() == EVENT_MOUSE_MOVE)
	{
		mousePos = {e->Mouse().X, e->Mouse().Y};
	}

	if (e->Type() == EVENT_FORM_INTERACTION)
	{
		if (e->Forms().RaisedBy == baseView)
		{
			if (e->Forms().EventFlag == FormEventType::MouseMove)
			{
				selection = {e->Forms().MouseInfo.X, e->Forms().MouseInfo.Y};
				selection /= BaseGraphics::TILE_SIZE;
				if (!drag)
				{
					selFacility = base->getFacility(selection);
				}
				return;
			}
			else if (e->Forms().EventFlag == FormEventType::MouseLeave)
			{
				selection = NO_SELECTION;
				selFacility = nullptr;
				return;
			}
		}
		if (e->Forms().RaisedBy->Name == "LISTBOX_FACILITIES")
		{
			if (!drag && e->Forms().EventFlag == FormEventType::ListBoxChangeHover)
			{
				auto list = form->FindControlTyped<ListBox>("LISTBOX_FACILITIES");
				auto dragFacilityName = list->GetHoveredData<UString>();
				if (dragFacilityName)
				{
					dragFacility = StateRef<FacilityType>{state.get(), *dragFacilityName};
					return;
				}
			}
		}
		if (e->Forms().RaisedBy->Name == "FACILITY_BUILD_TILE")
		{
			if (!drag && e->Forms().EventFlag == FormEventType::MouseLeave)
			{
				selection = NO_SELECTION;
				selFacility = nullptr;
				dragFacility = "";
			}
		}

		if (e->Forms().EventFlag == FormEventType::MouseDown)
		{
			if (!drag && dragFacility)
			{
				if (e->Forms().RaisedBy->Name == "LISTBOX_FACILITIES")
				{
					drag = true;
				}
			}
		}

		if (e->Forms().EventFlag == FormEventType::MouseUp)
		{
			// Facility construction
			if (drag && dragFacility)
			{
				if (selection != NO_SELECTION)
				{
					Base::BuildError error = base->canBuildFacility(dragFacility, selection);
					switch (error)
					{
						case Base::BuildError::NoError:
							base->buildFacility(*state, dragFacility, selection);
							form->FindControlTyped<Label>("TEXT_FUNDS")
							    ->SetText(state->getPlayerBalance());
							break;
						case Base::BuildError::Occupied:
							stageCmd.cmd = StageCmd::Command::PUSH;
							stageCmd.nextStage = mksp<MessageBox>(
							    tr("Area Occupied By Existing Facility"),
							    tr("Existing facilities in this area of the base must be destroyed "
							       "before construction work can begin."),
							    MessageBox::ButtonOptions::Ok);
							break;
						case Base::BuildError::OutOfBounds:
							stageCmd.cmd = StageCmd::Command::PUSH;
							stageCmd.nextStage = mksp<MessageBox>(
							    tr("Planning Permission Denied"),
							    tr("Planning permission is denied for this proposed extension to "
							       "the base, on the grounds that the additional excavations "
							       "required would seriously weaken the foundations of the "
							       "building."),
							    MessageBox::ButtonOptions::Ok);
							break;
						case Base::BuildError::NoMoney:
							stageCmd.cmd = StageCmd::Command::PUSH;
							stageCmd.nextStage = mksp<MessageBox>(
							    tr("Funds exceeded"), tr("The proposed construction work is not "
							                             "possible with your available funds."),
							    MessageBox::ButtonOptions::Ok);
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
					Base::BuildError error = base->canDestroyFacility(selection);
					switch (error)
					{
						case Base::BuildError::NoError:
							stageCmd.cmd = StageCmd::Command::PUSH;
							stageCmd.nextStage = mksp<MessageBox>(
							    tr("Destroy facility"), tr("Are you sure?"),
							    MessageBox::ButtonOptions::YesNo, [this] {
								    this->base->destroyFacility(*this->state, this->selection);
								    this->selFacility = nullptr;
								});
							break;
						case Base::BuildError::Occupied:
							stageCmd.cmd = StageCmd::Command::PUSH;
							stageCmd.nextStage = mksp<MessageBox>(tr("Facility in use"), tr(""),
							                                      MessageBox::ButtonOptions::Ok);
						default:
							break;
					}
				}
			}
		}
	}

	selText->SetText("");
	selGraphic->SetImage(nullptr);
	for (auto label : statsLabels)
	{
		label->SetText("");
	}
	for (auto value : statsValues)
	{
		value->SetText("");
	}
	if (dragFacility)
	{
		selText->SetText(tr(dragFacility->name));
		selGraphic->SetImage(dragFacility->sprite);
		statsLabels[0]->SetText(tr("Cost to build"));
		statsValues[0]->SetText(UString::format("$%d", dragFacility->buildCost));
		statsLabels[1]->SetText(tr("Days to build"));
		statsValues[1]->SetText(UString::format("%d", dragFacility->buildTime));
		statsLabels[2]->SetText(tr("Maintenance cost"));
		statsValues[2]->SetText(UString::format("$%d", dragFacility->weeklyCost));
	}
	else if (selFacility != nullptr)
	{
		selText->SetText(tr(selFacility->type->name));
		selGraphic->SetImage(selFacility->type->sprite);
		if (selFacility->type->capacityAmount > 0)
		{
			statsLabels[0]->SetText(tr("Capacity"));
			statsValues[0]->SetText(UString::format("%d", selFacility->type->capacityAmount));
			statsLabels[1]->SetText(tr("Usage"));
			statsValues[1]->SetText(UString::format("%d%%", 0));
		}
	}
	else if (selection != NO_SELECTION)
	{
		int sprite = BaseGraphics::getCorridorSprite(base, selection);
		auto image = UString::format(
		    "PCK:xcom3/UFODATA/BASE.PCK:xcom3/UFODATA/BASE.TAB:%d:xcom3/UFODATA/BASE.PCX", sprite);
		if (sprite != 0)
		{
			selText->SetText(tr("Corridor"));
		}
		else
		{
			selText->SetText(tr("Earth"));
		}
		selGraphic->SetImage(fw().data->load_image(image));
	}
}

void BaseScreen::Update(StageCmd *const cmd)
{
	form->Update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void BaseScreen::Render()
{
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	fw().renderer->drawFilledRect({0, 0}, fw().Display_GetSize(), Colour{0, 0, 0, 128});
	form->Render();
	RenderBase();

	// Highlight selected base
	for (auto &view : miniViews)
	{
		auto viewBase = view->GetData<Base>();
		if (base == viewBase)
		{
			Vec2<int> pos = form->Location + view->Location - 2;
			Vec2<int> size = view->Size + 4;
			fw().renderer->drawRect(pos, size, Colour{255, 0, 0});
			break;
		}
	}
}

bool BaseScreen::IsTransition() { return false; }

void BaseScreen::RenderBase()
{
	const Vec2<int> BASE_POS = form->Location + baseView->Location;

	BaseGraphics::renderBase(BASE_POS, base);

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
