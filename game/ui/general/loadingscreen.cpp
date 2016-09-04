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

void LoadingScreen::begin()
{
	// FIXME: This is now useless, as it doesn't actually load anything interesting here
	loadingimage = fw().data->loadImage("UI/LOADING.PNG");
	if (!backgroundimage)
	{
		backgroundimage = fw().data->loadImage("UI/LOGO.PNG");
	}
	fw().displaySetIcon();
	loadingimageangle = 0;
}

void LoadingScreen::pause() {}

void LoadingScreen::resume() {}

void LoadingScreen::finish() {}

void LoadingScreen::eventOccurred(Event *e) { std::ignore = e; }

sp<Stage> CreateUiForGame(sp<GameState> gameState)
{
	// FIXME load to correct screen based on loaded game state
	return mksp<CityView>(gameState);
}

void LoadingScreen::update()
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
				fw().stageQueueCommand({StageCmd::Command::REPLACEALL, mksp<CityView>(gameState)});
			}
			else
			{
				fw().stageQueueCommand({StageCmd::Command::POP});
			}
		}
			return;
		default:
			// Not yet finished
			return;
	}
}

void LoadingScreen::render()
{
	int logow = fw().displayGetWidth() / 3;
	float logosc = logow / static_cast<float>(backgroundimage->size.x);

	Vec2<float> logoPosition{fw().displayGetWidth() / 2 - (backgroundimage->size.x * logosc / 2),
	                         fw().displayGetHeight() / 2 - (backgroundimage->size.y * logosc / 2)};
	Vec2<float> logoSize{backgroundimage->size.x * logosc, backgroundimage->size.y * logosc};

	fw().renderer->drawScaled(backgroundimage, logoPosition, logoSize);

	fw().renderer->drawRotated(
	    loadingimage, Vec2<float>{24, 24},
	    Vec2<float>{fw().displayGetWidth() - 50, fw().displayGetHeight() - 50}, loadingimageangle);
}

bool LoadingScreen::isTransition() { return false; }

}; // namespace OpenApoc
