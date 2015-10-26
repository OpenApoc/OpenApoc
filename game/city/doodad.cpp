#include "game/city/doodad.h"
#include "game/tileview/tile.h"
#include "game/tileview/tileobject_doodad.h"
#include "framework/framework.h"
#include "game/city/city.h"
#include "game/gamestate.h"

namespace OpenApoc
{

Doodad::Doodad(DoodadDef &def, Vec3<float> position) : def(def), position(position), age(0) {}

void Doodad::update(GameState &state, int ticks)
{
	age += ticks;
	if (age >= def.lifetime)
	{
		auto thisPtr = shared_from_this();
		this->tileObject->removeFromMap();
		this->tileObject = nullptr;
		state.city->doodads.erase(thisPtr);
		return;
	}
}

sp<Image> Doodad::getSprite()
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

const Vec2<int> &Doodad::getImageOffset() const { return this->def.imageOffset; }

} // namespace OpenApoc
