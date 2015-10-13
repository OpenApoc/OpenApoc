
#include "game/base/basescreen.h"
#include "game/base/base.h"
#include "framework/framework.h"
#include "framework/image.h"

namespace OpenApoc
{

// key is 0bUDLR = Up Down Left Right (eg. 0b1111 is surrounded)
const std::unordered_map<std::bitset<4>, int> BaseScreen::TILE_CORRIDORS = {
    {std::bitset<4>{"1000"}, 4},  {std::bitset<4>{"0001"}, 5},  {std::bitset<4>{"1001"}, 6},
    {std::bitset<4>{"0100"}, 7},  {std::bitset<4>{"1100"}, 8},  {std::bitset<4>{"0101"}, 9},
    {std::bitset<4>{"1101"}, 10}, {std::bitset<4>{"0010"}, 11}, {std::bitset<4>{"1010"}, 12},
    {std::bitset<4>{"0011"}, 13}, {std::bitset<4>{"1011"}, 14}, {std::bitset<4>{"0110"}, 15},
    {std::bitset<4>{"1110"}, 16}, {std::bitset<4>{"0111"}, 17}, {std::bitset<4>{"1111"}, 18},
};

BaseScreen::BaseScreen(Framework &fw)
    : Stage(fw), basescreenform(fw.gamecore->GetForm("FORM_BASESCREEN")),
      base(fw.state->bases.front())
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

	std::shared_ptr<Image> grid = fw.data->load_image(
	    "PCK:xcom3/UFODATA/BASE.PCK:xcom3/UFODATA/BASE.TAB:0:xcom3/TACDATA/TACTICAL.PAL");
	// Draw grid
	Vec2<int> pos;
	for (pos.x = BASE_POS.x; pos.x < BASE_POS.x + TILE_SIZE * Base::SIZE; pos.x += TILE_SIZE)
	{
		for (pos.y = BASE_POS.y; pos.y < BASE_POS.y + TILE_SIZE * Base::SIZE; pos.y += TILE_SIZE)
		{
			fw.renderer->draw(grid, basescreenform->Location + pos);
		}
	}

	// TODO: Draw corridors

	// Draw facilities
	for (auto &facility : base.getFacilities())
	{
		std::shared_ptr<Image> sprite = fw.data->load_image(facility.def.sprite);
		Vec2<int> pos = BASE_POS + facility.pos * TILE_SIZE;
		fw.renderer->draw(sprite, basescreenform->Location + pos);
	}
}

}; // namespace OpenApoc
