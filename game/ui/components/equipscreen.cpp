#define _USE_MATH_DEFINES

#include "game/ui/components/equipscreen.h"
#include "framework/framework.h"
#include "framework/renderer.h"
#include <cmath>

namespace OpenApoc
{

EquipmentPaperDoll::EquipmentPaperDoll(Vec2<int> position, Vec2<int> size, Vec2<int> slotSizePixels)
    : slotSizePixels(slotSizePixels), slotHighlightColours({Colour{0, 0, 0, 0}, Colour{0, 0, 0, 0}})
{
	this->Location = position;
	this->Size = size;

	this->slotHighlightIncrement = M_PI / 15.0f;
}

Vec2<int>
EquipmentPaperDoll::getSlotPositionFromScreenPosition(const Vec2<int> &screenPosition) const
{
	auto pos = screenPosition - this->resolvedLocation;
	return pos / slotSizePixels;
}

Vec2<int> EquipmentPaperDoll::getScreenPositionFromSlotPosition(const Vec2<int> &slot) const
{
	return this->resolvedLocation + slot * slotSizePixels;
}

Vec2<int> EquipmentPaperDoll::getScreenPositionFromSlotPosition(const Vec2<float> &slot) const
{
	return this->resolvedLocation + Vec2<int>(slot.x * slotSizePixels.x, slot.y * slotSizePixels.y);
}

void EquipmentPaperDoll::setObject(sp<EquippableObject> newObject)
{
	this->object = newObject;
	this->setDirty();
}

void EquipmentPaperDoll::setHighlightColours(const std::array<Colour, 2> &colours)
{
	this->slotHighlightColours = colours;
	this->setDirty();
}

void EquipmentPaperDoll::setHighlight(const std::set<EquipmentSlotType> &types)
{
	this->slotHighlightTypes = types;
	this->setDirty();
}

void EquipmentPaperDoll::setNonHighlightColour(const Colour &colour)
{
	this->nonHighlightColour = colour;
	this->setDirty();
}

void EquipmentPaperDoll::update()
{
	// Only draw highlight if one of the colours isn't 100% transparent
	if (slotHighlightColours[0].a != 0 || slotHighlightColours[1].a != 0)
	{
		this->slotHighlightCounter += slotHighlightIncrement;
		while (this->slotHighlightCounter > 2.0f * M_PI)
		{
			this->slotHighlightCounter -= 2.0f * M_PI;
		}
		// Scale the sin curve from (-1, 1) to (0, 1)
		float glowFactor = (sin(this->slotHighlightCounter) + 1.0f) / 2.0f;

		this->highlightColour.r =
		    mix(slotHighlightColours[0].r, slotHighlightColours[1].r, glowFactor);
		this->highlightColour.g =
		    mix(slotHighlightColours[0].g, slotHighlightColours[1].g, glowFactor);
		this->highlightColour.b =
		    mix(slotHighlightColours[0].b, slotHighlightColours[1].b, glowFactor);
		this->highlightColour.a =
		    mix(slotHighlightColours[0].a, slotHighlightColours[1].a, glowFactor);

		this->setDirty();
	}
}

static const EquipmentLayoutSlot *getSlotAtPosition(Vec2<int> pos,
                                                    const std::list<EquipmentLayoutSlot> &slots)
{
	for (auto &slot : slots)
	{
		if (slot.bounds.within(pos))
		{
			return &slot;
		}
	}
	return nullptr;
}

void EquipmentPaperDoll::onRender()
{
	Control::onRender();

	if (!object)
	{
		return;
	}
	// Draw all equipment after slot lines to ensure they're on top
	const auto &slotList = this->object->getSlots();
	if (object->drawLines())
	{
		for (auto &slot : slotList)
		{
			Vec2<int> p00 = (slot.bounds.p0 * slotSizePixels);
			Vec2<int> p11 = (slot.bounds.p1 * slotSizePixels);
			Vec2<int> p01 = {p00.x, p11.y};
			Vec2<int> p10 = {p11.x, p00.y};

			if (this->slotHighlightTypes.find(slot.type) != this->slotHighlightTypes.end())
			{
				if (this->highlightColour.a == 0)
				{
					continue;
				}
				fw().renderer->drawLine(p00, p01, highlightColour, 2);
				fw().renderer->drawLine(p01, p11, highlightColour, 2);
				fw().renderer->drawLine(p11, p10, highlightColour, 2);
				fw().renderer->drawLine(p10, p00, highlightColour, 2);
			}
			else
			{
				if (this->nonHighlightColour.a == 0)
				{
					continue;
				}
				fw().renderer->drawLine(p00, p01, nonHighlightColour, 2);
				fw().renderer->drawLine(p01, p11, nonHighlightColour, 2);
				fw().renderer->drawLine(p11, p10, nonHighlightColour, 2);
				fw().renderer->drawLine(p10, p00, nonHighlightColour, 2);
			}
		}
	}
	// Draw equipment
	auto equipment = this->object->getEquipment();
	for (auto &equipPair : equipment)
	{
		auto pos = equipPair.first;
		auto equipment = equipPair.second;
		auto *slot = getSlotAtPosition(pos, slotList);
		if (!slot)
		{
			LogWarning("Equipment at %s not in slot", pos);
		}
		auto equipmentSize = equipment->getEquipmentSlotSize();
		auto alignX = slot->align_x;
		auto alignY = slot->align_y;
		auto slotBounds = slot->bounds;

		pos *= slotSizePixels;

		int diffX = slotBounds.getWidth() - equipmentSize.x;
		int diffY = slotBounds.getHeight() - equipmentSize.y;

		sp<Image> image;

		switch (slot->type)
		{
			case EquipmentSlotType::ArmorBody:
			case EquipmentSlotType::ArmorHelmet:
			case EquipmentSlotType::ArmorLeftHand:
			case EquipmentSlotType::ArmorLegs:
			case EquipmentSlotType::ArmorRightHand:
				image = equipment->getEquipmentArmorImage();
				pos = {70, 40};
				break;
			default:
				image = equipment->getEquipmentImage();
				switch (alignX)
				{
					case AlignmentX::Left:
						pos.x += 0;
						break;
					case AlignmentX::Right:
						pos.x += diffX * slotSizePixels.x;
						break;
					case AlignmentX::Centre:
						pos.x += (diffX * slotSizePixels.x) / 2;
						break;
				}

				switch (alignY)
				{
					case AlignmentY::Top:
						pos.y += 0;
						break;
					case AlignmentY::Bottom:
						pos.y += diffY * slotSizePixels.y;
						break;
					case AlignmentY::Centre:
						pos.y += (diffY * slotSizePixels.y) / 2;
						break;
				}
				break;
		}

		fw().renderer->draw(image, pos);
	}
}

void EquipmentPaperDoll::updateEquipment() { this->setDirty(); }

} // namespace OpenApoc
