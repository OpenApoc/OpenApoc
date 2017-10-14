#pragma once

#include "forms/control.h"
#include "game/state/shared/equipment.h"
#include <array>
#include <set>

namespace OpenApoc
{

class EquipmentPaperDoll : public Control
{
  private:
	Vec2<int> slotSizePixels;
	sp<EquippableObject> object;
	std::set<EquipmentSlotType> slotHighlightTypes;
	std::array<Colour, 2> slotHighlightColours;
	// value between 0->2*PI - interpolating between the slotHighlightColours by sin(value)
	float slotHighlightCounter = 0.0f;
	float slotHighlightIncrement = 0.0f;
	Colour highlightColour = {0, 0, 0, 0};
	Colour nonHighlightColour = {0, 0, 0, 255};

  protected:
	void onRender() override;

  public:
	EquipmentPaperDoll(Vec2<int> position, Vec2<int> size, Vec2<int> slotSizePixels);
	Vec2<int> getSlotPositionFromScreenPosition(const Vec2<int> &screenPosition) const;
	Vec2<int> getScreenPositionFromSlotPosition(const Vec2<float> &slot) const;
	Vec2<int> getScreenPositionFromSlotPosition(const Vec2<int> &slot) const;
	void setObject(sp<EquippableObject> newObject);
	void setHighlightColours(const std::array<Colour, 2> &colours);
	void setHighlight(const std::set<EquipmentSlotType> &types);
	void setNonHighlightColour(const Colour &colour);
	void updateEquipment();

	void update() override;

	~EquipmentPaperDoll() override = default;
};

} // namespace OpenApoc
