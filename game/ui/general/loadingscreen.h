#pragma once
#include "framework/includes.h"
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

  public:
	LoadingScreen(std::future<sp<GameState>> gameStateTask, sp<Image> background = nullptr);
	// Stage control
	void begin() override;
	void pause() override;
	void resume() override;
	void finish() override;
	void eventOccurred(Event *e) override;
	void update(StageCmd *const cmd) override;
	void render() override;
	bool isTransition() override;
};

}; // namespace OpenApoc
