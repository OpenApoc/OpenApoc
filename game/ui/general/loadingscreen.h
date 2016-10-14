#pragma once

#include "framework/stage.h"
#include "library/sp.h"
#include <future>

namespace OpenApoc
{

class Image;
class GameState;

class LoadingScreen : public Stage
{
  private:
	std::future<sp<GameState>> loading_task;
	sp<Image> loadingimage;
	sp<Image> backgroundimage;
	float loadingimageangle;
	bool showRotatingImage = false;
	int scaleDivisor = 0;

  public:
	LoadingScreen(std::future<sp<GameState>> gameStateTask, sp<Image> background = nullptr,
	              int scaleDivisor = 3, bool showRotatingImage = true);
	// can override this in a screen that would load into something else
	virtual sp<Stage> createUiForGame(sp<GameState> gameState) const;
	// Stage control
	void begin() override;
	void pause() override;
	void resume() override;
	void finish() override;
	void eventOccurred(Event *e) override;
	void update() override;
	void render() override;
	bool isTransition() override;
};

}; // namespace OpenApoc
