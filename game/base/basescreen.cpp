#include "library/sp.h"

#include "game/base/basescreen.h"
#include "game/base/base.h"
#include "game/base/facility.h"
#include "framework/framework.h"
#include "framework/image.h"

#include <sstream>

namespace OpenApoc
{

const int BaseScreen::TILE_SIZE = 32;

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
	if (!base.getCorridors()[pos.x][pos.y])
	{
		return 0;
	}
	bool north = pos.y > 0 && base.getCorridors()[pos.x][pos.y - 1];
	bool south = pos.y < Base::SIZE - 1 && base.getCorridors()[pos.x][pos.y + 1];
	bool west = pos.x > 0 && base.getCorridors()[pos.x - 1][pos.y];
	bool east = pos.x < Base::SIZE - 1 && base.getCorridors()[pos.x + 1][pos.y];
	return TILE_CORRIDORS.at({north, south, west, east});
}

BaseScreen::BaseScreen(Framework &fw)
    : Stage(fw), basescreenform(fw.gamecore->GetForm("FORM_BASESCREEN")),
      base(*fw.state->playerBases.front()), selection(-1, -1)
{
}

BaseScreen::~BaseScreen() {}

void BaseScreen::Begin()
{
	Label *funds = basescreenform->FindControlTyped<Label>("TEXT_FUNDS");
	funds->SetText(fw.state->getPlayerBalance());

	TextEdit *name = basescreenform->FindControlTyped<TextEdit>("TEXT_BASE_NAME");
	name->SetText(base.name);

	baseView = basescreenform->FindControlTyped<Graphic>("GRAPHIC_BASE_VIEW");
	selText = basescreenform->FindControlTyped<Label>("TEXT_SELECTED_FACILITY");
	selGraphic = basescreenform->FindControlTyped<Graphic>("GRAPHIC_SELECTED_FACILITY");
}

void BaseScreen::Pause() {}

void BaseScreen::Resume() {}

void BaseScreen::Finish() {}

void BaseScreen::EventOccurred(Event *e)
{
	basescreenform->EventOccured(e);
	fw.gamecore->MouseCursor->EventOccured(e);

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
		selection = {e->Data.Mouse.X, e->Data.Mouse.Y};
		selection -= (basescreenform->Location + baseView->Location);
		if (selection.x >= 0 && selection.y >= 0)
		{
			selection /= TILE_SIZE;
		}
		if (selection.x >= 0 && selection.y >= 0 && selection.x < Base::SIZE &&
		    selection.y < Base::SIZE)
		{
			selFacility = base.getFacility(selection);
			if (selFacility != nullptr)
			{
				selText->SetText(fw.gamecore->GetString(selFacility->def.name));
				selGraphic->SetImage(fw.data->load_image(selFacility->def.sprite));
			}
			else
			{
				int sprite = getCorridorSprite(selection);
				if (sprite != 0)
				{
					std::ostringstream ss;
					ss << "PCK:xcom3/UFODATA/BASE.PCK:xcom3/UFODATA/BASE.TAB:" << sprite
					   << ":UI/menuopt.pal";
					selText->SetText("Corridor");
					selGraphic->SetImage(fw.data->load_image(ss.str()));
				}
				else
				{
					selText->SetText("Earth");
					selGraphic->SetImage(fw.data->load_image(
					    "PCK:xcom3/UFODATA/BASE.PCK:xcom3/UFODATA/BASE.TAB:0:UI/menuopt.pal"));
				}
			}
		}
		else
		{
			selText->SetText("");
			selGraphic->SetImage(nullptr);
		}
	}

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick)
	{
		if (e->Data.Forms.RaisedBy->Name == "BUTTON_OK")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type == EVENT_FORM_INTERACTION &&
	    e->Data.Forms.EventFlag == FormEventType::TextEditFinish)
	{
		if (e->Data.Forms.RaisedBy->Name == "TEXT_BASE_NAME")
		{
			TextEdit *name = basescreenform->FindControlTyped<TextEdit>("TEXT_BASE_NAME");
			base.name = name->GetText();
			return;
		}
	}
}

void BaseScreen::Update(StageCmd *const cmd)
{
	basescreenform->Update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void BaseScreen::Render()
{
	fw.Stage_GetPrevious(this->shared_from_this())->Render();
	fw.renderer->drawFilledRect({0, 0}, fw.Display_GetSize(), Colour{0, 0, 0, 128});
	basescreenform->Render();
	RenderBase();
	fw.gamecore->MouseCursor->Render();
}

bool BaseScreen::IsTransition() { return false; }

void BaseScreen::RenderBase()
{
	const Vec2<int> BASE_POS = basescreenform->Location + baseView->Location;

	// Draw grid
	sp<Image> grid =
	    fw.data->load_image("PCK:xcom3/UFODATA/BASE.PCK:xcom3/UFODATA/BASE.TAB:0:UI/menuopt.pal");
	Vec2<int> i;
	for (i.x = 0; i.x < Base::SIZE; ++i.x)
	{
		for (i.y = 0; i.y < Base::SIZE; ++i.y)
		{
			Vec2<int> pos = BASE_POS + i * TILE_SIZE;
			fw.renderer->draw(grid, pos);
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
				std::ostringstream ss;
				ss << "PCK:xcom3/UFODATA/BASE.PCK:xcom3/UFODATA/BASE.TAB:" << sprite
				   << ":UI/menuopt.pal";
				fw.renderer->draw(fw.data->load_image(ss.str()), pos);
			}
		}
	}

	// Draw facilities
	for (auto &facility : base.getFacilities())
	{
		sp<Image> sprite = fw.data->load_image(facility->def.sprite);
		Vec2<int> pos = BASE_POS + facility->pos * TILE_SIZE;
		fw.renderer->draw(sprite, pos);
	}

	// Draw selection
	if (selection.x >= 0 && selection.y >= 0 && selection.x < Base::SIZE &&
	    selection.y < Base::SIZE)
	{
		Vec2<int> pos = selection;
		Vec2<int> size = {TILE_SIZE, TILE_SIZE};
		if (selFacility != nullptr)
		{
			pos = selFacility->pos;
			size *= selFacility->def.size;
		}
		pos = BASE_POS + pos * TILE_SIZE;
		fw.renderer->drawRect(pos, size, Colour{255, 255, 255});
	}
}
}; // namespace OpenApoc
