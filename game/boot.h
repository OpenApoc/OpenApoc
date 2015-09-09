
#pragma once

#include "framework/stage.h"
#include "framework/includes.h"

#include <future>
#include <atomic>

namespace OpenApoc
{

class Image;

class BootUp : public Stage
{
  private:
	std::shared_ptr<Image> loadingimage;
	std::shared_ptr<Image> logoimage;
	int loadtime;
	Angle<float> loadingimageangle;

	std::future<void> asyncGamecoreLoad;
	std::atomic<bool> gamecoreLoadComplete;

  public:
	BootUp(Framework &fw) : Stage(fw) {}
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
