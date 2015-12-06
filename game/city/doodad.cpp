#include "game/city/doodad.h"
#include "game/tileview/tile.h"
#include "game/tileview/tileobject_doodad.h"
#include "framework/framework.h"
#include "game/city/city.h"
#include "game/gamestate.h"

namespace OpenApoc
{

Doodad::Doodad(Vec3<float> position, Vec2<int> imageOffset, bool temporary, int lifetime)
    : position(position), imageOffset(imageOffset), temporary(temporary), age(0), lifetime(lifetime)
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
	state.city->doodads.erase(thisPtr);
}

void Doodad::setPosition(Vec3<float> position)
{
	this->position = position;
	this->tileObject->setPosition(position);
}

sp<Image> AnimatedDoodad::getSprite()
{
	int animTime = 0;
	sp<Image> sprite;
	for (auto &f : def.frames)
	{
		sprite = f.image;
		animTime += f.time;
		if (animTime >= age)
			return sprite;
	}
	LogWarning("Doodad \"%s\" reached age %d with no frame", def.ID.c_str(), age);
	return sprite;
}

sp<Image> StaticDoodad::getSprite() { return this->sprite; }

AnimatedDoodad::AnimatedDoodad(const DoodadDef &def, Vec3<float> position)
    : Doodad(position, def.imageOffset, true, def.lifetime), def(def)
{
}

StaticDoodad::StaticDoodad(sp<Image> sprite, Vec3<float> position, Vec2<int> imageOffset,
                           bool temporary, int lifetime)
    : Doodad(position, imageOffset, temporary, lifetime), sprite(sprite)
{
}
} // namespace OpenApoc
