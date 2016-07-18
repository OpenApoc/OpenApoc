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
	void Begin() override;
	void Pause() override;
	void Resume() override;
	void Finish() override;
	void EventOccurred(Event *e) override;
	void Update(StageCmd *const cmd) override;
	void Render() override;
	bool IsTransition() override;
};

}; // namespace OpenApoc
