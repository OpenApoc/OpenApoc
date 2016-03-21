#include "game/city/doodad.h"
#include "game/tileview/tile.h"
#include "game/tileview/tileobject_doodad.h"
#include "framework/framework.h"
#include "game/city/city.h"
#include "game/gamestate.h"

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
      lifetime(type->lifetime), type(type)
{
}

void Doodad::update(GameState &state, int ticks)
{
	if (!temporary)
		return;
	age += ticks;
	if (age >= lifetime)
	{
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
		city.second->doodads.erase(thisPtr);
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
	sp<Image> sprite;
	for (auto &f : type->frames)
	{
		sprite = f.image;
		animTime += f.time;
		if (animTime >= age)
			return sprite;
	}
	LogWarning("Doodad reached age %d with no frame", age);
	return sprite;
}

} // namespace OpenApoc
