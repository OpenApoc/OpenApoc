#pragma once

#include "framework/stage.h"
#include "library/vec.h"

#include "forms/forms.h"

#include <unordered_map>
#include <vector>

namespace OpenApoc
{

class Base;
class Facility;
class FacilityDef;
class GameState;

class BaseScreen : public Stage
{
  private:
	static const int TILE_SIZE;
	static const Vec2<int> NO_SELECTION;
	static const std::unordered_map<std::vector<bool>, int> TILE_CORRIDORS;

	sp<Form> form;
	StageCmd stageCmd;
	Base &base;
	Vec2<int> selection, mousePos;
	sp<const Facility> selFacility;
	sp<const FacilityDef> dragFacility;
	bool drag;

	sp<Graphic> baseView, selGraphic;
	sp<Label> selText, buildTime;
	std::vector<sp<Label>> statsLabels;
	std::vector<sp<Label>> statsValues;

	void RenderBase();
	int getCorridorSprite(Vec2<int> pos) const;
	sp<GameState> state;

  public:
	BaseScreen(sp<GameState> state);
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
