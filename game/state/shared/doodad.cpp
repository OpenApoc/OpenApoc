#include "game/state/shared/doodad.h"
#include "framework/framework.h"
#include "game/state/battle/battle.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"
#include "game/state/rules/doodadtype.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_doodad.h"
#include <algorithm>

namespace OpenApoc
{

Doodad::Doodad(Vec3<float> position, Vec2<int> imageOffset, bool temporary, int lifetime,
               sp<Image> image)
    : position(position), imageOffset(imageOffset), temporary(temporary), age(0),
      lifetime(lifetime), sprite(image)
{
}

Doodad::Doodad(Vec3<float> position, StateRef<DoodadType> type)
    : position(position), imageOffset(type->imageOffset), temporary(true), age(0),
      lifetime(type->lifetime * TICKS_MULTIPLIER), type(type)
{
}

void Doodad::update(GameState &state, int ticks)
{
	if (!temporary)
		return;
	age += ticks;
	if (age >= lifetime)
	{
		if (this->type->repeatable)
		{
			age = 0;
			return;
		}
		this->remove(state);
		return;
	}
}

void Doodad::remove(GameState &state)
{
	auto thisPtr = shared_from_this();
	this->tileObject->removeFromMap();
	this->tileObject = nullptr;
	for (auto &city : state.cities)
	{
		city.second->doodads.remove(thisPtr);
	}
	if (state.current_battle)
		state.current_battle->doodads.remove(thisPtr);
}

void Doodad::setPosition(Vec3<float> position)
{
	this->position = position;
	this->tileObject->setPosition(position);
}

sp<Image> Doodad::getSprite()
{
	if (this->sprite)
		return sprite;
	int animTime = 0;
	sp<Image> frame;
	for (auto &f : type->frames)
	{
		frame = f.image;
		animTime += f.time * TICKS_MULTIPLIER;
		if (animTime > age)
			return frame;
	}
	LogWarning("Doodad reached age %d with no frame", age);
	return frame;
}

} // namespace OpenApoc
