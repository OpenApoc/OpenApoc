#include "library/sp.h"

#include "framework/framework.h"
#include "framework/image.h"
#include "game/base/base.h"
#include "game/base/basescreen.h"
#include "game/base/facility.h"
#include "game/base/researchscreen.h"
#include "game/base/vequipscreen.h"
#include "game/resources/gamecore.h"
#include <game/general/messagebox.h>

namespace OpenApoc
{

const int BaseScreen::TILE_SIZE = 32;
const int BaseScreen::MINI_SIZE = 4;
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
	    !base->getCorridors()[pos.x][pos.y])
	{
		return 0;
	}
	bool north = pos.y > 0 && base->getCorridors()[pos.x][pos.y - 1];
	bool south = pos.y < Base::SIZE - 1 && base->getCorridors()[pos.x][pos.y + 1];
	bool west = pos.x > 0 && base->getCorridors()[pos.x - 1][pos.y];
	bool east = pos.x < Base::SIZE - 1 && base->getCorridors()[pos.x + 1][pos.y];
	return TILE_CORRIDORS.at({north, south, west, east});
}

BaseScreen::BaseScreen(sp<GameState> state)
    : Stage(), form(fw().gamecore->GetForm("FORM_BASESCREEN")),
      base({state.get(), state->player_bases.begin()->first}), selection(-1, -1),
      dragFacility(nullptr), drag(false), baseView(nullptr), selGraphic(nullptr), selText(nullptr),
      buildTime(nullptr), state(state)
{
}

BaseScreen::~BaseScreen() {}

