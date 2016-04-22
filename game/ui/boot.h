#pragma once
#include "framework/includes.h"
#include "framework/stage.h"
#include "library/sp.h"

namespace OpenApoc
{

class Image;

class BootUp : public Stage
{
  private:
	StageCmd stageCmd;

  public:
	BootUp() : Stage() {}
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
