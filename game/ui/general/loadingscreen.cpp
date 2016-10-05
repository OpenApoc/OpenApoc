#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/ui/general/loadingscreen.h"
#include "framework/framework.h"
#include "game/ui/battle/battleview.h"
#include "game/ui/city/cityview.h"
#include <algorithm>
#include <cmath>

namespace OpenApoc
{

LoadingScreen::LoadingScreen(std::future<sp<GameState>> gameStateTask, sp<Image> background,
                             int scaleDivisor, bool showRotatingImage)
    : Stage(), loading_task(std::move(gameStateTask)), backgroundimage(background),
      showRotatingImage(showRotatingImage), scaleDivisor(scaleDivisor)
{
}

void LoadingScreen::begin()
{
	// FIXME: This is now useless, as it doesn't actually load anything interesting here
	if (showRotatingImage)
	{
		loadingimage = fw().data->loadImage("ui/loading.png");
	}
	if (!backgroundimage)
	{
		backgroundimage = fw().data->loadImage("ui/logo.png");
	}
	fw().displaySetIcon();
	loadingimageangle = 0;
}

void LoadingScreen::pause() {}

void LoadingScreen::resume() {}

void LoadingScreen::finish() {}

void LoadingScreen::eventOccurred(Event *e) { std::ignore = e; }

sp<Stage> LoadingScreen::createUiForGame(sp<GameState> gameState) const
{
	// FIXME load to correct screen based on loaded game state
	if (gameState->current_battle)
		return mksp<BattleView>(gameState);
	else
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
				fw().stageQueueCommand({StageCmd::Command::REPLACEALL, createUiForGame(gameState)});
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
	int logow = fw().displayGetWidth() / scaleDivisor;
	int logoh = fw().displayGetHeight() / scaleDivisor;
	float logoscw = logow / static_cast<float>(backgroundimage->size.x);
	float logosch = logoh / static_cast<float>(backgroundimage->size.y);
	float logosc = std::min(logoscw, logosch);

	Vec2<float> logoPosition{fw().displayGetWidth() / 2 - (backgroundimage->size.x * logosc / 2),
	                         fw().displayGetHeight() / 2 - (backgroundimage->size.y * logosc / 2)};
	Vec2<float> logoSize{backgroundimage->size.x * logosc, backgroundimage->size.y * logosc};

	fw().renderer->drawScaled(backgroundimage, logoPosition, logoSize);
	if (loadingimage)
	{
		fw().renderer->drawRotated(
		    loadingimage, Vec2<float>{24, 24},
		    Vec2<float>{fw().displayGetWidth() - 50, fw().displayGetHeight() - 50},
		    loadingimageangle);
	}
}

bool LoadingScreen::isTransition() { return false; }

}; // namespace OpenApoc
