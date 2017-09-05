#pragma once

#include "framework/stage.h"
#include "library/sp.h"
#include <functional>
#include <future>

namespace OpenApoc
{

class Image;
class GameState;

class LoadingScreen : public Stage
{
  private:
	std::shared_future<void> loadingTask;
	std::function<sp<Stage>()> nextScreenFn;
	sp<Image> loadingimage;
	sp<Image> backgroundimage;
	float loadingimageangle;
	bool showRotatingImage = false;
	int scaleDivisor = 0;
	sp<GameState> state;

  public:
	LoadingScreen(sp<GameState> state, std::shared_future<void> task,
	              std::function<sp<Stage>()> nextScreenFn, sp<Image> background = nullptr,
	              int scaleDivisor = 3, bool showRotatingImage = true);
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
