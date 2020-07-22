#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/ui/general/loadingscreen.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/options.h"
#include "framework/renderer.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/ui/tileview/battleview.h"
#include "game/ui/tileview/cityview.h"
#include <algorithm>
#include <cmath>

namespace OpenApoc
{

LoadingScreen::LoadingScreen(sp<GameState> state, std::shared_future<void> task,
                             std::function<sp<Stage>()> nextScreenFn, sp<Image> background,
                             int scaleDivisor, bool showRotatingImage)
    : Stage(), loadingTask(std::move(task)), nextScreenFn(std::move(nextScreenFn)),
      backgroundimage(background), showRotatingImage(showRotatingImage), scaleDivisor(scaleDivisor),
      state(state)
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
	fw().displaySetIcon(std::dynamic_pointer_cast<RGBImage>(backgroundimage));
	loadingimageangle = 0;
	if (Options::asyncLoading.get() == false)
	{
		loadingTask.wait();
	}
}

void LoadingScreen::pause() {}

void LoadingScreen::resume() {}

void LoadingScreen::finish() {}

void LoadingScreen::eventOccurred(Event *e)
{
	if (!state)
	{
		return;
	}
	if (e->type() == EVENT_GAME_STATE)
	{
		auto gameEvent = dynamic_cast<GameEvent *>(e);
		if (!gameEvent)
		{
			LogError("Invalid game state event");
			return;
		}
		if (!gameEvent->message().empty())
		{
			state->logEvent(gameEvent);
		}
		switch (gameEvent->type)
		{
			default:
				break;
		}
	}
}

void LoadingScreen::update()
{
	loadingimageangle += (float)(M_PI + 0.05f);
	if (loadingimageangle >= (float)(M_PI * 2.0f))
		loadingimageangle -= (float)(M_PI * 2.0f);

	auto status = this->loadingTask.wait_for(std::chrono::seconds(0));
	if (Options::asyncLoading.get() == false)
	{
		LogAssert(status == std::future_status::ready);
	}
	switch (status)
	{
		case std::future_status::ready:
		{
			fw().stageQueueCommand({StageCmd::Command::REPLACE, nextScreenFn()});
			return;
		}
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
