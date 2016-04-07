#pragma once
#include "framework/includes.h"
#include "framework/stage.h"
#include "library/sp.h"
#include <atomic>
#include <future>

namespace OpenApoc
{

class Image;

class LoadingScreen : public Stage
{
  private:
	std::future<void> loading_task;
	sp<Image> loadingimage;
	std::function<sp<Stage>()> nextScreenFn;
	sp<Image> backgroundimage;
	float loadingimageangle;

  public:
	LoadingScreen(std::future<void> task, std::function<sp<Stage>()> nextScreenFn,
	              sp<Image> background = nullptr);
	// Stage control
	virtual void Begin() override;
	virtual void Pause() override;
	virtual void Resume() override;
	virtual void Finish() override;
	virtual void EventOccurred(Event *e) override;
	virtual void Update(StageCmd *const cmd) override;
	virtual void Render() override;
	virtual bool IsTransition() override;
};

}; // namespace OpenApoc
