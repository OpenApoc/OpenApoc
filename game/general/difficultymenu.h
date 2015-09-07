
#pragma once

#include "framework/stage.h"
#include "framework/includes.h"

#include "game/resources/gamecore.h"
#include "game/apocresources/apocresource.h"
#include "forms/forms.h"

namespace OpenApoc
{

class DifficultyMenu : public Stage
{
  private:
	Form *difficultymenuform;
	StageCmd stageCmd;

  public:
	DifficultyMenu(Framework &fw);
	~DifficultyMenu();
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
} // namespace OpenApoc