void BaseScreen::Begin()
{
	form->FindControlTyped<Label>("TEXT_FUNDS")->SetText(state->getPlayerBalance());
	form->FindControlTyped<TextEdit>("TEXT_BASE_NAME")->SetText(base->name);

	buildTime = form->FindControlTyped<Label>("TEXT_BUILD_TIME");
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
	for (int i = 0; i < 8; i++)
	{
		auto viewName = UString::format("BUTTON_BASE_%d", i + 1);
		auto view = form->FindControlTyped<GraphicButton>(viewName);
		if (!view)
		{
			LogError("Failed to find UI control matching \"%s\"", viewName.c_str());
		}
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

	{
		this->minimap_image = mksp<RGBImage>(Vec2<unsigned int>{100, 100});
		RGBImageLock l(this->minimap_image);
		std::map<Vec2<int>, int> minimap_image_z;
		for (auto &pair : state->cities["CITYMAP_HUMAN"]->initial_tiles)
		{
			auto &pos = pair.first;
			Vec2<int> pos2d = {pos.x, pos.y};
			auto &tile = pair.second;
			auto it = minimap_image_z.find(pos2d);
			if (it == minimap_image_z.end() || it->second < pos.z)
			{
				if (tile->minimap_colour.a == 0)
					continue;
				minimap_image_z[pos2d] = pos.z;
				l.set(pos2d, tile->minimap_colour);
			}
		}

		// Set the bounds of the current base to be a red block
		for (int y = this->base->building->bounds.p0.y; y < this->base->building->bounds.p1.y; y++)
		{
			for (int x = this->base->building->bounds.p0.x; x < this->base->building->bounds.p1.x;
			     x++)
			{
				l.set({x, y}, {255, 0, 0, 255});
			}
		}
	}

	this->form->FindControlTyped<Graphic>("MINIMAP")->SetImage(this->minimap_image);
}

void BaseScreen::Pause() {}

void BaseScreen::Resume() {}

void BaseScreen::Finish() {}

void BaseScreen::EventOccurred(Event *e)
{
	form->EventOccured(e);
	fw().gamecore->MouseCursor->EventOccured(e);

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
		if (e->Forms().EventFlag == FormEventType::ButtonClick)
		{
			if (e->Forms().RaisedBy->Name == "BUTTON_OK")
			{
				stageCmd.cmd = StageCmd::Command::POP;
				return;
			}
			else if (e->Forms().RaisedBy->Name == "BUTTON_BASE_EQUIPVEHICLE")
			{
				// FIXME: If you don't have any vehicles this button should do nothing
				stageCmd.cmd = StageCmd::Command::PUSH;
				stageCmd.nextStage = mksp<VEquipScreen>(state);
				return;
			}
			else if (e->Forms().RaisedBy->Name == "BUTTON_BASE_RES_AND_MANUF")
			{
				// FIXME: If you don't have any vehicles this button should do nothing
				stageCmd.cmd = StageCmd::Command::PUSH;
				stageCmd.nextStage = mksp<ResearchScreen>(state, this->base);
				return;
			}
			// test
			else if (e->Forms().RaisedBy->Name == "BUTTON_BASE_HIREFIRESTAFF")
			{
				stageCmd.cmd = StageCmd::Command::PUSH;
				stageCmd.nextStage =
					mksp<MessageBox>("Game Over", "Quit?", MessageBox::ButtonOptions::YesNo);
				return;
			}
		}

		if (e->Forms().EventFlag == FormEventType::TextEditFinish)
		{
			if (e->Forms().RaisedBy->Name == "TEXT_BASE_NAME")
			{
				auto name = form->FindControlTyped<TextEdit>("TEXT_BASE_NAME");
				base->name = name->GetText();
				return;
			}
		}

		if (e->Forms().RaisedBy == baseView)
		{
			if (e->Forms().EventFlag == FormEventType::MouseMove)
			{
				selection = {e->Forms().MouseInfo.X, e->Forms().MouseInfo.Y};
				selection /= TILE_SIZE;
				selFacility = base->getFacility(selection);
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
			if (drag && dragFacility)
			{
				base->buildFacility(dragFacility, selection);
				form->FindControlTyped<Label>("TEXT_FUNDS")->SetText(state->getPlayerBalance());
				// FIXME: Report build errors
				drag = false;
				dragFacility = "";
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
		int sprite = getCorridorSprite(selection);
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
	RenderMiniBase();
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
	for (i.x = 0; i.x < Base::SIZE; i.x++)
	{
		for (i.y = 0; i.y < Base::SIZE; i.y++)
		{
			Vec2<int> pos = BASE_POS + i * TILE_SIZE;
			fw().renderer->draw(grid, pos);
		}
	}

	// Draw corridors
	for (i.x = 0; i.x < Base::SIZE; i.x++)
	{
		for (i.y = 0; i.y < Base::SIZE; i.y++)
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
	for (auto &facility : base->getFacilities())
	{
		sp<Image> sprite = facility->type->sprite;
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
			if (facility->type->size == 1)
			{
				fw().renderer->draw(circleS, pos);
			}
			else
			{
				fw().renderer->draw(circleL, pos);
			}
			// Draw time remaining
			buildTime->Size = {TILE_SIZE, TILE_SIZE};
			buildTime->Size *= facility->type->size;
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
	for (auto &facility : base->getFacilities())
	{
		for (int y = 0; y < facility->type->size; y++)
		{
			Vec2<int> tile = facility->pos + Vec2<int>{-1, y};
			if (getCorridorSprite(tile) != 0)
			{
				Vec2<int> pos = BASE_POS + tile * TILE_SIZE;
				fw().renderer->draw(doorLeft, pos + Vec2<int>{TILE_SIZE / 2, 0});
			}
		}
		for (int x = 0; x < facility->type->size; x++)
		{
			Vec2<int> tile = facility->pos + Vec2<int>{x, facility->type->size};
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
		if (drag && dragFacility)
		{
			size *= dragFacility->size;
		}
		else if (selFacility != nullptr)
		{
			pos = selFacility->pos;
			size *= selFacility->type->size;
		}
		pos = BASE_POS + pos * TILE_SIZE;
		fw().renderer->drawRect(pos, size, Colour{255, 255, 255});
	}

	// Draw dragged facility
	if (drag && dragFacility)
	{
		sp<Image> facility = dragFacility->sprite;
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

void BaseScreen::RenderMiniBase()
{
	const Vec2<int> BASE_POS = form->Location + miniViews[0]->Location;

	// Draw corridors
	Vec2<int> i;
	for (i.x = 0; i.x < Base::SIZE; i.x++)
	{
		for (i.y = 0; i.y < Base::SIZE; i.y++)
		{
			int sprite = getCorridorSprite(i);
			if (sprite != 0)
			{
				sprite -= 3;
			}
			Vec2<int> pos = BASE_POS + i * MINI_SIZE;
			auto image = UString::format(
			    "RAW:xcom3/UFODATA/MINIBASE.DAT:4:4:%d:xcom3/UFODATA/BASE.PCX", sprite);
			fw().renderer->draw(fw().data->load_image(image), pos);
		}
	}

	// Draw facilities
	sp<Image> normal =
	    fw().data->load_image("RAW:xcom3/UFODATA/MINIBASE.DAT:4:4:16:xcom3/UFODATA/BASE.PCX");
	sp<Image> highlighted =
	    fw().data->load_image("RAW:xcom3/UFODATA/MINIBASE.DAT:4:4:17:xcom3/UFODATA/BASE.PCX");
	sp<Image> selected =
	    fw().data->load_image("RAW:xcom3/UFODATA/MINIBASE.DAT:4:4:18:xcom3/UFODATA/BASE.PCX");
	for (auto &facility : base->getFacilities())
	{
		sp<Image> sprite = (facility->buildTime == 0) ? normal : highlighted;
		for (i.x = 0; i.x < facility->type->size; i.x++)
		{
			for (i.y = 0; i.y < facility->type->size; i.y++)
			{
				Vec2<int> pos = BASE_POS + (facility->pos + i) * MINI_SIZE;
				fw().renderer->draw(sprite, pos);
			}
		}
	}

	// Draw selection
	{
		Vec2<int> pos = BASE_POS - 2;
		Vec2<int> size = miniViews[0]->Size + 4;
		fw().renderer->drawRect(pos, size, Colour{255, 0, 0});
	}
}

}; // namespace OpenApoc
