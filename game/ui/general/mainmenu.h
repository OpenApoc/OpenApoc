#pragma once

#include "framework/stage.h"

namespace OpenApoc
{

class Form;

class MainMenu : public Stage
{
  private:
	sp<Form> mainmenuform;
	StageCmd stageCmd;

  public:
	MainMenu();
	~MainMenu();
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
