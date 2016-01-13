#include "library/sp.h"

#include "game/base/basescreen.h"
#include "game/base/base.h"
#include "game/base/facility.h"
#include "game/base/vequipscreen.h"
#include "framework/framework.h"
#include "framework/image.h"

namespace OpenApoc
{

const int BaseScreen::TILE_SIZE = 32;
const Vec2<int> BaseScreen::NO_SELECTION = {-1, -1};

// key is North South West East (true = occupied, false = vacant)
const std::unordered_map<std::vector<bool>, int> BaseScreen::TILE_CORRIDORS = {
    {{true, false, false, false}, 4}, {{false, false, false, true}, 5},
    {{true, false, false, true}, 6},  {{false, true, false, false}, 7},
    {{true, true, false, false}, 8},  {{false, true, false, true}, 9},
    {{true, true, false, true}, 10},  {{false, false, true, false}, 11},
    {{true, false, true, false}, 12}, {{false, false, true, true}, 13},
    {{true, false, true, true}, 14},  {{false, true, true, false}, 15},
    {{true, true, true, false}, 16},  {{false, true, true, true}, 17},
    {{true, true, true, true}, 18}};

int BaseScreen::getCorridorSprite(Vec2<int> pos) const
{
	if (pos.x < 0 || pos.y < 0 || pos.x >= Base::SIZE || pos.y >= Base::SIZE ||
	    !base.getCorridors()[pos.x][pos.y])
	{
		return 0;
	}
	bool north = pos.y > 0 && base.getCorridors()[pos.x][pos.y - 1];
	bool south = pos.y < Base::SIZE - 1 && base.getCorridors()[pos.x][pos.y + 1];
	bool west = pos.x > 0 && base.getCorridors()[pos.x - 1][pos.y];
	bool east = pos.x < Base::SIZE - 1 && base.getCorridors()[pos.x + 1][pos.y];
	return TILE_CORRIDORS.at({north, south, west, east});
}

BaseScreen::BaseScreen(sp<GameState> state)
    : Stage(), form(fw().gamecore->GetForm("FORM_BASESCREEN")), base(*state->playerBases.front()),
      selection(-1, -1), dragFacility(nullptr), drag(false), baseView(nullptr), selGraphic(nullptr),
      selText(nullptr), buildTime(nullptr), state(state)
{
}

BaseScreen::~BaseScreen() {}

void BaseScreen::Begin()
{
	form->FindControlTyped<Label>("TEXT_FUNDS")->SetText(state->getPlayerBalance());
	form->FindControlTyped<TextEdit>("TEXT_BASE_NAME")->SetText(base.name);

	buildTime = form->FindControlTyped<Label>("TEXT_BUILD_TIME");
	baseView = form->FindControlTyped<Graphic>("GRAPHIC_BASE_VIEW");
	selText = form->FindControlTyped<Label>("TEXT_SELECTED_FACILITY");
	selGraphic = form->FindControlTyped<Graphic>("GRAPHIC_SELECTED_FACILITY");
	for (int i = 0; i < 3; i++)
	{
		auto labelName = UString::format("LABEL_%d", i + 1);
		auto *label = form->FindControlTyped<Label>(labelName);
		if (!label)
		{
			LogError("Failed to find UI control matching \"%s\"", labelName.c_str());
		}
		statsLabels.push_back(label);

		auto valueName = UString::format("VALUE_%d", i + 1);
		auto *value = form->FindControlTyped<Label>(valueName);
		if (!value)
		{
			LogError("Failed to find UI control matching \"%s\"", valueName.c_str());
		}
		statsValues.push_back(value);
	}

	ListBox *facilities = form->FindControlTyped<ListBox>("LISTBOX_FACILITIES");
	for (auto &i : state->getRules().getFacilityDefs())
	{
		auto &facility = i.second;
		if (facility.fixed)
			continue;

		Graphic *graphic = new Graphic(nullptr, fw().data->load_image(facility.sprite));
		graphic->AutoSize = true;
		graphic->Data = const_cast<void *>(static_cast<const void *>(&facility));
		facilities->AddItem(graphic);
	}
}

void BaseScreen::Pause() {}

void BaseScreen::Resume() {}

void BaseScreen::Finish() {}

void BaseScreen::EventOccurred(Event *e)
{
	form->EventOccured(e);
	fw().gamecore->MouseCursor->EventOccured(e);

	if (e->Type == EVENT_KEY_DOWN)
	{
		if (e->Data.Keyboard.KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type == EVENT_MOUSE_MOVE)
	{
		mousePos = {e->Data.Mouse.X, e->Data.Mouse.Y};
	}

	if (e->Type == EVENT_FORM_INTERACTION)
	{
		if (e->Data.Forms.EventFlag == FormEventType::ButtonClick)
		{
			if (e->Data.Forms.RaisedBy->Name == "BUTTON_OK")
			{
				stageCmd.cmd = StageCmd::Command::POP;
				return;
			}
			else if (e->Data.Forms.RaisedBy->Name == "BUTTON_BASE_EQUIPVEHICLE")
			{
				// FIXME: If you don't have any vehicles this button should do nothing
				stageCmd.cmd = StageCmd::Command::PUSH;
				stageCmd.nextStage = std::make_shared<VEquipScreen>(state);
				return;
			}
		}

		if (e->Data.Forms.EventFlag == FormEventType::TextEditFinish)
		{
			if (e->Data.Forms.RaisedBy->Name == "TEXT_BASE_NAME")
			{
				TextEdit *name = form->FindControlTyped<TextEdit>("TEXT_BASE_NAME");
				base.name = name->GetText();
				return;
			}
		}

		if (e->Data.Forms.RaisedBy == baseView)
		{
			if (e->Data.Forms.EventFlag == FormEventType::MouseMove)
			{
				selection = {e->Data.Forms.MouseInfo.X, e->Data.Forms.MouseInfo.Y};
				selection /= TILE_SIZE;
				selFacility = base.getFacility(selection);
				return;
			}
			else if (e->Data.Forms.EventFlag == FormEventType::MouseLeave)
			{
				selection = NO_SELECTION;
				selFacility = nullptr;
				return;
			}
		}

		if (e->Data.Forms.EventFlag == FormEventType::ListBoxChangeHover)
		{
			if (e->Data.Forms.RaisedBy->Name == "LISTBOX_FACILITIES" && !drag)
			{
				ListBox *list = form->FindControlTyped<ListBox>("LISTBOX_FACILITIES");
				if (list->getHoveredData() == nullptr)
				{
					dragFacility = nullptr;
				}
				else
				{
					dragFacility = static_cast<FacilityDef *>(list->getHoveredData());
				}
				return;
			}
		}

		if (e->Data.Forms.EventFlag == FormEventType::MouseDown)
		{
			if (!drag && dragFacility != nullptr)
			{
				drag = true;
			}
		}

		if (e->Data.Forms.EventFlag == FormEventType::MouseUp)
		{
			if (drag && dragFacility != nullptr)
			{
				base.buildFacility(*dragFacility, selection);
				form->FindControlTyped<Label>("TEXT_FUNDS")->SetText(state->getPlayerBalance());
				// FIXME: Report build errors
				drag = false;
				dragFacility = nullptr;
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
	if (dragFacility != nullptr)
	{
		selText->SetText(fw().gamecore->GetString(dragFacility->name));
		selGraphic->SetImage(fw().data->load_image(dragFacility->sprite));
		statsLabels[0]->SetText("Cost to build");
		statsValues[0]->SetText("$" + Strings::FromInteger(dragFacility->buildCost));
		statsLabels[1]->SetText("Days to build");
		statsValues[1]->SetText(Strings::FromInteger(dragFacility->buildTime));
		statsLabels[2]->SetText("Maintenance cost");
		statsValues[2]->SetText("$" + Strings::FromInteger(dragFacility->weeklyCost));
	}
	else if (selFacility != nullptr)
	{
		selText->SetText(fw().gamecore->GetString(selFacility->def.name));
		selGraphic->SetImage(fw().data->load_image(selFacility->def.sprite));
		if (selFacility->def.capacityAmount > 0)
		{
			statsLabels[0]->SetText("Capacity");
			statsValues[0]->SetText(Strings::FromInteger(selFacility->def.capacityAmount));
			statsLabels[1]->SetText("Usage");
			statsValues[1]->SetText("0%");
		}
	}
	else if (selection != NO_SELECTION)
	{
		int sprite = getCorridorSprite(selection);
		auto image = UString::format(
		    "PCK:xcom3/UFODATA/BASE.PCK:xcom3/UFODATA/BASE.TAB:%d:xcom3/UFODATA/BASE.PCX", sprite);
		if (sprite != 0)
		{
			selText->SetText("Corridor");
		}
		else
		{
			selText->SetText("Earth");
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
	fw().gamecore->MouseCursor->Render();
}

bool BaseScreen::IsTransition() { return false; }

void BaseScreen::RenderBase()
{
	const Vec2<int> BASE_POS = form->Location + baseView->Location;

	// Draw grid
	sp<Image> grid = fw().data->load_image(
	    "PCK:xcom3/UFODATA/BASE.PCK:xcom3/UFODATA/BASE.TAB:0:xcom3/UFODATA/BASE.PCX");
	Vec2<int> i;
	for (i.x = 0; i.x < Base::SIZE; ++i.x)
	{
		for (i.y = 0; i.y < Base::SIZE; ++i.y)
		{
			Vec2<int> pos = BASE_POS + i * TILE_SIZE;
			fw().renderer->draw(grid, pos);
		}
	}

	// Draw corridors
	for (i.x = 0; i.x < Base::SIZE; ++i.x)
	{
		for (i.y = 0; i.y < Base::SIZE; ++i.y)
		{
			int sprite = getCorridorSprite(i);
			if (sprite != 0)
			{
				Vec2<int> pos = BASE_POS + i * TILE_SIZE;
				auto image = UString::format(
				    "PCK:xcom3/UFODATA/BASE.PCK:xcom3/UFODATA/BASE.TAB:%d:xcom3/UFODATA/BASE.PCX",
				    sprite);
				fw().renderer->draw(fw().data->load_image(image), pos);
			}
		}
	}

	// Draw facilities
	sp<Image> circleS = fw().data->load_image(
	    "PCK:xcom3/UFODATA/BASE.PCK:xcom3/UFODATA/BASE.TAB:25:xcom3/UFODATA/BASE.PCX");
	sp<Image> circleL = fw().data->load_image(
	    "PCK:xcom3/UFODATA/BASE.PCK:xcom3/UFODATA/BASE.TAB:26:xcom3/UFODATA/BASE.PCX");
	buildTime->Visible = true;
	for (auto &facility : base.getFacilities())
	{
		sp<Image> sprite = fw().data->load_image(facility->def.sprite);
		Vec2<int> pos = BASE_POS + facility->pos * TILE_SIZE;
		if (facility->buildTime == 0)
		{
			fw().renderer->draw(sprite, pos);
		}
		else
		{
			// Fade out facility
			fw().renderer->drawTinted(sprite, pos, Colour(255, 255, 255, 128));
			// Draw construction overlay
			if (facility->def.size == 1)
			{
				fw().renderer->draw(circleS, pos);
			}
			else
			{
				fw().renderer->draw(circleL, pos);
			}
			// Draw time remaining
			buildTime->Size = {TILE_SIZE, TILE_SIZE};
			buildTime->Size *= facility->def.size;
			buildTime->Location = pos;
			buildTime->SetText(Strings::FromInteger(facility->buildTime));
			buildTime->Render();
		}
	}
	buildTime->Visible = false;

	// Draw doors
	sp<Image> doorLeft = fw().data->load_image(
	    "PCK:xcom3/UFODATA/BASE.PCK:xcom3/UFODATA/BASE.TAB:2:xcom3/UFODATA/BASE.PCX");
	sp<Image> doorBottom = fw().data->load_image(
	    "PCK:xcom3/UFODATA/BASE.PCK:xcom3/UFODATA/BASE.TAB:3:xcom3/UFODATA/BASE.PCX");
	for (auto &facility : base.getFacilities())
	{
		for (int y = 0; y < facility->def.size; ++y)
		{
			Vec2<int> tile = facility->pos + Vec2<int>{-1, y};
			if (getCorridorSprite(tile) != 0)
			{
				Vec2<int> pos = BASE_POS + tile * TILE_SIZE;
				fw().renderer->draw(doorLeft, pos + Vec2<int>{TILE_SIZE / 2, 0});
			}
		}
		for (int x = 0; x < facility->def.size; ++x)
		{
			Vec2<int> tile = facility->pos + Vec2<int>{x, facility->def.size};
			if (getCorridorSprite(tile) != 0)
			{
				Vec2<int> pos = BASE_POS + tile * TILE_SIZE;
				fw().renderer->draw(doorBottom, pos - Vec2<int>{0, TILE_SIZE / 2});
			}
		}
	}

	// Draw selection
	if (selection != NO_SELECTION)
	{
		Vec2<int> pos = selection;
		Vec2<int> size = {TILE_SIZE, TILE_SIZE};
		if (drag && dragFacility != nullptr)
		{
			size *= dragFacility->size;
		}
		else if (selFacility != nullptr)
		{
			pos = selFacility->pos;
			size *= selFacility->def.size;
		}
		pos = BASE_POS + pos * TILE_SIZE;
		fw().renderer->drawRect(pos, size, Colour{255, 255, 255});
	}

	// Draw dragged facility
	if (drag && dragFacility != nullptr)
	{
		sp<Image> facility = fw().data->load_image(dragFacility->sprite);
		Vec2<int> pos;
		if (selection == NO_SELECTION)
		{
			pos = mousePos - Vec2<int>{TILE_SIZE / 2, TILE_SIZE / 2} * dragFacility->size;
		}
		else
		{
			pos = BASE_POS + selection * TILE_SIZE;
		}
		fw().renderer->draw(facility, pos);
	}
}
}; // namespace OpenApoc
