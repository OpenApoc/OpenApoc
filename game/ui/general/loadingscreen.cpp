#define _USE_MATH_DEFINES
#include "game/ui/general/loadingscreen.h"
#include "framework/framework.h"
#include "game/ui/city/cityview.h"
#include <cmath>

namespace OpenApoc
{

LoadingScreen::LoadingScreen(std::future<sp<GameState>> gameStateTask, sp<Image> background)
    : Stage(), loading_task(std::move(gameStateTask)), backgroundimage(background)
{
}

void LoadingScreen::Begin()
{
	// FIXME: This is now useless, as it doesn't actually load anything interesting here
	loadingimage = fw().data->load_image("UI/LOADING.PNG");
	if (!backgroundimage)
	{
		backgroundimage = fw().data->load_image("UI/LOGO.PNG");
	}
	fw().Display_SetIcon();
	loadingimageangle = 0;
}

void LoadingScreen::Pause() {}

void LoadingScreen::Resume() {}

void LoadingScreen::Finish() {}

void LoadingScreen::EventOccurred(Event *e) { std::ignore = e; }

sp<Stage> CreateUiForGame(sp<GameState> gameState)
{
	// FIXME load to correct screen based on loaded game state
	return mksp<CityView>(gameState);
}

void LoadingScreen::Update(StageCmd *const cmd)
{
	loadingimageangle += (float)(M_PI + 0.05f);
	if (loadingimageangle >= (float)(M_PI * 2.0f))
		loadingimageangle -= (float)(M_PI * 2.0f);

	auto status = this->loading_task.wait_for(std::chrono::seconds(0));
	switch (status)
	{
		case std::future_status::ready:
		{
			auto gameState = loading_task.get();
			if (gameState != nullptr)
			{
				cmd->cmd = StageCmd::Command::REPLACE;
				cmd->nextStage = mksp<CityView>(gameState);
			}
			else
			{
				cmd->cmd = StageCmd::Command::POP;
			}
		}
			return;
		default:
			// Not yet finished
			return;
	}
}

void LoadingScreen::Render()
{
	int logow = fw().Display_GetWidth() / 3;
	float logosc = logow / static_cast<float>(backgroundimage->size.x);

	Vec2<float> logoPosition{fw().Display_GetWidth() / 2 - (backgroundimage->size.x * logosc / 2),
	                         fw().Display_GetHeight() / 2 - (backgroundimage->size.y * logosc / 2)};
	Vec2<float> logoSize{backgroundimage->size.x * logosc, backgroundimage->size.y * logosc};

	fw().renderer->drawScaled(backgroundimage, logoPosition, logoSize);

	fw().renderer->drawRotated(
	    loadingimage, Vec2<float>{24, 24},
	    Vec2<float>{fw().Display_GetWidth() - 50, fw().Display_GetHeight() - 50},
	    loadingimageangle);
}

bool LoadingScreen::IsTransition() { return false; }

}; // namespace OpenApoc
