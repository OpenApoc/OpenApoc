
#pragma once

#include "framework/stage.h"

#include "forms/forms.h"
#include "game/apocresources/apocresource.h"

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
