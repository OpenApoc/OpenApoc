#pragma once

#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <vector>

namespace OpenApoc
{
class Image;
class BattleUnitImagePack : public StateObject<BattleUnitImagePack>
{
  public:
	Vec2<float> image_offset;

	std::vector<sp<Image>> images;

	// high level api for loading
	bool loadImagePack(GameState &state, const UString &path);

	// high level api for saving
	bool saveImagePack(const UString &path, bool pack = true, bool pretty = false);

	// Function used when getting file path
	static const UString getNameFromID(UString id);

	static UString getImagePackPath();
};
} // namespace OpenApoc
