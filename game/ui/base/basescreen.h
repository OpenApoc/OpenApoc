#pragma once
#include "forms/forms.h"
#include "framework/stage.h"
#include "library/vec.h"
#include <unordered_map>
#include <vector>

namespace OpenApoc
{

class Base;
class Facility;
class GameState;
class FacilityType;

class BaseScreen : public Stage
{
  private:
	static const int TILE_SIZE, MINI_SIZE;
	static const Vec2<int> NO_SELECTION;
	static const std::unordered_map<std::vector<bool>, int> TILE_CORRIDORS;

	sp<Form> form;
	StageCmd stageCmd;
	StateRef<Base> base;
	Vec2<int> selection, mousePos;
	sp<Facility> selFacility;
	StateRef<FacilityType> dragFacility;
	bool drag;

	sp<Graphic> baseView, selGraphic;
	sp<Label> selText, buildTime;
	std::vector<sp<Label>> statsLabels;
	std::vector<sp<Label>> statsValues;
	std::vector<sp<GraphicButton>> miniViews;

	int getCorridorSprite(Vec2<int> pos) const;
	void RenderBase();
	void RenderMiniBase();
	sp<GameState> state;

	sp<RGBImage> minimap_image;

  public:
	BaseScreen(sp<GameState> state);
	~BaseScreen();
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
