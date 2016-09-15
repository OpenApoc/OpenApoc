#pragma once
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include <vector>

namespace OpenApoc
{
class Image;
class BattleUnitImagePack : public StateObject<BattleUnitImagePack>
{
  public:
	std::vector<sp<Image>> images;

	// high level api for loading
	bool loadImagePack(GameState &state, const UString &path);

	// high level api for saving
	bool saveImagePack(const UString &path, bool pack = true);

	// Function used when getting file path
	static const UString getNameFromID(UString id);

	static const UString imagePackPath;
};
}
