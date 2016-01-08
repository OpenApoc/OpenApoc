#pragma once

#include "framework/stage.h"
#include "framework/includes.h"
#include "library/vec.h"

#include "game/resources/gamecore.h"
#include "game/apocresources/apocresource.h"
#include "forms/forms.h"

#include <unordered_map>
#include <vector>

namespace OpenApoc
{

class Base;
class Facility;

class BaseScreen : public Stage
{
  private:
	static const std::unordered_map<std::vector<bool>, int> TILE_CORRIDORS;
	static const int TILE_SIZE;

	std::unique_ptr<Form> basescreenform;
	StageCmd stageCmd;
	Base &base;
	Vec2<int> selection;
	sp<const Facility> selFacility;

	Graphic *baseView;
	Label *selText;
	Graphic *selGraphic;

	void RenderBase();
	int getCorridorSprite(Vec2<int> pos) const;

  public:
	BaseScreen();
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
