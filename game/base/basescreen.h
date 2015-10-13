#pragma once

#include "framework/stage.h"
#include "framework/includes.h"

#include "game/resources/gamecore.h"
#include "game/apocresources/apocresource.h"
#include "forms/forms.h"

#include <unordered_map>
#include <bitset>

namespace OpenApoc
{

class Base;
class Image;

class BaseScreen : public Stage
{
  private:
	static const std::unordered_map<std::bitset<4>, int> TILE_CORRIDORS;

	std::unique_ptr<Form> basescreenform;
	StageCmd stageCmd;
	Base &base;

	void RenderBase();

  public:
	BaseScreen(Framework &fw);
	~BaseScreen();
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
