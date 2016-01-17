
#pragma once

#include "framework/stage.h"

#include "forms/forms.h"

namespace OpenApoc
{

class Building;

class BuildingScreen : public Stage
{
  private:
	std::unique_ptr<Form> menuform;
	StageCmd stageCmd;
	sp<Building> building;

  public:
	BuildingScreen(sp<Building> building);
	~BuildingScreen();
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
