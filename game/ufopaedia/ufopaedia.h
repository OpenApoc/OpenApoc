
#pragma once
#include "library/sp.h"

#include "framework/includes.h"
#include "framework/stage.h"

#include "forms/forms.h"

#include "ufopaediacategory.h"

namespace OpenApoc
{

class Ufopaedia : public Stage
{
  private:
	sp<Form> menuform;
	StageCmd stageCmd;

  public:
	static std::vector<sp<UfopaediaCategory>> UfopaediaDB;

	Ufopaedia();
	~Ufopaedia();
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
