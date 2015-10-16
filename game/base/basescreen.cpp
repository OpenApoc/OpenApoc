
#include "game/base/basescreen.h"
#include "game/base/base.h"
#include "framework/framework.h"
#include "framework/image.h"

#include <sstream>

namespace OpenApoc
{

// key is North South West East (true = occupied, false = vacant)
const std::unordered_map<std::vector<bool>, int> BaseScreen::TILE_CORRIDORS = {
	{{true, false, false, false}, 4},
	{{false, false, false, true}, 5 },
	{{true, false, false, true}, 6 },
	{{false, true, false, false}, 7},
	{{true, true, false, false}, 8},
	{{false, true, false, true}, 9},
	{{true, true, false, true}, 10},
	{{false, false, true, false}, 11},
	{{true, false, true, false}, 12},
	{{false, false, true, true}, 13},
	{{true, false, true, true}, 14},
	{{false, true, true, false}, 15},
	{{true, true, true, false}, 16},
	{{false, true, true, true}, 17},
	{{true, true, true, true}, 18}
};

int BaseScreen::getCorridorSprite(Vec2<int> pos) const
{
	if (!base.getCorridors()[pos.x][pos.y])
	{
		return 0;
	}
	bool north = pos.y > 0 && base.getCorridors()[pos.x][pos.y-1];
	bool south = pos.y < Base::SIZE-1 && base.getCorridors()[pos.x][pos.y+1];
	bool west = pos.x > 0 && base.getCorridors()[pos.x-1][pos.y];
	bool east = pos.x < Base::SIZE-1 && base.getCorridors()[pos.x+1][pos.y];
	return TILE_CORRIDORS.at({north, south, west, east});
}

BaseScreen::BaseScreen(Framework &fw)
    : Stage(fw), basescreenform(fw.gamecore->GetForm("FORM_BASESCREEN")),
      base(*fw.state->playerBases.front())
{
}

BaseScreen::~BaseScreen() {}

void BaseScreen::Begin()
{
	Label *funds = basescreenform->FindControlTyped<Label>("TEXT_FUNDS");
	funds->SetText(fw.state->getPlayerBalance());

	Label *name = basescreenform->FindControlTyped<Label>("TEXT_BASE_NAME");
	name->SetText(base.name);
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
		if (e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
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
	const int TILE_SIZE = 32;
	const Vec2<int> BASE_POS = {202, 84};

	// Draw grid
	std::shared_ptr<Image> grid = fw.data->load_image(
	    "PCK:xcom3/UFODATA/BASE.PCK:xcom3/UFODATA/BASE.TAB:0:xcom3/TACDATA/TACTICAL.PAL");
	Vec2<int> i;
	for (i.x = 0; i.x < Base::SIZE; ++i.x)
	{
		for (i.y = 0; i.y < Base::SIZE; ++i.y)
		{
			Vec2<int> pos = BASE_POS + i * TILE_SIZE;
			fw.renderer->draw(grid, basescreenform->Location + pos);
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
				ss << "PCK:xcom3/UFODATA/BASE.PCK:xcom3/UFODATA/BASE.TAB:"
					<< sprite
					<< ":xcom3/TACDATA/TACTICAL.PAL";
				fw.renderer->draw(fw.data->load_image(ss.str()), basescreenform->Location + pos);
			}
		}
	}

	// Draw facilities
	for (auto &facility : base.getFacilities())
	{
		std::shared_ptr<Image> sprite = fw.data->load_image(facility.def.sprite);
		Vec2<int> pos = BASE_POS + facility.pos * TILE_SIZE;
		fw.renderer->draw(sprite, basescreenform->Location + pos);
	}
}
}; // namespace OpenApoc
